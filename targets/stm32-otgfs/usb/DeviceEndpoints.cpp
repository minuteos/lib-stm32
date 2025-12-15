/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * efm32-usb/usb/DeviceEndpoints.cpp
 */

#include <usb/DeviceEndpoints.h>
#include <usb/Device.h>

namespace usb
{

void DeviceInEndpoint::Reset()
{
    // break any ongoing transfer
    transferComplete = endpointDisabled = true;
    // put the endpoint in its initial state (also disables it on the bus)
    ep->DIEPCTL = USB_OTG_DIEPCTL_EPDIS | USB_OTG_DIEPCTL_SNAK | USB_OTG_DIEPCTL_SD0PID_SEVNFRM;
    ep->InterruptClear();
}

async(DeviceInEndpoint::Configure, const EndpointDescriptor *cfg)
async_def_sync()
{
    if (ep->IsActive())
        ep->Deactivate();

    if (cfg == NULL)
    {
        USBDEBUG("IN(%d) deactivated", ep->Index());
    }
    else
    {
        ep->Activate(cfg->wMaxPacketSize, cfg->type, ep->Index());
        USBDEBUG("IN(%d) MPS %d Type %s", ep->Index(), cfg->wMaxPacketSize, ((const char *[]){"CTL", "ISO", "BULK", "INT"})[cfg->type]);
        ep->InterruptEnable();
    }
}
async_end

async(DeviceInEndpoint::SendPacket, Span data, Timeout timeout)
async_def()
{
    ASSERT(!txWords);
    if (data.Length() > ep->MaxTransferSize())
    {
        USBDEBUG("SendPacket: data length %d exceeds MTS %d", data.Length(), ep->MaxTransferSize());
        data = data.Left(ep->MaxTransferSize());
    }

    txPtr = data.Pointer<uint32_t>();
    txWords = (data.Length() + 3) >> 2;
    transferComplete = endpointDisabled = false;
    USBDIAG("TX: EP%d %db %H", ep->Index(), data.Length(), data);
    ep->TransmitPacket(data.Length());
    if (txWords)
    {
        ep->InterruptEmpty(true);
    }
    // let the interrupt push the actual data
    if (!await_signal_timeout(transferComplete, timeout))
    {
        // stop receiving (in case of timeout)
        ep->Reset();
        // wait for the endpoint to be actually disabled
        await_signal(endpointDisabled);
        txWords = 0;
    }

    async_return(txPtr - data.Pointer<uint32_t>());
}
async_end

void DeviceInEndpoint::HandleInterrupts(uint32_t status)
{
    if (status & USB_OTG_DIEPINT_TXFE)
    {
        if (auto batch = std::min(txWords, size_t(ep->DTXFSTS & USB_OTG_DTXFSTS_INEPTFSAV)))
        {
            ep->Owner()->TxFifoWrite(ep->Index(), txPtr, batch * 4);
            txPtr += batch;
            txWords -= batch;
        }

        if (!txWords) { ep->InterruptEmpty(false); }
    }

    if (status & USB_OTG_DIEPINT_XFRC)
    {
        transferComplete = true;
    }

    if (status & USB_OTG_DIEPINT_EPDISD)
    {
        endpointDisabled = true;
    }
}

void DeviceOutEndpoint::Reset()
{
    // break any ongoing transfer
    transferComplete = endpointDisabled = true;
    // put the endpoint in its initial state (also disables it on the bus)
    ep->DOEPCTL = USB_OTG_DOEPCTL_EPDIS | USB_OTG_DOEPCTL_SNAK | USB_OTG_DOEPCTL_SD0PID_SEVNFRM;
    ep->InterruptClear();
}

async(DeviceOutEndpoint::Configure, const EndpointDescriptor *cfg)
async_def_sync()
{
    if (ep->IsActive())
        ep->Deactivate();

    if (cfg == NULL)
    {
        USBDEBUG("OUT(%d) deactivated", ep->Index());
    }
    else
    {
        ep->Activate(cfg->wMaxPacketSize, cfg->type);
        USBDEBUG("OUT(%d) MPS %d Type %s", ep->Index(), cfg->wMaxPacketSize, ((const char *[]){"CTL", "ISO", "BULK", "INT"})[cfg->type]);
        ep->InterruptEnable();
    }
}
async_end

async(DeviceOutEndpoint::ReceivePacket, Buffer buffer, Timeout timeout)
async_def()
{
    ASSERT(!ep->IsEnabled());
    packetBuffer = buffer;
    transferComplete = endpointDisabled = false;
    ep->ReceivePacket(ep->PacketSize());
    if (!await_signal_timeout(transferComplete, timeout))
    {
        // stop receiving (in case of timeout)
        ep->Reset();
        // wait for the endpoint to be actually disabled
        await_signal(endpointDisabled);
    }

    async_return(transferComplete ? packetBuffer.Length() : 0);
}
async_end

void DeviceOutEndpoint::PacketReceived(volatile uint32_t& fifo, size_t size)
{
    ASSERT(!transferComplete);
    if (size > packetBuffer.Length())
    {
        USBDEBUG("Received packet larger than buffer (%d > %d)", size, packetBuffer.Length());
    }

    // align the size
    size_t words = (size + 3) >> 2;

    // read aligned data from RX FIFO
    auto p = packetBuffer.Pointer<uint32_t>();
    if (packetBuffer.Length() >= words * 4)
    {
        // typical case - buffer is large enough and aligned
        while (words--) { *p++ = fifo; }
    }
    else
    {
        // buffer is smaller than the received packet,
        // we may need to align the last word and discard the rest
        size_t toRead = packetBuffer.Length() >> 2;
        words -= toRead;
        while (toRead--) { *p++ = fifo; }
        if (packetBuffer.Length() & 3)
        {
            uint32_t tmp = fifo;
            memcpy(p, &tmp, packetBuffer.Length() & 3);
            words--;
        }
        while (words--) { auto discard = fifo; (void)discard; }
    }

    packetBuffer = packetBuffer.Left(size);
    transferComplete = true;

    USBDIAG("RX: EP%d %db %H", ep->Index(), packetBuffer.Length(), packetBuffer);
}

}

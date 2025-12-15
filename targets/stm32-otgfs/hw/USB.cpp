/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32-otgfs/hw/USB.cpp
 */

#include <hw/USB.h>
#include <hw/RCC.h>

const GPIOPinTable_t _USB::afSof = GPIO_PINS(pA(8, 10));
const GPIOPinTable_t _USB::afId = GPIO_PINS(pA(10, 10));
const GPIOPinTable_t _USB::afDm = GPIO_PINS(pA(11, 10));
const GPIOPinTable_t _USB::afDp = GPIO_PINS(pA(12, 10));
const GPIOPinTable_t _USB::afNoe = GPIO_PINS(pC(9, 10), pA(13, 10));

void _USB::RxFifoRead(unsigned ep, void* buf, size_t bytes)
{
    if (bytes)
    {
        auto& r = DFIFO[ep].u32;
        auto p = (uint32_t*)buf;
        auto e = (const uint32_t*)((const uint8_t*)buf + bytes);

        do { *p++ = r; } while (p < e);
    }
}

void _USB::RxFifoDiscard(unsigned ep, size_t bytes)
{
    if (bytes)
    {
        auto& r = DFIFO[ep].u32;
        do { auto discard = r; (void)discard; bytes -= 4; } while (int(bytes) > 0);
    }
}

void _USB::TxFifoWrite(unsigned ep, const void* buf, size_t bytes)
{
    if (bytes)
    {
        auto& r = DFIFO[ep].u32;
        auto p = (const uint32_t*)buf;
        auto e = (const uint32_t*)((const uint8_t*)buf + bytes);

        do { r = *p++; } while (p < e);
    }
}

void USBOutEndpoint::ReceivePacket(size_t size)
{
    if (size)
    {
        ASSERT(size <= MaxTransferSize());

        auto maxpkt = PacketSize();
        DOEPTSIZ = (((size - 1) / maxpkt + 1) << USB_OTG_DOEPTSIZ_PKTCNT_Pos) |
            (size << USB_OTG_DOEPTSIZ_XFRSIZ_Pos);
    }
    else
    {
        // zero-length packet
        DOEPTSIZ = 1 << USB_OTG_DOEPTSIZ_PKTCNT_Pos;
    }
    Control(USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK);
}

void USBInEndpoint::TransmitPacket(size_t size)
{
    if (size)
    {
        ASSERT(size <= MaxTransferSize());

        auto maxpkt = PacketSize();
        DIEPTSIZ = (((size - 1) / maxpkt + 1) << USB_OTG_DIEPTSIZ_PKTCNT_Pos) |
            (size << USB_OTG_DIEPTSIZ_XFRSIZ_Pos);
    }
    else
    {
        // zero-length packet
        DIEPTSIZ = 1 << USB_OTG_DIEPTSIZ_PKTCNT_Pos;
    }
    Control(USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_CNAK);
}

void USBInEndpoint::InterruptEmpty(bool enable)
{
    MODBIT(Owner()->DEV.DIEPEMPMSK, Index(), enable);
    if (!enable)
    {
        // clear the interrupt after masking it to avoid unnecessary extra firing
        DIEPINT = USB_OTG_DIEPINT_TXFE;
    }
}

#ifdef Ckernel

//! Based on section 37.3.2 and 37.4.1 of EFM32GG11 Reference Manual
async(_USB::CoreEnable)
async_def()
{
    PWR->CR2 |= PWR_CR2_USV;  // enable USB voltage detector
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
    __DSB();
    USBDEBUG("Core clock enabled");
}
async_end

async(_USB::CoreReset)
async_def()
{
    PCGCCTL = 0;

    GRSTCTL |= USB_OTG_GRSTCTL_CSRST;
    await_mask(GRSTCTL, USB_OTG_GRSTCTL_AHBIDL, USB_OTG_GRSTCTL_AHBIDL);

    // mask all interrupts
    GINTMSK = 0;

    // enable global interrupt mask
    GAHBCFG |= USB_OTG_GAHBCFG_GINT;

    // reset all interrupts
    GINTSTS = ~0u;

    USBDEBUG("Core reset complete");
}
async_end

#endif

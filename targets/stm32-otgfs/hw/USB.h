/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32-otgfs/hw/USB.h
 */

#pragma once

#include <base/base.h>
#include <base/Span.h>

#include <hw/IRQ.h>
#include <hw/GPIO.h>

#ifdef Ckernel
#include <kernel/kernel.h>
#endif

#define USBDEBUG(...)  DBGCL("USB", __VA_ARGS__)

#if USB_DIAGNOSTICS
#define USBDIAG USBDEBUG
#else
#define USBDIAG(...)
#endif

#if USB_ENDPOINT_TRACE
#define USBEPTRACE(index, ptr, len) _CDBG(index, "%b", Span(ptr, len))
#else
#define USBEPTRACE(...)
#endif

#define USB_IN_ENDPOINTS    6 /* not defined in CMSIS */
#define USB_OUT_ENDPOINTS   6 /* not defined in CMSIS */

#define USB_SETUP_PACKET_SIZE   12

#define USB_XFERSIZE_EP0_MAX    0x7F    /* not defined in CMSIS */
#define USB_XFERSIZE_MAX        USB_OTG_DIEPTSIZ_XFRSIZ_Msk

#undef USB
#define USB CM_PERIPHERAL(_USB, USB_OTG_FS_PERIPH_BASE)

class USBInEndpoint : public USB_OTG_INEndpointTypeDef
{
    enum
    {
        USB_OTG_DIEPCTL_COMMAND_MASK = USB_OTG_DIEPCTL_EPENA |
            USB_OTG_DIEPCTL_EPDIS |
            USB_OTG_DIEPCTL_SD0PID_SEVNFRM |
            USB_OTG_DIEPCTL_SODDFRM |
            USB_OTG_DIEPCTL_SNAK |
            USB_OTG_DIEPCTL_CNAK |
            USB_OTG_DIEPCTL_STALL,
    };

public:
    // Gets the index of the endpoint
    constexpr unsigned Index() const { return ((uintptr_t)this & 0xFF) / sizeof(USBInEndpoint); }
    class _USB* Owner() const { return USB; } // there is just one USB peripheral

    // Gets the maximum packet size for the endpoint
    unsigned PacketSize() const
    {
        if (Index() == 0)
            return 64 >> ((DIEPCTL >> USB_OTG_DIEPCTL_MPSIZ_Pos) & 3);
        else
            return (DIEPCTL & USB_OTG_DIEPCTL_MPSIZ_Msk) >> USB_OTG_DIEPCTL_MPSIZ_Pos;
    }

    // Gets the maximum transfer size for the endpoint
    unsigned MaxTransferSize() const { return Index() == 0 ? USB_XFERSIZE_EP0_MAX : USB_XFERSIZE_MAX; }

    //! Determines if the endpoint has data ready to be transmitted
    bool IsEnabled() { return DIEPCTL & USB_OTG_DIEPCTL_EPENA; }
    //! Determines if the endpoint is responding to requests
    bool IsActive() { return DIEPCTL & USB_OTG_DIEPCTL_USBAEP; }
    //! Determines if the endpoint is stalled
    bool IsStalled() { return DIEPCTL & USB_OTG_DIEPCTL_STALL; }

    //! Abort current transaction and set NAK
    void Reset() { Control((IsEnabled() * USB_OTG_DIEPCTL_EPDIS) | USB_OTG_DIEPCTL_SNAK); }
    //! Make the endpoint unresponsive
    void Disable() { Control((IsEnabled() * USB_OTG_DIEPCTL_EPDIS) | USB_OTG_DIEPCTL_CNAK); }
    //! Stall the endpoint
    void Stall() { Control((IsEnabled() * USB_OTG_DIEPCTL_EPDIS) | USB_OTG_DIEPCTL_STALL); }
    //! Unstall the endpoint
    void Unstall() { Control(USB_OTG_DIEPCTL_SD0PID_SEVNFRM); }
    //! Stall or unstall the endpoint
    void Stall(bool stall) { if (stall != IsStalled()) { stall ? Stall() : Unstall(); } }
    //! Reinitialize the endpoint (put it in reset state)
    void Reinitialize() { DIEPCTL = USB_OTG_DIEPCTL_EPDIS | USB_OTG_DIEPCTL_SNAK | USB_OTG_DIEPCTL_SD0PID_SEVNFRM; }

    //! Activate the endpoint with the specified packet size and type
    void Activate(size_t maxPacketSize, unsigned type, unsigned fifo)
    {
        MODMASK(DIEPCTL,
            USB_OTG_DIEPCTL_COMMAND_MASK | USB_OTG_DIEPCTL_EPTYP | USB_OTG_DIEPCTL_TXFNUM | USB_OTG_DIEPCTL_MPSIZ,
            (maxPacketSize << USB_OTG_DIEPCTL_MPSIZ_Pos) |
            (type << USB_OTG_DIEPCTL_EPTYP_Pos) |
            (fifo << USB_OTG_DIEPCTL_TXFNUM_Pos) |
            USB_OTG_DIEPCTL_SD0PID_SEVNFRM |
            USB_OTG_DIEPCTL_USBAEP |
            USB_OTG_DIEPCTL_SNAK);
    }

    //! Deactiate the endpoint
    void Deactivate() { MODMASK(DIEPCTL, USB_OTG_DIEPCTL_COMMAND_MASK | USB_OTG_DIEPCTL_USBAEP, 0); }

    //! Reads and clears current interrupt state
    uint32_t InterruptClear() { auto state = DIEPINT; DIEPINT = state; return state; }
    //! Enables the interrupt for the endpoint
    void InterruptEnable();
    //! Disables the interrupt for the endpoint
    void InterruptDisable();

    //! Configures the endpoint to transmit a packet
    void TransmitPacket(size_t size);
    //! Enables or disables empty interrupt generation
    void InterruptEmpty(bool enable);

    void Control(uint32_t cmd) { MODMASK(DIEPCTL, USB_OTG_DIEPCTL_COMMAND_MASK, cmd); }
};

class USBOutEndpoint : public USB_OTG_OUTEndpointTypeDef
{
    enum
    {
        USB_OTG_DOEPCTL_COMMAND_MASK = USB_OTG_DOEPCTL_EPENA |
            USB_OTG_DOEPCTL_EPDIS |
            USB_OTG_DOEPCTL_SODDFRM |
            USB_OTG_DOEPCTL_SD0PID_SEVNFRM |
            USB_OTG_DOEPCTL_SNAK |
            USB_OTG_DOEPCTL_CNAK |
            USB_OTG_DOEPCTL_STALL,
    };

public:
    // Gets the index on the endpoint
    unsigned Index() const { return ((uintptr_t)this & 0xFF) / sizeof(USBOutEndpoint); }

    // Gets the maximum packet size for the endpoint
    unsigned PacketSize() const
    {
        if (Index() == 0)
            return 64 >> ((DOEPCTL >> USB_OTG_DOEPCTL_MPSIZ_Pos) & 3);
        else
            return (DOEPCTL & USB_OTG_DOEPCTL_MPSIZ_Msk) >> USB_OTG_DOEPCTL_MPSIZ_Pos;
    }

    // Gets the maximum transfer size for the endpoint
    unsigned MaxTransferSize() const { return Index() == 0 ? USB_XFERSIZE_EP0_MAX : USB_XFERSIZE_MAX; }

    //! Retrieves the number of setup packets before running out of buffer space
    unsigned SetupCount() { return (DOEPTSIZ & USB_OTG_DOEPTSIZ_STUPCNT) >> USB_OTG_DOEPTSIZ_STUPCNT_Pos; }

    //! Determines if the endpoint is ready to receive data
    bool IsEnabled() { return DOEPCTL & USB_OTG_DOEPCTL_EPENA; }
    //! Determines if the endpoint is responding to requests
    bool IsActive() { return DOEPCTL & USB_OTG_DOEPCTL_USBAEP; }
    //! Determines if the endpoint is stalled
    bool IsStalled() { return DOEPCTL & USB_OTG_DOEPCTL_STALL; }

    //! Abort current transaction and set NAK
    void Reset() { Control((IsEnabled() * USB_OTG_DOEPCTL_EPDIS) | USB_OTG_DOEPCTL_SNAK); }
    //! Make the endpoint unresponsive
    void Disable() { Control((IsEnabled() * USB_OTG_DOEPCTL_EPDIS) | USB_OTG_DOEPCTL_CNAK); }
    //! Stall the endpoint
    void Stall() { Control((IsEnabled() * USB_OTG_DOEPCTL_EPDIS) | USB_OTG_DOEPCTL_STALL); }
    //! Unstall the endpoint
    void Unstall() { Control(USB_OTG_DOEPCTL_SD0PID_SEVNFRM); }
    //! Stall or unstall the endpoint
    void Stall(bool stall) { if (stall != IsStalled()) { stall ? Stall() : Unstall(); } }

    //! Activate the endpoint with the specified packet size and type
    void Activate(size_t maxPacketSize, unsigned type)
    {
        MODMASK(DOEPCTL,
            USB_OTG_DOEPCTL_COMMAND_MASK | USB_OTG_DOEPCTL_EPTYP | USB_OTG_DOEPCTL_MPSIZ | USB_OTG_DOEPCTL_USBAEP,
            (maxPacketSize << USB_OTG_DOEPCTL_MPSIZ_Pos) |
            (type << USB_OTG_DOEPCTL_EPTYP_Pos) |
            USB_OTG_DOEPCTL_USBAEP | USB_OTG_DOEPCTL_SNAK);
    }
    //! Deactiate the endpoint
    void Deactivate() { MODMASK(DOEPCTL, USB_OTG_DOEPCTL_COMMAND_MASK | USB_OTG_DOEPCTL_USBAEP, 0); }
    //! Reinitialize the endpoint (put it in reset state)
    void Reinitialize() { DOEPCTL = USB_OTG_DOEPCTL_EPDIS | USB_OTG_DOEPCTL_SNAK | USB_OTG_DOEPCTL_SD0PID_SEVNFRM; }

    //! Reads and clears current interrupt state
    uint32_t InterruptClear() { auto state = DOEPINT; DOEPINT = state; return state; }
    //! Enables the interrupt for the endpoint
    void InterruptEnable();
    //! Disables the interrupt for the endpoint
    void InterruptDisable();

    //! Configures the endpoint to receive SETUP request packets
    void ConfigureSetup(size_t count)
    {
        ASSERT(Index() == 0);
        ASSERT(count <= 3);

        DOEPTSIZ = (count << USB_OTG_DOEPTSIZ_STUPCNT_Pos) |
            (1 << USB_OTG_DOEPTSIZ_PKTCNT_Pos) |
            ((count * USB_SETUP_PACKET_SIZE) << USB_OTG_DOEPTSIZ_XFRSIZ_Pos);
        Control(USB_OTG_DOEPCTL_EPENA);    // enable but keep NAK set
    }

    //! Configures the endpoint to receive a packet
    void ReceivePacket(size_t size);

    //! Gets the length of the last received packet
    uint32_t ReceivedLength() { return DOEPTSIZ & USB_OTG_DOEPTSIZ_XFRSIZ_Msk; }

    void Control(uint32_t cmd) { MODMASK(DOEPCTL, USB_OTG_DOEPCTL_COMMAND_MASK, cmd); }
};

class _USB : public USB_OTG_GlobalTypeDef
{
public:
    // USB_OTG_GlobalTypeDef only contains the initial registers
    uint8_t __pad1[USB_OTG_DEVICE_BASE - sizeof(USB_OTG_GlobalTypeDef)];
    USB_OTG_DeviceTypeDef DEV;
    uint8_t __pad2[USB_OTG_IN_ENDPOINT_BASE - USB_OTG_DEVICE_BASE - sizeof(USB_OTG_DeviceTypeDef)];
    USBInEndpoint IEP[USB_IN_ENDPOINTS];
    uint8_t __pad3[USB_OTG_OUT_ENDPOINT_BASE - USB_OTG_IN_ENDPOINT_BASE - sizeof(USBInEndpoint) * USB_IN_ENDPOINTS];
    USBOutEndpoint OEP[USB_OUT_ENDPOINTS];
    uint8_t __pad4[USB_OTG_PCGCCTL_BASE - USB_OTG_OUT_ENDPOINT_BASE - sizeof(USBOutEndpoint) * USB_OUT_ENDPOINTS];
    __IO uint32_t PCGCCTL;
    uint8_t __pad5[USB_OTG_FIFO_BASE - USB_OTG_PCGCCTL_BASE - sizeof(uint32_t)];
    struct {
        __IO uint32_t u32;
        uint8_t __pad[USB_OTG_FIFO_SIZE - sizeof(uint32_t)];
    } DFIFO[6];

    enum DeviceFlags
    {
        SpeedFull = USB_OTG_DCFG_DSPD,

        PeriodicFrame80 = 0,
        PeriodicFrame85 = USB_OTG_DCFG_PFIVL_0,
        PeriodicFrame90 = USB_OTG_DCFG_PFIVL_1,
        PeriodicFrame95 = USB_OTG_DCFG_PFIVL,

        NonZeroLengthStatusOutHandshake = USB_OTG_DCFG_NZLSOHSK,

        _SetupMask = USB_OTG_DCFG_DSPD | USB_OTG_DCFG_PFIVL | USB_OTG_DCFG_NZLSOHSK,
    };

    enum ThresholdFlags
    {
        ThresholdNone = 0,
        ThresholdRx = USB_OTG_DTHRCTL_RXTHREN,
        ThresholdIsoTx = USB_OTG_DTHRCTL_ISOTHREN,
        ThresholdNonIsoTx = USB_OTG_DTHRCTL_NONISOTHREN,
        ThresholdTx = ThresholdIsoTx | ThresholdNonIsoTx,

        _ThresholdMask = USB_OTG_DTHRCTL_RXTHREN | USB_OTG_DTHRCTL_ISOTHREN | USB_OTG_DTHRCTL_NONISOTHREN,
    };

    void EnablePHY(bool enable = true) { MODMASK(GCCFG, USB_OTG_GCCFG_PWRDWN, enable ? USB_OTG_GCCFG_PWRDWN : 0); }

    void ForceDevice() { MODMASK(GUSBCFG, USB_OTG_GUSBCFG_FDMOD | USB_OTG_GUSBCFG_FHMOD, USB_OTG_GUSBCFG_FDMOD); }
    void ForceHost() { MODMASK(GUSBCFG, USB_OTG_GUSBCFG_FDMOD | USB_OTG_GUSBCFG_FHMOD, USB_OTG_GUSBCFG_FHMOD); }

    void DeviceSetup(DeviceFlags flags) { MODMASK_SAFE(DEV.DCFG, _SetupMask, flags); }
    void DeviceAddress(uint32_t addr) { MODMASK_SAFE(DEV.DCFG, USB_OTG_DCFG_DAD, addr << USB_OTG_DCFG_DAD_Pos); }

    void DeviceConnect() { DEV.DCTL &= ~USB_OTG_DCTL_SDIS; }
    void DeviceDisconnect() { DEV.DCTL |= USB_OTG_DCTL_SDIS; }
    void DeviceControl(uint32_t ctl) { DEV.DCTL |= ctl; }
    void DeviceGlobalInNak(bool set) { DeviceControl(set ? USB_OTG_DCTL_SGINAK : USB_OTG_DCTL_CGINAK); }
    void DeviceGlobalOutNak(bool set) { DeviceControl(set ? USB_OTG_DCTL_SGONAK : USB_OTG_DCTL_CGONAK); }

    bool DeviceFullSpeed() { return (DEV.DSTS & USB_OTG_DSTS_ENUMSPD) == USB_OTG_DSTS_ENUMSPD; }

    uint32_t DeviceEndpointInterrupts() { return DEV.DAINT & DEV.DAINTMSK; }

    void DeviceThresholdEnable(ThresholdFlags flags) { MODMASK_SAFE(DEV.DTHRCTL, _ThresholdMask, flags); }

    void RxFifoSetup(size_t words) { GRXFSIZ = words << USB_OTG_GRXFSIZ_RXFD_Pos; }
    void TxFifo0Setup(size_t start, size_t words) { DIEPTXF0_HNPTXFSIZ = (start << USB_OTG_TX0FSA_Pos) | (words << USB_OTG_TX0FD_Pos); }
    void TxFifoSetup(unsigned n, size_t start, size_t words) { DIEPTXF[n - 1] = (start << USB_OTG_DIEPTXF_INEPTXSA_Pos) | (words << USB_OTG_DIEPTXF_INEPTXFD_Pos); }

    void TxFifoFlush(unsigned n) { GRSTCTL = USB_OTG_GRSTCTL_TXFFLSH | (n << USB_OTG_GRSTCTL_TXFFLSH_Pos); while (GRSTCTL & USB_OTG_GRSTCTL_TXFFLSH); }
    void TxFifoFlushAll() { GRSTCTL = USB_OTG_GRSTCTL_TXFFLSH | USB_OTG_GRSTCTL_TXFNUM; while (GRSTCTL & USB_OTG_GRSTCTL_TXFFLSH); }
    void RxFifoFlush() { GRSTCTL = USB_OTG_GRSTCTL_RXFFLSH; while (GRXSTSP & USB_OTG_GRSTCTL_RXFFLSH); }

    void RxFifoRead(unsigned n, void* buf, size_t bytes);
    void RxFifoDiscard(unsigned n, size_t bytes);
    void TxFifoWrite(unsigned n, const void* buf, size_t bytes);

    USBInEndpoint& In(unsigned n) { return IEP[n]; }
    USBOutEndpoint& Out(unsigned n) { return OEP[n]; }

    class IRQ IRQ() const { return OTG_FS_IRQn; }

    void CoreInterruptEnable() { GAHBCFG |= USB_OTG_GAHBCFG_GINT; }
    void CoreInterruptDisable() { GAHBCFG &= ~USB_OTG_GAHBCFG_GINT; }

#ifdef Ckernel
    async(CoreEnable);
    async(CoreReset);
#endif

    static const GPIOPinTable_t afSof, afId, afDm, afDp, afNoe;

    // pin configuration
    void ConfigureSof(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::SpeedHigh)
        { pin.ConfigureAlternate(afSof, mode); }
    void ConfigureId(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::SpeedHigh)
        { pin.ConfigureAlternate(afId, mode); }
    void ConfigureDataM(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::SpeedHigh)
        { pin.ConfigureAlternate(afDm, mode); }
    void ConfigureDataP(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::SpeedHigh)
        { pin.ConfigureAlternate(afDp, mode); }
    void ConfigureNoe(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::SpeedHigh)
        { pin.ConfigureAlternate(afNoe, mode); }
};

DEFINE_FLAG_ENUM(_USB::DeviceFlags);
DEFINE_FLAG_ENUM(_USB::ThresholdFlags);

ALWAYS_INLINE void USBInEndpoint::InterruptEnable() { SETBIT(USB->DEV.DAINTMSK, USB_OTG_DAINTMSK_IEPM_Pos + Index()); }
ALWAYS_INLINE void USBInEndpoint::InterruptDisable() { RESBIT(USB->DEV.DAINTMSK, USB_OTG_DAINTMSK_IEPM_Pos + Index()); }
ALWAYS_INLINE void USBOutEndpoint::InterruptEnable() { SETBIT(USB->DEV.DAINTMSK, USB_OTG_DAINTMSK_OEPM_Pos + Index()); }
ALWAYS_INLINE void USBOutEndpoint::InterruptDisable() { RESBIT(USB->DEV.DAINTMSK, USB_OTG_DAINTMSK_OEPM_Pos + Index()); }
/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/USART.h
 */

#pragma once

#include <base/base.h>
#include <base/Span.h>

#include <hw/DMA.h>
#include <hw/GPIO.h>
#include <hw/IRQ.h>
#include <hw/ConfigRegister.h>

struct USART : USART_TypeDef
{
    //! Gets the zero-based index of the peripheral
    constexpr unsigned Index() const { return unsigned(this) == USART1_BASE ? 0 : ((unsigned(this) >> 10) & 15); }

    struct SyncTransferDescriptor
    {
        static const uint8_t s_zero;
        static uint8_t s_discard;

        DMADescriptor rx, tx;

        void Transmit(Span d)
        {
            rx = DMADescriptor::Transfer((const void*)NULL, &s_discard, d.Length(), DMADescriptor::UnitByte | DMADescriptor::P2M);
            tx = DMADescriptor::Transfer(d, NULL, d.Length(), DMADescriptor::UnitByte | DMADescriptor::M2P);
        }

        void Receive(Buffer d)
        {
            rx = DMADescriptor::Transfer((const void*)NULL, d.Pointer(), d.Length(), DMADescriptor::UnitByte | DMADescriptor::P2M);
            tx = DMADescriptor::Transfer(&s_zero, NULL, d.Length(), DMADescriptor::UnitByte | DMADescriptor::M2P);
        }
    };

    GPIOPinID GetCsLocation(GPIOPin pin) { return pin.GetID(); }
    bool BindCs(GPIOPinID pin);
    //! Attempts to bind to the specified CS pin; returns false if CS is already bound
    async(BindCs, GPIOPinID pin, Timeout timeout = Timeout::Infinite);
    //! Releases the currently bound CS pin
    void ReleaseCs();

    //! Performs a chain of synchronous transfers
    async(SyncTransfer, SyncTransferDescriptor* descriptors, size_t count);

    //! Configures the baud rate
    unsigned BaudRate(unsigned rate);

    //! Checks whether the peripheral is enabled
    bool IsEnabled() const { return CR1 & USART_CR1_UE; }

    #pragma region Configuration registers

    using _CR1 = ConfigRegister<&USART_TypeDef::CR1>;
    using _CR2 = ConfigRegister<&USART_TypeDef::CR2>;
    using _CR3 = ConfigRegister<&USART_TypeDef::CR3>;

    #pragma region CR1 (enable, interrupts, format)

    static constexpr auto Enable(bool enable = true) { return _CR1(enable, USART_CR1_UE); }
    static constexpr auto StopEnable(bool enable = true) { return _CR1(enable, USART_CR1_UESM); }
    static constexpr auto Rx(bool enable = true) { return _CR1(enable, USART_CR1_RE); }
    static constexpr auto Tx(bool enable = true) { return _CR1(enable, USART_CR1_TE); }

    static constexpr auto InterruptIdle(bool enable = true) { return _CR1(enable, USART_CR1_IDLEIE); }
    static constexpr auto InterruptRxReady(bool enable = true) { return _CR1(enable, USART_CR1_RXNEIE); }
    static constexpr auto InterruptTxComplete(bool enable = true) { return _CR1(enable, USART_CR1_TCIE); }
    static constexpr auto InterruptTxEmpty(bool enable = true) { return _CR1(enable, USART_CR1_TXEIE); }
    static constexpr auto InterruptParityError(bool enable = true) { return _CR1(enable, USART_CR1_PEIE); }
    static constexpr auto InterruptCharMatch(bool enable = true) { return _CR1(enable, USART_CR1_CMIE); }
    static constexpr auto InterruptRxTimeout(bool enable = true) { return _CR1(enable, USART_CR1_RTOIE); }
    static constexpr auto InterruptEndOfBlock(bool enable = true) { return _CR1(enable, USART_CR1_EOBIE); }

    enum ParityMode
    {
        ParityNone = 0,
        ParityEven = USART_CR1_PCE,
        ParityOdd = USART_CR1_PCE | USART_CR1_PS,
    };

    static constexpr auto Parity(ParityMode p) { return _CR1(uint32_t(p), USART_CR1_PCE | USART_CR1_PS); }
    static constexpr auto DataBits(unsigned n)
    {
        ASSERT(n >= 7 && n <= 9);
        return _CR1(USART_CR1_M1 * (n == 7) | USART_CR1_M0 * (n == 9), USART_CR1_M0 | USART_CR1_M1);
    }

    enum MuteMode
    {
        MuteNone = 0,
        MuteIdle = USART_CR1_MME,
        MuteAddress = USART_CR1_MME | USART_CR1_WAKE,
    };

    static constexpr auto Mute(MuteMode m) { return _CR1(uint32_t(m), USART_CR1_MME | USART_CR1_WAKE); }
    static constexpr auto DeAssertTime(uint32_t value) { return _CR1((value << USART_CR1_DEAT_Pos) & USART_CR1_DEAT_Msk, USART_CR1_DEAT); }
    static constexpr auto DeDeassertAssertTime(uint32_t value) { return _CR1((value << USART_CR1_DEDT_Pos) & USART_CR1_DEDT_Msk, USART_CR1_DEDT); }

    #pragma endregion

    #pragma region CR2 (special functions, synchronous mode, framing)

    static constexpr auto LongLinBreak(bool enable = true) { return _CR2(enable, USART_CR2_LBDL); }

    static constexpr auto InterruptLinBreak(bool enable = true) { return _CR2(enable, USART_CR2_LBDIE); }

    static constexpr auto LastBitClockPulse(bool enable = true) { return _CR2(enable, USART_CR2_LBCL); }
    static constexpr auto SPIMode(unsigned mode) { return _CR2(uint32_t((mode & 3) << USART_CR2_CPHA_Pos), USART_CR2_CPOL | USART_CR2_CPHA); }
    static constexpr auto ClockOutput(bool enable = true) { return _CR2(enable, USART_CR2_CLKEN); }

    static constexpr auto StopBits(float count) { return _CR2(USART_CR2_STOP_1 * (count > 1) | USART_CR2_STOP_0 * (int(count * 2) & 1), USART_CR2_STOP); }
    static constexpr auto LinMode(bool enable = true) { return _CR2(enable, USART_CR2_LINEN); }
    static constexpr auto SwapPins(bool enable = true) { return _CR2(enable, USART_CR2_SWAP); }
    static constexpr auto RxInvert(bool enable = true) { return _CR2(enable, USART_CR2_RXINV); }
    static constexpr auto TxInvert(bool enable = true) { return _CR2(enable, USART_CR2_TXINV); }
    static constexpr auto Invert(bool enable = true) { return _CR2(enable, USART_CR2_DATAINV); }
    static constexpr auto ReverseBits(bool enable = true) { return _CR2(enable, USART_CR2_MSBFIRST); }

    enum AutoBaudMode
    {
        AutoBaudDisable = 0,
        AutoBaudStartBit = USART_CR2_ABREN,
        AutoBaudFallToFall = USART_CR2_ABREN | USART_CR2_ABRMODE_0,
        AutoBaud7F = USART_CR2_ABREN | USART_CR2_ABRMODE_1,
        AutoBaud55 = USART_CR2_ABREN | USART_CR2_ABRMODE_0 | USART_CR2_ABRMODE_1,
    };

    static constexpr auto AutoBaud(AutoBaudMode mode) { return _CR2(uint32_t(mode), USART_CR2_ABREN | USART_CR2_ABRMODE); }

    static constexpr auto RxTimeout(bool enable = true) { return _CR2(enable, USART_CR2_RTOEN); }

    static constexpr auto Address4(uint8_t addr) { return _CR2(uint32_t(addr << USART_CR2_ADD_Pos), USART_CR2_ADD | USART_CR2_ADDM7); }
    static constexpr auto Address7(uint8_t addr) { return _CR2(addr << USART_CR2_ADD_Pos | USART_CR2_ADDM7, USART_CR2_ADD | USART_CR2_ADDM7); }

    #pragma endregion

    #pragma region CR3

    static constexpr auto InterruptError(bool enable = true) { return _CR3(enable, USART_CR3_EIE); }
    static constexpr auto InterruptCts(bool enable = true) { return _CR3(enable, USART_CR3_CTSIE); }

    static constexpr auto IrDAMode(bool enable = true) { return _CR3(enable, USART_CR3_IREN); }
    static constexpr auto IrDALowPower(bool enable = true) { return _CR3(enable, USART_CR3_IRLP); }
    static constexpr auto HalfDuplex(bool enable = true) { return _CR3(enable, USART_CR3_HDSEL); }
    static constexpr auto SmartNack(bool enable = true) { return _CR3(enable, USART_CR3_NACK); }
    static constexpr auto SmartCard(bool enable = true) { return _CR3(enable, USART_CR3_SCEN); }

    static constexpr auto RxDma(bool enable = true) { return _CR3(enable, USART_CR3_DMAR); }
    static constexpr auto TxDma(bool enable = true) { return _CR3(enable, USART_CR3_DMAT); }

    enum FlowControl
    {
        FlowControlNone = 0,
        FlowControlRts = USART_CR3_RTSE,
        FlowControlCts = USART_CR3_CTSE,
        FlowControlRtsCts = USART_CR3_RTSE | USART_CR3_CTSE,
    };

    static constexpr auto FlowControl(FlowControl ctl) { return _CR3(uint32_t(ctl), USART_CR3_CTSE | USART_CR3_RTSE); }
    static constexpr auto SampleOneBit(bool enable = true) { return _CR3(enable, USART_CR3_ONEBIT); }
    static constexpr auto Overrun(bool enable = true) { return _CR3(!enable, USART_CR3_OVRDIS); }
    static constexpr auto RxDmaStopOnError(bool enable = true) { return _CR3(enable, USART_CR3_DDRE); }
    static constexpr auto DeEnable(bool enable = true) { return _CR3(enable, USART_CR3_DEM); }
    static constexpr auto DeInvert(bool enable = true) { return _CR3(enable, USART_CR3_DEP); }
    static constexpr auto SmartRetry(unsigned count) { return _CR3(uint32_t(count << USART_CR3_SCARCNT_Pos), USART_CR3_SCARCNT); }

    enum WakeInterruptMode
    {
        WakeInterruptNone = 0,
        WakeInterruptAddress = USART_CR3_WUFIE,
        WakeInterruptStartBit = USART_CR3_WUFIE | USART_CR3_WUS_1,
        WakeInterruptRx = USART_CR3_WUFIE | USART_CR3_WUS_1 | USART_CR3_WUS_0,
    };

    static constexpr auto InterruptWake(WakeInterruptMode mode) { return _CR3(uint32_t(mode), USART_CR3_WUFIE | USART_CR3_WUS); }
    static constexpr auto InterruptSmartEarlyComplete(bool enable = true) { return _CR3(enable, BIT(24) /* USART_CR3_TCBGTIE */); }

    static constexpr auto ClockInStop(bool enable = true) { return _CR3(enable, USART_CR3_UCESM); }

    #pragma endregion

    DEFINE_CONFIGURE_METHOD(USART_TypeDef);

    #pragma endregion

    ALWAYS_INLINE class IRQ IRQ() const { return LOOKUP_TABLE(IRQn_Type, USART1_IRQn, USART2_IRQn, USART3_IRQn, UART4_IRQn, UART5_IRQn)[Index()]; }

#if Ckernel
    async_once(RxIdle, Timeout timeout = Timeout::Infinite) { return async_forward(WaitMask, ISR, USART_ISR_BUSY, 0, timeout); }
    async_once(TxIdle, Timeout timeout = Timeout::Infinite) { return async_forward(WaitMask, ISR, USART_ISR_TC, USART_ISR_TC, timeout); }
#endif

};

template<unsigned n>
struct _USART : USART
{
    static const GPIOPinTable_t afClk, afTx, afRx, afCts, afRts;

    //! Gets the zero-based index of the peripheral
    constexpr unsigned Index() const { return n - 1; }

    //! Enables peripheral clock
    void EnableClock() const;

    void ConfigureClk(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::SpeedMedium)
        { pin.ConfigureAlternate(afClk, mode); }
    void ConfigureTx(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::SpeedMedium)
        { pin.ConfigureAlternate(afTx, mode); }
    void ConfigureRx(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::SpeedMedium)
        { pin.ConfigureAlternate(afRx, mode); }
    void ConfigureCts(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::SpeedMedium)
        { pin.ConfigureAlternate(afCts, mode); }
    void ConfigureRts(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::SpeedMedium)
        { pin.ConfigureAlternate(afRts, mode); }

    DMAChannel* DmaRx() const;
    DMAChannel* DmaTx() const;
};

#undef USART1
using USART1_t = _USART<1>;
#define USART1  CM_PERIPHERAL(USART1_t, USART1_BASE)

template<> inline void USART1_t::EnableClock() const { RCC->APB2ENR |= RCC_APB2ENR_USART1EN; }
template<> inline DMAChannel* USART1_t::DmaRx() const { return DMA::ClaimChannel({ 0, 5, 2 }, { 1, 7, 2 }); }
template<> inline DMAChannel* USART1_t::DmaTx() const { return DMA::ClaimChannel({ 0, 4, 2 }, { 1, 6, 2 }); }

#undef USART2
using USART2_t = _USART<2>;
#define USART2  CM_PERIPHERAL(USART2_t, USART2_BASE)

template<> inline void USART2_t::EnableClock() const { RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN; }
template<> inline DMAChannel* USART2_t::DmaRx() const { return DMA::ClaimChannel({ 0, 6, 2 }); }
template<> inline DMAChannel* USART2_t::DmaTx() const { return DMA::ClaimChannel({ 0, 7, 2 }); }

#undef USART3
using USART3_t = _USART<3>;
#define USART3  CM_PERIPHERAL(USART3_t, USART3_BASE)

template<> inline void USART3_t::EnableClock() const { RCC->APB1ENR1 |= RCC_APB1ENR1_USART3EN; }
template<> inline DMAChannel* USART3_t::DmaRx() const { return DMA::ClaimChannel({ 0, 3, 2 }); }
template<> inline DMAChannel* USART3_t::DmaTx() const { return DMA::ClaimChannel({ 0, 2, 2 }); }

#undef UART4
using UART4_t = _USART<4>;
#define UART4   CM_PERIPHERAL(UART4_t, UART4_BASE)

template<> inline void UART4_t::EnableClock() const { RCC->APB1ENR1 |= RCC_APB1ENR1_UART4EN; }
template<> inline DMAChannel* UART4_t::DmaRx() const { return DMA::ClaimChannel({ 1, 5, 2 }); }
template<> inline DMAChannel* UART4_t::DmaTx() const { return DMA::ClaimChannel({ 1, 3, 2 }); }

#undef UART5
using UART5_t = _USART<5>;
#define UART5   CM_PERIPHERAL(UART5_t, USART5_BASE)

template<> inline void UART5_t::EnableClock() const { RCC->APB1ENR1 |= RCC_APB1ENR1_UART5EN; }

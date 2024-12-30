/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/TIM.h
 */

#include <base/base.h>

#include <hw/RCC.h>
#include <hw/GPIO.h>

struct TIM : TIM_TypeDef
{
    ALWAYS_INLINE uint32_t Count() const { return CNT; }
    ALWAYS_INLINE void Count(uint32_t count) { CNT = count; }

    ALWAYS_INLINE void Enable() { CR1 |= TIM_CR1_CEN; }
    ALWAYS_INLINE void Disable() { CR1 &= ~TIM_CR1_CEN; }

    enum struct OCMode
    {
        Frozen = 0,
        MatchOn = TIM_CCMR1_OC1M_0,
        MatchOff = TIM_CCMR1_OC1M_1,
        MatchToggle = TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0,
        Off = TIM_CCMR1_OC1M_2,
        On = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_0,
        PWM1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1,
        PWM2 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0,
        OPM1 = TIM_CCMR1_OC1M_3,
        OPM2 = TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_0,
        CombinedPWM1 = TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_2,
        CombinedPWM2 = TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_0,
        AsymmetricPWM1 = TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1,
        AsymmetricPWM2 = TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0,

        Fast = TIM_CCMR1_OC1FE,
        Preload = TIM_CCMR1_OC1PE,
        Clear = TIM_CCMR1_OC1CE,
    };

    DECLARE_FLAG_ENUM(OCMode);
};

DEFINE_FLAG_ENUM(TIM::OCMode);

typedef volatile uint32_t TIM_TypeDef::*TIM_regptr_t;

template<unsigned n>
struct _TIMChReg : TIM_TypeDef
{
    volatile uint32_t& CCR();
    volatile uint32_t& CCMR();
};

template<unsigned ntim> struct _TIM;

template <unsigned ntim, unsigned n>
struct _TIMChannel : _TIMChReg<n>
{
    static constexpr unsigned N = n;
    static const GPIOPinTable_t afPin;
    static const GPIOPinTable_t afNeg;

    //! Gets the timer to which this channel belongs
    ALWAYS_INLINE constexpr _TIM<ntim>& Timer() const { return *(_TIM<ntim>*)this; }
    //! Gets the zero-based index of the channel
    ALWAYS_INLINE constexpr unsigned Index() const { return n - 1; }

    static constexpr unsigned CCMR_OFFSET = ((n - 1) & 1) << 3;
    static constexpr uint32_t CCMR_MASK = 0xFF00FF << CCMR_OFFSET;
    static constexpr unsigned CCER_OFFSET = (n - 1) << 2;
    static constexpr uint32_t CCER_MASK = 3 << CCER_OFFSET;

    ALWAYS_INLINE void OutputCompare(TIM::OCMode mode) { MODMASK(this->CCMR(), CCMR_MASK, uint32_t(mode) << CCMR_OFFSET); }
    ALWAYS_INLINE void CompareValue(uint32_t compare) { this->CCR() = compare; }
    ALWAYS_INLINE void OutputCompare(TIM::OCMode mode, uint32_t compare) { OutputCompare(mode); CompareValue(compare); }
    ALWAYS_INLINE void OutputEnable(bool invert = false, bool enable = true) { MODMASK(this->CCER, CCER_MASK, (enable * TIM_CCER_CC1E | invert * TIM_CCER_CC1P) << CCER_OFFSET); }
    ALWAYS_INLINE void ComplementaryEnable(bool invert = true, bool enable = true) { MODMASK(this->CCER, CCER_MASK << 2, (enable * TIM_CCER_CC1E | invert * TIM_CCER_CC1P) << CCER_OFFSET << 2); }
    ALWAYS_INLINE void OutputDisable() { OutputEnable(false, false); }
    ALWAYS_INLINE void ComplementaryDisable() { OutputEnable(false, false); }

    ALWAYS_INLINE void ConfigureOutput(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::SpeedMedium)
        { pin.ConfigureAlternate(afPin, mode); }
    ALWAYS_INLINE void ConfigureComplementary(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::SpeedMedium)
        { pin.ConfigureAlternate(afNeg, mode); }
};

template <unsigned n>
struct _TIM : TIM
{
    static constexpr unsigned N = n;

    //! Enables peripheral clock
    void EnableClock() const;

    //! Gets the zero-based index of the peripheral
    ALWAYS_INLINE constexpr unsigned Index() const { return n - 1; }

    using TIM_t = _TIM;

    template<unsigned ch> ALWAYS_INLINE constexpr _TIMChannel<n, ch>& CH() { return *(_TIMChannel<n, ch>*)this; }
    ALWAYS_INLINE constexpr auto& CH1() { return CH<1>(); }
    ALWAYS_INLINE constexpr auto& CH2() { return CH<2>(); }
    ALWAYS_INLINE constexpr auto& CH3() { return CH<3>(); }
    ALWAYS_INLINE constexpr auto& CH4() { return CH<4>(); }
    ALWAYS_INLINE constexpr auto& CH5() { return CH<5>(); }
    ALWAYS_INLINE constexpr auto& CH6() { return CH<6>(); }
};

#ifdef TIM1
#undef TIM1
using TIM1_t = _TIM<1>;
#define TIM1    CM_PERIPHERAL(TIM1_t, TIM1_BASE)

template<> ALWAYS_INLINE void TIM1_t::EnableClock() const { RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; __DSB(); }

#endif

#ifdef TIM2
#undef TIM2
using TIM2_t = _TIM<2>;
#define TIM2    CM_PERIPHERAL(TIM2_t, TIM2_BASE)

template<> ALWAYS_INLINE void TIM2_t::EnableClock() const { RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN; __DSB(); }

#endif

#ifdef TIM3
#undef TIM3
using TIM3_t = _TIM<3>;
#define TIM3    CM_PERIPHERAL(TIM3_t, TIM3_BASE)

template<> ALWAYS_INLINE void TIM3_t::EnableClock() const { RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN; __DSB(); }

#endif

#ifdef TIM4
#undef TIM4
using TIM4_t = _TIM<4>;
#define TIM4    CM_PERIPHERAL(TIM4_t, TIM4_BASE)

template<> ALWAYS_INLINE void TIM4_t::EnableClock() const { RCC->APB1ENR1 |= RCC_APB1ENR1_TIM4EN; __DSB(); }

#endif

#ifdef TIM5
#undef TIM5
using TIM5_t = _TIM<5>;
#define TIM5    CM_PERIPHERAL(TIM5_t, TIM5_BASE)

template<> ALWAYS_INLINE void TIM5_t::EnableClock() const { RCC->APB1ENR1 |= RCC_APB1ENR1_TIM5EN; __DSB(); }

#endif

#ifdef TIM6
#undef TIM6
using TIM6_t = _TIM<6>;
#define TIM6    CM_PERIPHERAL(TIM6_t, TIM6_BASE)

template<> ALWAYS_INLINE void TIM6_t::EnableClock() const { RCC->APB1ENR1 |= RCC_APB1ENR1_TIM6EN; __DSB(); }

#endif

#ifdef TIM7
#undef TIM7
using TIM7_t = _TIM<7>;
#define TIM7    CM_PERIPHERAL(TIM7_t, TIM7_BASE)

template<> ALWAYS_INLINE void TIM7_t::EnableClock() const { RCC->APB1ENR1 |= RCC_APB1ENR1_TIM7EN; __DSB(); }

#endif

#ifdef TIM8
#undef TIM8
using TIM8_t = _TIM<8>;
#define TIM8    CM_PERIPHERAL(TIM8_t, TIM8_BASE)

template<> ALWAYS_INLINE void TIM8_t::EnableClock() const { RCC->APB2ENR |= RCC_APB2ENR_TIM8EN; __DSB(); }

#endif

#ifdef TIM15
#undef TIM15
using TIM15_t = _TIM<15>;
#define TIM15    CM_PERIPHERAL(TIM15_t, TIM15_BASE)

template<> ALWAYS_INLINE void TIM15_t::EnableClock() const { RCC->APB2ENR |= RCC_APB2ENR_TIM15EN; __DSB(); }

#endif

#ifdef TIM16
#undef TIM16
using TIM16_t = _TIM<16>;
#define TIM16    CM_PERIPHERAL(TIM16_t, TIM16_BASE)

template<> ALWAYS_INLINE void TIM16_t::EnableClock() const { RCC->APB2ENR |= RCC_APB2ENR_TIM16EN; __DSB(); }

#endif

#ifdef TIM17
#undef TIM17
using TIM17_t = _TIM<17>;
#define TIM17    CM_PERIPHERAL(TIM17_t, TIM17_BASE)

template<> ALWAYS_INLINE void TIM17_t::EnableClock() const { RCC->APB2ENR |= RCC_APB2ENR_TIM17EN; __DSB(); }

#endif

template<> struct _TIMChReg<1> : TIM_TypeDef
{
    volatile uint32_t& CCR() { return CCR1; }
    volatile uint32_t& CCMR() { return CCMR1; }
};

template<> struct _TIMChReg<2> : TIM_TypeDef
{
    volatile uint32_t& CCR() { return CCR2; }
    volatile uint32_t& CCMR() { return CCMR1; }
};

template<> struct _TIMChReg<3> : TIM_TypeDef
{
    volatile uint32_t& CCR() { return CCR3; }
    volatile uint32_t& CCMR() { return CCMR2; }
};

template<> struct _TIMChReg<4> : TIM_TypeDef
{
    volatile uint32_t& CCR() { return CCR4; }
    volatile uint32_t& CCMR() { return CCMR2; }
};

template<> struct _TIMChReg<5> : TIM_TypeDef
{
    volatile uint32_t& CCR() { return CCR5; }
    volatile uint32_t& CCMR() { return CCMR3; }
};

template<> struct _TIMChReg<6> : TIM_TypeDef
{
    volatile uint32_t& CCR() { return CCR6; }
    volatile uint32_t& CCMR() { return CCMR3; }
};

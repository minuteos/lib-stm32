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
    uint32_t Count() const { return CNT; }
    void Count(uint32_t count) { CNT = count; }

    void Enable() { CR1 |= TIM_CR1_CEN; }
    void Disable() { CR1 &= ~TIM_CR1_CEN; }

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

template <unsigned ntim, unsigned nch>
struct _TIMGPIO
{
    static const GPIOPinTable_t afPin;
    static const GPIOPinTable_t afNeg;
};

template <typename TTIM, unsigned n, unsigned ccr, unsigned ccmr>
struct _TIMChannel
{
    using TIMGPIO_t = _TIMGPIO<TTIM::N, n>;
    static constexpr unsigned N = n;
    static constexpr unsigned CCR_OFF = ccr;
    static constexpr unsigned CCMR_OFF = ccmr;

    union
    {
        TTIM tim;
        struct
        {
            uint8_t __ccr_offset[CCR_OFF];
            __IO uint32_t CCR;
        };
        struct
        {
            uint8_t __ccmr_offset[CCMR_OFF];
            __IO uint32_t CCMR;
        };
    };

    //! Gets the zero-based index of the channel
    constexpr unsigned Index() const { return n - 1; }

    static constexpr unsigned CCMR_OFFSET = ((n - 1) & 1) << 3;
    static constexpr uint32_t CCMR_MASK = 0xFF00FF << CCMR_OFFSET;
    static constexpr unsigned CCER_OFFSET = (n - 1) << 2;
    static constexpr uint32_t CCER_MASK = 3 << CCER_OFFSET;

    void OutputCompare(TIM::OCMode mode) { MODMASK(CCMR, CCMR_MASK, uint32_t(mode) << CCMR_OFFSET); }
    void CompareValue(uint32_t compare) { CCR = compare; }
    void OutputCompare(TIM::OCMode mode, uint32_t compare) { OutputCompare(mode); CompareValue(compare); }
    void OutputEnable(bool invert = false, bool enable = true) { MODMASK(tim.CCER, CCER_MASK, (enable * TIM_CCER_CC1E | invert * TIM_CCER_CC1P) << CCER_OFFSET); }
    void ComplementaryEnable(bool invert = true, bool enable = true) { MODMASK(tim.CCER, CCER_MASK << 2, (enable * TIM_CCER_CC1E | invert * TIM_CCER_CC1P) << CCER_OFFSET << 2); }
    void OutputDisable() { OutputEnable(false, false); }
    void ComplementaryDisable() { OutputEnable(false, false); }

    void ConfigureOutput(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::SpeedMedium)
        { pin.ConfigureAlternate(TIMGPIO_t::afPin, mode); }
    void ConfigureComplementary(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::SpeedMedium)
        { pin.ConfigureAlternate(TIMGPIO_t::afNeg, mode); }
};

template <unsigned n, unsigned apb, uint32_t enmask>
struct _TIM : TIM
{
    static constexpr unsigned N = n;

    //! Enables peripheral clock
    void EnableClock()
    {
        if constexpr (apb == 1)
        {
            RCC->APB1ENR1 |= enmask;
        }
        else if constexpr (apb == 2)
        {
            RCC->APB2ENR |= enmask;
        }
        else
        {
            static_assert(false);
        }
    }

    //! Gets the zero-based index of the peripheral
    constexpr unsigned Index() const { return n - 1; }

    using TIM_t = _TIM;

    constexpr auto& CH1() { return *(_TIMChannel<TIM_t, 1, offsetof(TIM_TypeDef, CCR1), offsetof(TIM_TypeDef, CCMR1)>*)this; }
    constexpr auto& CH2() { return *(_TIMChannel<TIM_t, 2, offsetof(TIM_TypeDef, CCR2), offsetof(TIM_TypeDef, CCMR1)>*)this; }
    constexpr auto& CH3() { return *(_TIMChannel<TIM_t, 3, offsetof(TIM_TypeDef, CCR3), offsetof(TIM_TypeDef, CCMR2)>*)this; }
    constexpr auto& CH4() { return *(_TIMChannel<TIM_t, 4, offsetof(TIM_TypeDef, CCR4), offsetof(TIM_TypeDef, CCMR2)>*)this; }
    constexpr auto& CH5() { return *(_TIMChannel<TIM_t, 5, offsetof(TIM_TypeDef, CCR5), offsetof(TIM_TypeDef, CCMR3)>*)this; }
    constexpr auto& CH6() { return *(_TIMChannel<TIM_t, 6, offsetof(TIM_TypeDef, CCR6), offsetof(TIM_TypeDef, CCMR3)>*)this; }
};

#ifdef TIM1
#undef TIM1
using TIM1_t = _TIM<1, 2, RCC_APB2ENR_TIM1EN>;
#define TIM1    CM_PERIPHERAL(TIM1_t, TIM1_BASE)
#endif

#ifdef TIM2
#undef TIM2
using TIM2_t = _TIM<2, 1, RCC_APB1ENR1_TIM2EN>;
#define TIM2    CM_PERIPHERAL(TIM2_t, TIM2_BASE)
#endif

#ifdef TIM3
#undef TIM3
using TIM3_t = _TIM<3, 1, RCC_APB1ENR1_TIM3EN>;
#define TIM3    CM_PERIPHERAL(TIM3_t, TIM3_BASE)
#endif

#ifdef TIM4
#undef TIM4
using TIM4_t = _TIM<4, 1, RCC_APB1ENR1_TIM4EN>;
#define TIM4    CM_PERIPHERAL(TIM4_t, TIM4_BASE)
#endif

#ifdef TIM5
#undef TIM5
using TIM5_t = _TIM<5, 1, RCC_APB1ENR1_TIM5EN>;
#define TIM5    CM_PERIPHERAL(TIM5_t, TIM5_BASE)
#endif

#ifdef TIM6
#undef TIM6
using TIM6_t = _TIM<6, 1, RCC_APB1ENR1_TIM6EN>;
#define TIM6    CM_PERIPHERAL(TIM6_t, TIM6_BASE)
#endif

#ifdef TIM7
#undef TIM7
using TIM7_t = _TIM<7, 1, RCC_APB1ENR1_TIM7EN>;
#define TIM7    CM_PERIPHERAL(TIM7_t, TIM7_BASE)
#endif

#ifdef TIM8
#undef TIM8
using TIM8_t = _TIM<8, 2, RCC_APB2ENR_TIM8EN>;
#define TIM8    CM_PERIPHERAL(TIM8_t, TIM8_BASE)
#endif

#ifdef TIM15
#undef TIM15
using TIM15_t = _TIM<15, 2, RCC_APB2ENR_TIM15EN>;
#define TIM15    CM_PERIPHERAL(TIM15_t, TIM15_BASE)
#endif

#ifdef TIM16
#undef TIM16
using TIM16_t = _TIM<16, 2, RCC_APB2ENR_TIM16EN>;
#define TIM16    CM_PERIPHERAL(TIM16_t, TIM16_BASE)
#endif

#ifdef TIM17
#undef TIM17
using TIM17_t = _TIM<17, 2, RCC_APB2ENR_TIM15EN>;
#define TIM17    CM_PERIPHERAL(TIM17_t, TIM17_BASE)
#endif

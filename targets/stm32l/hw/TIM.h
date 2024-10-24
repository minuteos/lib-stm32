/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/TIM.h
 */

#include <base/base.h>

#include <hw/RCC.h>

#ifdef TIM1
#undef TIM1
#define TIM1    CM_PERIPHERAL(TIM, TIM1_BASE)
#endif

#ifdef TIM2
#undef TIM2
#define TIM2    CM_PERIPHERAL(APB1TIM, TIM2_BASE)
#endif

#ifdef TIM3
#undef TIM3
#define TIM3    CM_PERIPHERAL(APB1TIM, TIM3_BASE)
#endif

#ifdef TIM4
#undef TIM4
#define TIM4    CM_PERIPHERAL(APB1TIM, TIM4_BASE)
#endif

#ifdef TIM5
#undef TIM5
#define TIM5    CM_PERIPHERAL(APB1TIM, TIM5_BASE)
#endif

#ifdef TIM6
#undef TIM6
#define TIM6    CM_PERIPHERAL(APB1TIM, TIM6_BASE)
#endif

#ifdef TIM7
#undef TIM7
#define TIM7    CM_PERIPHERAL(APB1TIM, TIM7_BASE)
#endif

#ifdef TIM8
#undef TIM8
#define TIM8    CM_PERIPHERAL(TIM, TIM8_BASE)
#endif

#ifdef TIM9
#undef TIM9
#define TIM9    CM_PERIPHERAL(TIM, TIM9_BASE)
#endif

#ifdef TIM10
#undef TIM10
#define TIM10    CM_PERIPHERAL(TIM, TIM10_BASE)
#endif

#ifdef TIM11
#undef TIM11
#define TIM11    CM_PERIPHERAL(TIM, TIM11_BASE)
#endif

#ifdef TIM12
#undef TIM12
#define TIM12    CM_PERIPHERAL(TIM, TIM12_BASE)
#endif

#ifdef TIM13
#undef TIM13
#define TIM13    CM_PERIPHERAL(TIM, TIM13_BASE)
#endif

#ifdef TIM14
#undef TIM14
#define TIM14    CM_PERIPHERAL(TIM, TIM14_BASE)
#endif

#ifdef TIM15
#undef TIM15
#define TIM15    CM_PERIPHERAL(TIM, TIM15_BASE)
#endif

#ifdef TIM16
#undef TIM16
#define TIM16    CM_PERIPHERAL(TIM, TIM16_BASE)
#endif

#ifdef TIM17
#undef TIM17
#define TIM17    CM_PERIPHERAL(TIM, TIM17_BASE)
#endif

struct TIM : TIM_TypeDef
{
};

struct APB1TIM : TIM
{
    //! Enables peripheral clock
    void EnableClock() { RCC->EnableTimer(Index()); }

    //! Gets the zero-based index of the peripheral
    unsigned Index() const { return ((unsigned)this - TIM2_BASE) / (TIM3_BASE - TIM2_BASE) + 1; }

    uint32_t Count() const { return CNT; }
    void Count(uint32_t count) { CNT = count; }

    void Enable() { CR1 |= TIM_CR1_CEN; }
    void Disable() { CR1 &= ~TIM_CR1_CEN; }
};

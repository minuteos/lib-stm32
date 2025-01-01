/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32-rtc-timing/base/stm32_rtc.cpp
 */

#include <base/base.h>

void _stm32_rtc_init()
{
    // enable PWR
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    __DSB();
    // enable RTC write access
    PWR->CR1 |= PWR_CR1_DBP;

    // make sure the RTC is running
#if STM32_USE_LSE
    auto rtcSel = RCC_BDCR_RTCSEL_0;        // LSE
#elif STM32_RTC_FROM_HSE
    auto rtcSel = RCC_BDCR_RTCSEL_0 | RCC_BDCR_RTCSEL_1;    // HSE / 32
#else
    auto rtcSel = RCC_BDCR_RTCSEL_1;        // LSI
#endif

    if ((RCC->BDCR & (RCC_BDCR_RTCEN | RCC_BDCR_RTCSEL)) != (RCC_BDCR_RTCEN | rtcSel))
    {
        // select proper clock for RTC and enable it
        RCC->BDCR |= RCC_BDCR_BDRST;
        __DSB();
        RCC->BDCR &= ~RCC_BDCR_BDRST;
        __DSB();
        RCC->BDCR |= RCC_BDCR_RTCEN | rtcSel;
    }

    // set RTC prescaler to full 15-bit synchronous mode (so SSR is updated as much as possible)
    // plus optional async prescaler from HSE
#if STM32_RTC_FROM_HSE
    auto prer = RTC_PRER_PREDIV_S | (STM32_RTC_FROM_HSE - 1) << RTC_PRER_PREDIV_A_Pos;
#else
    auto prer = RTC_PRER_PREDIV_S;
#endif
    if (RTC->PRER != prer)
    {
        // unlock RTC
        RTC->WPR = 0xCA;
        RTC->WPR = 0x53;

        // go to init mode
        RTC->ISR |= RTC_ISR_INIT;
        while (!(RTC->ISR & RTC_ISR_INITF))
            ;

        // set full sync mode (maximum SSR resolution)
        RTC->PRER = prer;
        // finish init
        RTC->ISR &= ~(RTC_ISR_INIT);

        // lock RTC
        RTC->WPR = 0;
    }

#ifdef DEBUG
    // freeze RTC when debugging
    DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_RTC_STOP;
#endif
}

CORTEX_PREINIT(!rtcinit, _stm32_rtc_init);

OPTIMIZE mono_t _stm32_rtc_mono()
{
    static struct
    {
        uint32_t last;
        mono_t mono;
    } state = {};

    auto val = RTC->SSR;
    auto s = state;
    if (auto elapsed = (s.last - val) & MASK(15))
    {
        s.last = val;
        s.mono += elapsed;
        state = s;
    }
    return s.mono;
}

OPTIMIZE mono_t _stm32_rtc_us()
{
    static struct
    {
        mono_t last;
        uint64_t t64;
    } state = {};

    auto mono = _stm32_rtc_mono();
    auto t64 = state.t64;
    if (auto elapsed = (mono - state.last))
    {
        state.last = mono;
        state.t64 = t64 += elapsed;
    }
    return (t64 * (((int64_t)1000000 << 32) / MONO_FREQUENCY)) >> 32;
}
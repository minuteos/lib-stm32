/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32-rtc-timing/base/stm32_rtc.cpp
 */

#include <base/base.h>

#include <hw/RTC.h>

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
        RTC->Unlock();

        // go to init mode
        RTC->ISR |= RTC_ISR_INIT;
        while (!(RTC->ISR & RTC_ISR_INITF))
            ;

        // set full sync mode (maximum SSR resolution)
        RTC->PRER = prer;
        // finish init
        RTC->ISR &= ~(RTC_ISR_INIT);

        RTC->Lock();
    }

    if (!(RTC->CR & RTC_CR_BYPSHAD))
    {
        // disable register shadowing - all we use is SSR
        RTC->Unlock();
        RTC->CR |= RTC_CR_BYPSHAD;
        RTC->Lock();
    }

#if DEBUG
    // freeze RTC when debugging
    DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_RTC_STOP;
    // allow debugging in low power modes
    DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP | DBGMCU_CR_DBG_STOP;
#endif

    // enable EXTI18 (RTC ALARM)
    EXTI->RTSR1 |= EXTI_RTSR1_RT18;
    EXTI->EMR1 |= EXTI_EMR1_EM18;
}

CORTEX_PREINIT(!rtcinit, _stm32_rtc_init);

static struct
{
    uint32_t last;
    mono_t mono;
} state = {};

OPTIMIZE mono_t _stm32_rtc_mono()
{
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

OPTIMIZE void _stm32_rtc_setup_wake(mono_t wakeAt)
{
    RTC->Unlock();

    while (!(RTC->ISR & RTC_ISR_ALRAWF));
    RTC->ISR &= ~RTC_ISR_ALRAF;
    // don't care about whole seconds
    RTC->ALRMAR = RTC_ALRMAR_MSK1 | RTC_ALRMAR_MSK2 | RTC_ALRMAR_MSK3 | RTC_ALRMAR_MSK4;

    // number of ticks to sleep
    auto sleepTicks = wakeAt - state.mono;
    // number of SSR ticks to sleep (cannot overflow)
    auto ssrSleep = std::min(MASK(15), unsigned(sleepTicks));
    // wake up when the fraction matches
    auto wakeSsr = (state.last - ssrSleep) & MASK(15);
    RTC->ALRMASSR = RTC_ALRMASSR_MASKSS | wakeSsr;
    // enable the alarm
    RTC->CR |= RTC_CR_ALRAE | RTC_CR_ALRAIE;
    // if the alarm time is very close, trigger a wakeup event manually so it isn't missed
    if (ssrSleep < MASK(14))
    {
        // the has overflowed
        auto toGo = (wakeSsr - RTC->SSR + 1) & MASK(15);
        if (toGo < 16)
        {
            // make sure an event is triggered
            __SEV();
        }
    }

    RTC->Lock();
}

OPTIMIZE void _stm32_rtc_clean_wake()
{
    RTC->Unlock();
    RTC->CR &= ~(RTC_CR_ALRAE | RTC_CR_ALRAIE);
    RTC->Lock();
}

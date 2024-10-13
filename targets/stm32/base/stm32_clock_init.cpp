/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32/base/stm32_clock_init.cpp
 */

#include <base/base.h>

void _stm32_clock_init()
{
    // enable PWR
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    // enable RTC write access
    PWR->CR1 |= PWR_CR1_DBP;

#if STM32_USE_LSE
    // make sure LSE is running
    if (!(RCC->BDCR & RCC_BDCR_LSERDY))
    {
        // enable LSE at maximum drive
        RCC->BDCR |= RCC_BDCR_LSEON | RCC_BDCR_LSEDRV;
        // wait for LSE lock
        while (!(RCC->BDCR & RCC_BDCR_LSERDY))
            ;
    }

    // lower LSE drive
    RCC->BDCR &= ~RCC_BDCR_LSEDRV;
#endif

    // make sure the RTC is running
    if (!(RCC->BDCR & RCC_BDCR_RTCEN))
    {
        // select LSE/LSI for RTC
#if STM32_USE_LSE
        MODMASK(RCC->BDCR, RCC_BDCR_RTCSEL, RCC_BDCR_RTCSEL_0);
#else
        MODMASK(RCC->BDCR, RCC_BDCR_RTCSEL, RCC_BDCR_RTCSEL_1);
#endif

        // enable RTC
        RCC->BDCR |= RCC_BDCR_RTCEN;
    }

    // set RTC prescaler to full synchronous mode
    if (RTC->PRER != RTC_PRER_PREDIV_S)
    {
        // unlock RTC
        RTC->WPR = 0xCA;
        RTC->WPR = 0x53;

        // go to init mode
        RTC->ISR |= RTC_ISR_INIT;
        while (!(RTC->ISR & RTC_ISR_INITF))
            ;

        // set full sync mode (maximum SSR resolution)
        RTC->PRER = RTC_PRER_PREDIV_S;
        // finish init
        RTC->ISR &= ~(RTC_ISR_INIT);

        // lock RTC
        RTC->WPR = 0;
    }
}

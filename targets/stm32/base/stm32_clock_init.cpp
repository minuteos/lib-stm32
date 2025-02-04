/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32/base/stm32_clock_init.cpp
 */

#include <base/base.h>

void _stm32_sysclk_init()
{
    // maximum wait states
    MODMASK(FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_4WS);
    // make sure performance voltage range is configured
    MODMASK(PWR->CR1, PWR_CR1_VOS, PWR_CR1_VOS_0);
    // jump to maximum MSI clock for continued init
    MODMASK(RCC->CR, RCC_CR_MSIRANGE, RCC_CR_MSIRANGE_11 | RCC_CR_MSIRGSEL);
    // set MSI as 48MHz clock (USB, SDMMC)
    MODMASK(RCC->CCIPR, RCC_CCIPR_CLK48SEL, RCC_CCIPR_CLK48SEL_0 | RCC_CCIPR_CLK48SEL_1);
    SystemCoreClock = 48000000;

#if STM32_HSE_FREQUENCY
    // enable crystal
    RCC->CR |= RCC_CR_HSEON;

    // wait for it to stabilize
    while (!(RCC->CR & RCC_CR_HSERDY));

    // configure PLL
#if STM32_HSE_FREQUENCY == 8000000
    MODMASK(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLQ | RCC_PLLCFGR_PLLR,
        RCC_PLLCFGR_PLLSRC_HSE |
        (36 << RCC_PLLCFGR_PLLN_Pos) |            // VCO = 8x36 = 288 MHz
        RCC_PLLCFGR_PLLQ_1 |                      // PLLQ = 288/6 = 48 MHz
        RCC_PLLCFGR_PLLR_0);                      // PLLR = 288/4 = 72 MHz
#else
#error "Unsupported HSE frequency"
#endif

    // enable main PLL output
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLREN;
    // enable PLL
    RCC->CR |= RCC_CR_PLLON;
    // wait for PLL ready
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // select PLL for clock
    RCC->CFGR = RCC_CFGR_SW_PLL;
    SystemCoreClock = 72000000;
#endif

#if TRACE || MINTRACE
    // enable trace pin output, leave the rest to the probe
    DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN;
#endif
}

CORTEX_PREINIT(!!0_sysclk, _stm32_sysclk_init);

#if STM32_USE_LSE

void _stm32_lse_init()
{
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
}

CORTEX_PREINIT(!!1_LSE, _stm32_lse_init);

#endif

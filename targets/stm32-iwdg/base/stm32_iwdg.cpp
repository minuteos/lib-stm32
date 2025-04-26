/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32-iwdg/base/stm32_iwdg.cpp
 */

#include <base/base.h>

static void _stm32_iwdg_init()
{
    IWDG->KR = 0xCCCC;    // enable watchdog
    IWDG->KR = 0x5555;    // register access
    IWDG->PR = 0;         // prescaler = 4, freq = 32k/4 = 8kHz
    IWDG->RLR = 0x7FF;    // reload value = 4095, timeout = ~250 ms
    IWDG->KR = 0xAAAA;    // start watchdog

#if DEBUG
    // freeze IWDG when debugging
    DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_IWDG_STOP;
#endif
}

// watchdog is enabled before anything else
CORTEX_PREINIT(!!!wdginit, _stm32_iwdg_init);

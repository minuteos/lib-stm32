/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32/base/platform.h
 */

#pragma once

#define CORTEX_STARTUP_HARDWARE_INIT _stm32_early_init
#define CORTEX_STARTUP_BEFORE_C_INIT _stm32_clock_init

#include_next <base/platform.h>

extern void _stm32_early_init();
extern void _stm32_clock_init();

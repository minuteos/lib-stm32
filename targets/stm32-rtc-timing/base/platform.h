/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32-rtc-timing/base/platform.h
 */

#pragma once

typedef uint32_t mono_t;

#define MONO_CLOCKS _stm32_rtc_mono()
#define MONO_FREQUENCY 32768
#define MONO_US _stm32_rtc_us()

#define PLATFORM_SLEEP(start, duration)

#include_next <base/platform.h>

extern mono_t _stm32_rtc_mono();
extern mono_t _stm32_rtc_us();

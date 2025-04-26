/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32-iwdg/base/platform.h
 */

 #pragma once

 #define PLATFORM_WATCHDOG_HIT()  (IWDG->KR = 0xAAAA)

 #include_next <base/platform.h>

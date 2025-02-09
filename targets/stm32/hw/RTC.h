/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32/hw/RTC.h
 */

#pragma once

#include <base/base.h>

#undef RTC
#define RTC    CM_PERIPHERAL(_RTC, RTC_BASE)

struct _RTC : RTC_TypeDef
{
    ALWAYS_INLINE void Unlock() { WPR = 0xCA; WPR = 0x53; }
    ALWAYS_INLINE void Lock() { WPR = 0; }
};

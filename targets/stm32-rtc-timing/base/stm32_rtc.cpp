/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32-rtc-timing/base/stm32_rtc.cpp
 */

#include <base/base.h>

OPTIMIZE mono_t _stm32_rtc_mono()
{
    static struct
    {
        uint32_t last;
        mono_t mono;
    } state = {};

    auto val = RTC->SSR;
    auto elapsed = (state.last - val) & MASK(15);
    state.last = val;
    return state.mono += elapsed;
}

OPTIMIZE mono_t _stm32_rtc_us()
{
    static struct
    {
        mono_t last;
        uint64_t t64;
    } state = {};

    auto mono = _stm32_rtc_mono();
    auto elapsed = (mono - state.last);
    state.last = mono;
    return ((state.t64 += elapsed) * ((int64_t)1000000 << 17)) >> 32;
}
/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/PWR.h
 */

#pragma once

#include <base/base.h>

#undef PWR
#define PWR CM_PERIPHERAL(_PWR, PWR_BASE)

struct _PWR : PWR_TypeDef
{
    enum struct WakeReason
    {
        Internal = PWR_SR1_WUFI,
        Gpio = PWR_SR1_WUF,
        Gpio1 = PWR_SR1_WUF1,
        Gpio2 = PWR_SR1_WUF2,
        Gpio3 = PWR_SR1_WUF3,
        Gpio4 = PWR_SR1_WUF4,
        Gpio5 = PWR_SR1_WUF5,
        Standby = PWR_SR1_SBF,

        _Mask = Internal | Gpio | Standby,
    };

    static enum WakeReason WakeReason() { return s_wakeReason; }

    static void __CaptureWakeReason();

private:
    static enum WakeReason s_wakeReason;
};

DEFINE_FLAG_ENUM(enum _PWR::WakeReason);

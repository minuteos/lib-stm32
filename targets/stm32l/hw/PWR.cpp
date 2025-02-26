/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/PWR.cpp
 */

#include "PWR.h"

enum _PWR::WakeReason _PWR::s_wakeReason;

void _PWR::__CaptureWakeReason()
{
    s_wakeReason = (enum WakeReason)PWR->SR1 & WakeReason::_Mask;
#if !BOOTLOADER
    PWR->SCR = PWR_SR1_WUF | PWR_SR1_SBF;
#endif

#if TRACE
    auto reason = s_wakeReason;
    if (!!reason)
    {
        DBGC("PWR", "Wake reason:");
#define TRACE_CAUSE(name, flag) if (!!(reason & (flag))) { _DBGCHAR(' '); _DBG(name); }
        TRACE_CAUSE("INT", WakeReason::Internal);
        TRACE_CAUSE("GPIO", WakeReason::Gpio);
        TRACE_CAUSE("STBY", WakeReason::Standby);
#undef TRACE_CAUSE
        _DBGCHAR('\n');
    }
#endif
}

CORTEX_PREINIT(!!1_wake, _PWR::__CaptureWakeReason);

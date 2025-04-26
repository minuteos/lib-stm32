/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/RCC.cpp
 */

#include "RCC.h"

enum _RCC::ResetCause _RCC::s_resetCause;

void _RCC::__CaptureResetCause()
{
    s_resetCause = (enum ResetCause)RCC->CSR & ResetCause::_Mask;

#if !BOOTLOADER
    RCC->CSR |= RCC_CSR_RMVF;
#endif

#if TRACE
    auto cause = s_resetCause;
    if (!!cause)
    {
        DBGC("RCC", "Reset cause:");
#define TRACE_CAUSE(name, flag) if (!!(cause & (flag))) { _DBGCHAR(' '); _DBG(name); }
        TRACE_CAUSE("FW", ResetCause::Firewall);
        TRACE_CAUSE("OBL", ResetCause::OptionByteLoad);
        TRACE_CAUSE("RST", ResetCause::Hardware);
        TRACE_CAUSE("BOR", ResetCause::Brownout);
        TRACE_CAUSE("SFT", ResetCause::Software);
        TRACE_CAUSE("IWDG", ResetCause::IWatchdog);
        TRACE_CAUSE("WWDG", ResetCause::WWatchdog);
        TRACE_CAUSE("LPWR", ResetCause::LowPower);
#undef TRACE_CAUSE
        _DBGCHAR('\n');
    }
#endif
}

CORTEX_PREINIT(!!1_reset, _RCC::__CaptureResetCause);

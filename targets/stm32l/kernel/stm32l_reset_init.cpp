/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/kernel/stm32l_reset_init.cpp
 */

#include <kernel/kernel.h>

#include <hw/RCC.h>
#include <hw/PWR.h>

#if !HAS_BOOTLOADER

void _stm32l_kernel_reset_reason()
{
    auto rc = RCC->ResetCause();

    if (!!(rc & decltype(rc)::Watchdog))
    {
        kernel::resetCause = kernel::ResetCause::Watchdog;
    }
    else if (!!(rc & decltype(rc)::Brownout))
    {
        kernel::resetCause = kernel::ResetCause::Brownout;
    }
    else if (!!(rc & decltype(rc)::Hardware))
    {
        kernel::resetCause = kernel::ResetCause::Hardware;
    }
    else if (!!(rc & decltype(rc)::Software))
    {
        kernel::resetCause = kernel::ResetCause::Software;
    }
    else if (!!rc)
    {
        kernel::resetCause = kernel::ResetCause::MCU;
    }
    else if (!!PWR->WakeReason())
    {
        kernel::resetCause = kernel::ResetCause::Hibernation;
    }
    else
    {
        kernel::resetCause = kernel::ResetCause::PowerOn;
    }
}

CORTEX_PREINIT(!!1x_kernel_reset, _stm32l_kernel_reset_reason);

#endif
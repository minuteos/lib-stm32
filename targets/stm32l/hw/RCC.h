/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/RCC.h
 */

#pragma once

#include <base/base.h>

#undef RCC
#define RCC CM_PERIPHERAL(_RCC, RCC_BASE)

struct _RCC : RCC_TypeDef
{
    enum struct ResetCause : uint32_t
    {
        Firewall = RCC_CSR_FWRSTF,
        OptionByteLoad = RCC_CSR_OBLRSTF,
        Hardware = RCC_CSR_PINRSTF,
        Brownout = RCC_CSR_BORRSTF,
        Software = RCC_CSR_SFTRSTF,
        IWatchdog = RCC_CSR_IWDGRSTF,
        WWatchdog = RCC_CSR_WWDGRSTF,
        LowPower = RCC_CSR_LPWRRSTF,

        Watchdog = IWatchdog | WWatchdog,
        _Mask = Firewall | OptionByteLoad | Hardware | Brownout | Software | Watchdog | LowPower,
    };

    void EnableI2C(unsigned index)
    {
        ASSERT(index < 3);
        APB1ENR1 |= RCC_APB1ENR1_I2C1EN << index;
        __DSB();
    }

    void EnableDMA(unsigned index)
    {
        ASSERT(index < 2);
        AHB1ENR |= RCC_AHB1ENR_DMA1EN << index;
        __DSB();
    }

    static enum ResetCause ResetCause() { return s_resetCause; }

    static void __CaptureResetCause();
private:
    static enum ResetCause s_resetCause;
};

DEFINE_FLAG_ENUM(enum _RCC::ResetCause);

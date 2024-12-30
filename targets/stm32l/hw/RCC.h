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
};

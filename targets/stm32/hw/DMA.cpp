/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32/hw/DMA.cpp
 */

#include "DMA.h"

DMAChannel* DMA::ClaimChannel(uint32_t specs)
{
    while (specs)
    {
        ChannelSpec spec = { .spec = uint8_t(specs) };
        auto dma = spec.dma ? DMA2 : DMA1;
        auto n = spec.ch - 1;   // zero-based index, the spec is one-based
        auto& ch = dma->CH[n];
        if (!ch.CCR)
        {
            auto off = n << 2;
            dma->EnableClock();
            MODMASK(dma->CSELR, MASK(4) << off, spec.map << off);
            ch.CCR = DMA_CCR_MEM2MEM;
            return &ch;
        }
        specs >>= 8;
    }

    return NULL;
}

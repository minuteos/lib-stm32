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

class IRQ DMAChannel::IRQ() const
{
    // need a lookup table as the numbers aren't completely contiguous
    return LOOKUP_TABLE(IRQn_Type,
            DMA1_Channel1_IRQn, DMA1_Channel2_IRQn, DMA1_Channel3_IRQn, DMA1_Channel4_IRQn, DMA1_Channel5_IRQn, DMA1_Channel6_IRQn, DMA1_Channel7_IRQn, {},
            DMA2_Channel1_IRQn, DMA2_Channel2_IRQn, DMA2_Channel3_IRQn, DMA2_Channel4_IRQn, DMA2_Channel5_IRQn, DMA2_Channel6_IRQn, DMA2_Channel7_IRQn
            )
        [DMA().Index() << 3 | Index()];
}

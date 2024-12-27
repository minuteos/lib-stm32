/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/io/DMATransmitter.h
 *
 * A transmitter strategy using DMA buffer linking.
 *
 * Recommended for most scenarios.
 */

#pragma once

#include <io/Transmitter.h>

#include <hw/DMA.h>

namespace io
{

class DMATransmitter : public Transmitter
{
public:
    DMATransmitter(DMAChannel& dma, volatile void* destination, DMADescriptor::Flags flags = {})
        : dma(dma)
    {
        dma.CPAR = uint32_t(destination);
        dma.CCR = flags | DMADescriptor::M2P | DMADescriptor::IncrementMemory | DMADescriptor::UnitByte;
    }

protected:
    virtual size_t TryAddBlock(Span block);
    virtual const char* GetReadPointer();
    virtual async_once(Wait, const char* current, Timeout timeout = Timeout::Infinite);

private:
    DMAChannel& dma;
};

}
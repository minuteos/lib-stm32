/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/io/DMAReceiver.h
 *
 * A receiver strategy using DMA buffer linking.
 *
 * Data is transferred directly into pipe buffers, saving some overhead.
 * Suitable for all but high transfer speeds - there is a short time when
 * buffer linking takes place and data can be lost before DMA is ready again.
 */

#pragma once

#include <io/io.h>

#include <hw/DMA.h>

namespace io
{

class DMAReceiver : public Receiver
{
public:
    DMAReceiver(DMAChannel& dma, const volatile void* source, DMADescriptor::Flags flags = {})
        : dma(dma)
    {
        dma.CPAR = uint32_t(source);
        dma.CCR = flags | DMADescriptor::P2M | DMADescriptor::IncrementMemory | DMADescriptor::UnitByte;
    }

protected:
    virtual size_t TryAddBuffer(size_t offset, Buffer buffer);
    virtual const char* GetWritePointer(Buffer buffer);
    virtual async_once(Wait, const char* current, Timeout timeout = Timeout::Infinite);
    virtual async_once(Close);

private:
    DMAChannel& dma;
};

}
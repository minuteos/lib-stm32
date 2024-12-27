/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/io/DMAReceiverCircular.h
 *
 * A receiver strategy using circular DMA buffer.
 *
 * This is the only strategy allowing continuous high-speed transfers without
 * hardware flow control. The main disadvantage is that it requires a dedicated
 * buffer and an extra copying step.
 */

#pragma once

#include <io/io.h>

#include <hw/DMA.h>

namespace io
{

class DMAReceiverCircular : public Receiver
{
public:
    DMAReceiverCircular(DMAChannel& dma, const volatile void* source, Buffer buffer, DMADescriptor::Flags flags = {})
        : dma(dma)
    {
        dma.CPAR = uint32_t(source);
        dma.CMAR = uint32_t(r = buffer.Pointer());
        dma.CNDTR = buffer.Length();
        dma.CCR = flags | DMADescriptor::P2M | DMADescriptor::IncrementMemory | DMADescriptor::UnitByte | DMADescriptor::Circular | DMADescriptor::Start;
        end = buffer.end();
    }

protected:
    virtual size_t TryAddBuffer(size_t offset, Buffer buffer);
    virtual const char* GetWritePointer(Buffer buf);
    virtual async_once(Wait, const char* current, Timeout timeout = Timeout::Infinite);
    virtual async_once(Close);

private:
    DMAChannel& dma;
    char* end;
    char* r;
};

inline DMAReceiverCircular* CreateDMAReceiverCircularWithBuffer(DMAChannel& dma, const volatile void* source, size_t bufferSize, DMADescriptor::Flags flags = {})
{
    char* mem = new char[sizeof(DMAReceiverCircular) + bufferSize];
    Buffer buf(mem + sizeof(DMAReceiverCircular), bufferSize);
    return new(mem) DMAReceiverCircular(dma, source, buf, flags);
}

}

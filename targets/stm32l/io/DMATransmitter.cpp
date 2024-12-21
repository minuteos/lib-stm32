/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/io/DMATransmitter.cpp
 */

#include "DMATransmitter.h"

namespace io
{

size_t DMATransmitter::TryAddBlock(Span block)
{
    return dma.TryLinkBuffer(block);
}

const char* DMATransmitter::GetReadPointer()
{
    return dma.LinkPointer();
}

async_once(DMATransmitter::Wait, const char* current, Timeout timeout)
{
    return async_forward(dma.LinkPointerNot, current, timeout);
}

}
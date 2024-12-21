/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/io/DMAReceiver.cpp
 */

#include "DMAReceiver.h"

namespace io
{

size_t DMAReceiver::TryAddBuffer(size_t offset, Buffer buf)
{
    return dma.TryLinkBuffer(buf);
}

const char* DMAReceiver::GetWritePointer(Buffer buf)
{
    return dma.LinkPointer();
}

async_once(DMAReceiver::Wait, const char* current, Timeout timeout)
{
    return async_forward(dma.LinkPointerNot, current, timeout);
}

async_once(DMAReceiver::Close)
{
    dma.Unlink();
    async_once_return(0);
}

}

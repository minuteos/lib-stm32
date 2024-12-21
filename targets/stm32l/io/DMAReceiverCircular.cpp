/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/io/DMAReceiverCircular.cpp
 */

#include "DMAReceiverCircular.h"

namespace io
{


size_t DMAReceiverCircular::TryAddBuffer(size_t offset, Buffer buf)
{
    // accept the buffer if it's the first one - there is no point
    // pre-allocating more than one buffer
    return offset ? 0 : buf.Length();
}

const char* DMAReceiverCircular::GetWritePointer(Buffer buf)
{
    // this is actually the point where we load the destination buffer with received data
    auto end = this->end, r = this->r;
    char* w = end - dma.CNDTR;  // next write position

    if (w == r || buf.Length() == 0)
    {
        // no new data or no buffer space
        return buf.Pointer();
    }

    if (w < r)
    {
        // write index is before read index, so the part to the end of the buffer is filled
        if (size_t block = std::min(size_t(end - r), buf.Length()))
        {
            memcpy(buf.Pointer(), r, block);
            buf = buf.RemoveLeft(block);
            if ((r += block) == end)
            {
                r = (char*)dma.CMAR;
            }
        }
    }
    if (r < w)
    {
        if (size_t block = std::min(size_t(w - r), buf.Length()))
        {
            memcpy(buf.Pointer(), r, block);
            buf = buf.RemoveLeft(block);
            r += block;
            ASSERT(r < end);    // cannot happen
        }
    }

    this->r = r;
    return buf.Pointer();
}

async_once(DMAReceiverCircular::Wait, const char* current, Timeout timeout)
{
    return async_forward(WaitMaskNot, dma.CNDTR, ~0u, end - r, timeout);
}

async_once(DMAReceiverCircular::Close)
{
    async_once_return(0);
}

}
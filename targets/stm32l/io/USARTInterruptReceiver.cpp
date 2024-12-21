/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/io/USARTInterruptReceiver.cpp
 */

#include "USARTInterruptReceiver.h"

namespace io
{

size_t USARTInterruptReceiver::TryAddBuffer(size_t offset, Buffer buf)
{
    return handler.AddRxBuffer(buf) * buf.Length();
}

const char* USARTInterruptReceiver::GetWritePointer(Buffer buf)
{
    return handler.RxPointer();
}

async_once(USARTInterruptReceiver::Wait, const char* current, Timeout timeout)
{
    return async_forward(WaitMaskNot, handler.RxPointer(), ~0u, current, timeout);
}

async_once(USARTInterruptReceiver::Close)
{
    handler.ResetRx();
    async_once_return(0);
}

}

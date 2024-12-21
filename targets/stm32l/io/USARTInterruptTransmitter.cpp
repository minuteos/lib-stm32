/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/io/USARTInterruptTransmitter.cpp
 */

#include "USARTInterruptTransmitter.h"

namespace io
{

size_t USARTInterruptTransmitter::TryAddBlock(Span block)
{
    return handler.AddTxBuffer(block) ? block.Length() : 0;
}

const char* USARTInterruptTransmitter::GetReadPointer()
{
    return handler.TxPointer();
}

async_once(USARTInterruptTransmitter::Wait, const char* current, Timeout timeout)
{
    return async_forward(WaitMaskNot, handler.TxPointer(), ~0u, current, timeout);
}

}
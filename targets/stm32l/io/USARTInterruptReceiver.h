/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/io/USARTInterruptReceiver.h
 *
 * An USART receiver strategy using interrupts.
 *
 * This should be used only as a fallback when DMA is not available, due
 * to the additional per-byte interrupt overhead.
 */

#pragma once

#include <io/Receiver.h>

#include "USARTInterrupt.h"

namespace io
{

class USARTInterruptReceiver : public Receiver
{
public:
    USARTInterruptReceiver(USARTInterrupt& handler)
        : handler(handler) {}

protected:
    virtual size_t TryAddBuffer(size_t offset, Buffer buffer);
    virtual const char* GetWritePointer(Buffer buffer);
    virtual async_once(Wait, const char* current, Timeout timeout = Timeout::Infinite);
    virtual async_once(Close);

private:
    USARTInterrupt& handler;
};

}

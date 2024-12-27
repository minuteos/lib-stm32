/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/io/USARTInterruptTransmitter.h
 *
 * An USART transmitter strategy using interrupts.
 *
 * This should be used only as a fallback when DMA is not available, due
 * to the additional per-byte interrupt overhead.
 */

#pragma once

#include <io/Transmitter.h>

#include "USARTInterrupt.h"

namespace io
{

class USARTInterruptTransmitter : public Transmitter
{
public:
    USARTInterruptTransmitter(USARTInterrupt& handler)
        : handler(handler) {}

protected:
    virtual size_t TryAddBlock(Span block);
    virtual const char* GetReadPointer();
    virtual async_once(Wait, const char* current, Timeout timeout = Timeout::Infinite);

private:
    USARTInterrupt& handler;
};

}

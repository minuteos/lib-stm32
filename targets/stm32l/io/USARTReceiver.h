/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/io/USARTReceiver.h
 */

#pragma once

#include <io/io.h>

#include <hw/USART.h>

namespace io
{

class USARTReceiver
{
public:
    USARTReceiver(USART* usart, Receiver* receiver)
        : usart(usart), receiver(receiver) {}

    USARTReceiver(USART* usart)
        : usart(usart), receiver(new USARTInterruptReceiver(USARTInterrupt::Get(usart))) {}
    template<unsigned n> USARTReceiver(_USART<n>* usart)
        : usart(usart), receiver(new DMAReceiver(*usart->DmaRx(), &usart->RDR, DMADescriptor::PrioVeryHigh)) {}
    template<unsigned n> USARTReceiver(_USART<n>* usart, size_t bufferSize)
        : usart(usart), receiver(CreateDMAReceiverCircularWithBuffer(*usart->DmaRx(), &usart->RDR, bufferSize, DMADescriptor::PrioVeryHigh)) {}

    ~USARTReceiver() { delete receiver; }

    operator PipeReader() { return pipe; }

    void Start(size_t blockHint = 1)
    {
        receiver->StartReceiveToPipe(pipe, blockHint);
    }

private:
    Pipe pipe;
    USART* usart;
    Receiver* receiver;
};

}
/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/io/USARTDuplexPipe.h
 */

#pragma once

#include <io/io.h>

namespace io
{

class USARTDuplexPipe
{
public:
    USARTDuplexPipe(USART* usart)
        : receiver(usart), transmitter(usart) {}
    template<unsigned n> USARTDuplexPipe(_USART<n>* usart)
        : receiver(usart), transmitter(usart) {}
    template<unsigned n> USARTDuplexPipe(_USART<n>* usart, size_t bufferSize)
        : receiver(usart, bufferSize), transmitter(usart) {}

    operator DuplexPipe() { return { receiver, transmitter }; }

    void Start()
    {
        receiver.Start();
        transmitter.Start();
    }

private:
    USARTReceiver receiver;
    USARTTransmitter transmitter;
};

}
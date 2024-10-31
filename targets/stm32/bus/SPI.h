/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32/bus/SPI.h
 */

#pragma once

#include <base/base.h>

#include <hw/GPIO.h>

namespace bus
{

class SPI
{
public:
    //! Opaque handle representing the ChipSelect signal
    class ChipSelect {};

    //! SPI transfer descriptor
    struct Descriptor
    {
        void Transmit(Span d);
        void Receive(Buffer d);
    };

    //! Gets the maximum number of bytes that can be transferred using a single transfer descriptor
    constexpr static size_t MaximumTransferSize() { return 2 << 16; }

    //! Retrieves a ChipSelect handle for the specified GPIO pin
    ChipSelect GetChipSelect(GPIOPin pin);
    //! Acquires the bus for the device identified by the specified @ref ChipSelect
    async(Acquire, ChipSelect cs, Timeout timeout = Timeout::Infinite);
    //! Releases the bus
    void Release();

    //! Performs a single SPI transfer
    async(Transfer, Descriptor& descriptor);
    //! Performs a chain of SPI transfers
    async(Transfer, Descriptor* descriptors, size_t count);
    //! Performs a chain of SPI transfers
    template<size_t n> async(Transfer, Descriptor (&descriptors)[n]);
};

}

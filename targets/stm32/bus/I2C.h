/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32/bus/SPI.h
 */

#pragma once

#include <base/base.h>
#include <base/Span.h>

#include <hw/I2C.h>

namespace bus
{

class I2C
{
    ::I2C& i2c;

public:
    constexpr I2C(::I2C& i2c)
        : i2c(i2c) {}
    constexpr I2C(::I2C* i2c)
        : i2c(*i2c) {}

    //! Hints at the next operation to be expected on the device
    using Next = ::I2C::Next;
    //! Control structure for a slave device on the bus
    using Device = ::I2C::Device;

    //! Creates a Master control structure for the specified device
    Device Master(uint8_t address) { return i2c.Master(address); }

    //! Gets the current bus frequency
    uint32_t OutputFrequency() const { return i2c.OutputFrequency(); }
    //! Sets the current bus frequency
    void OutputFrequency(uint32_t frequency) const { i2c.OutputFrequency(frequency); }
};

}

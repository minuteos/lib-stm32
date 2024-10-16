/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/I2C.h
 */

#pragma once

#include <kernel/kernel.h>

#include <base/Span.h>
#include <hw/IRQ.h>
#include <hw/GPIO.h>
#include <hw/RCC.h>

#undef I2C1
#define I2C1    CM_PERIPHERAL(_I2C<1>, I2C1_BASE)
#undef I2C2
#define I2C2    CM_PERIPHERAL(_I2C<2>, I2C2_BASE)
#undef I2C3
#define I2C3    CM_PERIPHERAL(_I2C<3>, I2C3_BASE)

struct I2C : I2C_TypeDef
{
    //! Gets the zero-based index of the peripheral
    unsigned Index() const { return ((unsigned)this - I2C1_BASE) / (I2C2_BASE - I2C1_BASE); }

    //! Enables the peripheral
    void Enable() { CR1 |= I2C_CR1_PE; }
    //! Disables the peripheral
    void Disable() { CR1 &= ~I2C_CR1_PE; }
    //! Checks whether the peripheral is enabled
    bool IsEnabled() const { return CR1 & I2C_CR1_PE; }
    
    //! Sends one byte of data
    void Send(uint8_t data) { TXDR = data; }
    //! Receives one byte of data
    uint8_t Receive() { return RXDR; }

    //! Gets the BUSY status of the bus
    bool IsBusy() const { return ISR & I2C_ISR_BUSY; }


    //! Enables peripheral clock
    void EnableClock() { RCC->EnableI2C(Index()); }
    IRQ ErrorIRQ() const { return LOOKUP_TABLE(IRQn_Type, I2C1_ER_IRQn, I2C2_ER_IRQn, I2C3_ER_IRQn)[Index()]; }
    IRQ EventIRQ() const { return LOOKUP_TABLE(IRQn_Type, I2C1_EV_IRQn, I2C2_EV_IRQn, I2C3_EV_IRQn)[Index()]; }
    
    void ConfigureScl(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::OpenDrain | GPIOPin::SpeedVeryHigh)
        { pin.ConfigureAlternate(afScl[Index()], mode); }
    void ConfigureSda(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::OpenDrain | GPIOPin::SpeedVeryHigh)
        { pin.ConfigureAlternate(afSda[Index()], mode); }

    //! Sets the I2C clock frequency
    void OutputFrequency(unsigned freq);
    //! Gets the current I2C clock frequency
    unsigned OutputFrequency() const;

    //! Resets the bus
    async(Reset);

    //! Starts or restarts a read transaction, reading the specified number of bytes from the bus
    async(Read, uint8_t address, Buffer data, bool start, bool stop) { return async_forward(_Read, Operation(address, true, true, stop, data.Length()), data.Pointer()); }
    //! Continues a read transaction, reading the specified number of bytes from the bus
    async(Read, Buffer data, bool stop) { return async_forward(_Read, Operation(true, stop, data.Length()), data.Pointer()); }

    //! Starts or restarts a write transaction, writing the specified number of bytes to the bus
    async(Write, uint8_t address, Span data, bool start, bool stop) { return async_forward(_Write, Operation(address, false, true, stop, data.Length()), data.Pointer()); }
    //! Continues a write transaction, writing the specified number of bytes to the bus
    async(Write, Span data, bool stop) { return async_forward(_Write, Operation(false, stop, data.Length()), data.Pointer()); }

private:
    static const GPIOPinTables_t afScl;
    static const GPIOPinTables_t afSda;

    union Operation
    {
        constexpr Operation(uint8_t address, bool read, bool start, bool stop, uint8_t length)
            : value(address << 1 | read * I2C_CR2_RD_WRN | start * I2C_CR2_START | stop * I2C_CR2_AUTOEND | (length << I2C_CR2_NBYTES_Pos)) {}
        constexpr Operation(bool read, bool stop, uint16_t length)
            : value(read * I2C_CR2_RD_WRN | I2C_CR2_START | stop * I2C_CR2_AUTOEND | (length << I2C_CR2_NBYTES_Pos)) {}

        uint32_t value;
        struct
        {
            union
            {
                struct
                {
                    uint16_t : 1;
                    uint16_t addr7 : 7;
                    uint16_t : 2;
                    bool read : 1;
                    bool send10 : 1;
                    bool head10 : 1;
                    bool start : 1;
                    bool stop : 1;
                    bool nack : 1;
                };
                uint16_t addr10 : 10;
            };
            uint8_t length;
            bool reload : 1;
            bool autoEnd : 1;
            bool pecByte : 1;
        };
    };

    async(_Read, Operation op, char* data);
    async(_Write, Operation op, const char* data);
};

template<unsigned n> struct _I2C : I2C
{
    static const GPIOPinTable_t afScl, afSda;

    //! Gets the zero-based index of the peripheral
    constexpr unsigned Index() const { return n - 1; }

    void ConfigureScl(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::SpeedMedium)
        { pin.ConfigureAlternate(afScl, mode | GPIOPin::OpenDrain | GPIOPin::FlagSet); }
    void ConfigureSda(GPIOPin pin, GPIOPin::Mode mode = GPIOPin::SpeedMedium)
        { pin.ConfigureAlternate(afSda, mode | GPIOPin::OpenDrain | GPIOPin::FlagSet); }
};


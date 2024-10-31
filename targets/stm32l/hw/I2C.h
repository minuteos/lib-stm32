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
    unsigned Index() const { return (((unsigned)this >> 10) & 3) - 1; }

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

    //! Next action after this operation
    enum struct Next
    {
        //! Send an I2C stop condition
        Stop,
        //! Continue the same operation
        Continue,
        //! Restart a different operation (but keep holding the bus)
        Restart
    };

    //! Device control structure
    class Device
    {
        I2C& i2c;           //< I2C peripheral reference
        uint8_t address;    //< Device address
        bool active : 1;    //< The device is currently active
        bool reloaded : 1;  //< Previous transfer had the RELOAD flag set
        uint16_t wx;        //< Number of bytes transferred in the last operation

    public:
        // prevent copying the instance as it is used to track state
        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        constexpr Device(I2C& i2c, uint8_t address)
            : i2c(i2c), address(address), active{}, reloaded{}, wx{} {}

        I2C& Bus() const { return i2c; }
        uint8_t Address() const { return address; }
        unsigned Transferred() const { return wx; }

        async(Read, Buffer data, Next next = Next::Stop);
        async(Write, Span data, Next next = Next::Stop);

    private:
        unsigned Index() const { return i2c.Index(); }

        //! Acquires the bus for communication with the specified device
        async(Acquire);
        //! Releases the bus
        void Release();
    };

    constexpr Device Master(uint8_t address) { return Device(*this, address); }

private:
    static const GPIOPinTables_t afScl;
    static const GPIOPinTables_t afSda;
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


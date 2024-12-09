/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/GPIO.h
 */

#pragma once

#include <base/base.h>

#ifdef Ckernel
#include <kernel/kernel.h>
#endif

#define GPIO_P(n) CM_PERIPHERAL(GPIOPort, GPIOA_BASE + (GPIOB_BASE - GPIOA_BASE) * (n))

class GPIO;
class GPIOPort;
class GPIOPinID;

//! Compact identification of a GPIO pin
class GPIOPinID
{
    union
    {
        uint16_t id;
        uint8_t pinId;
        struct
        {
            uint8_t port : 4;
            uint8_t index : 4;
            uint8_t afn : 4;
        };
    };

public:
    //! Creates a GPIOPinID from the provided port (0-14), pin index (0-15) and alt function (0-15)
    constexpr GPIOPinID(int port, int index, int afn = 0) : port(port + 1), index(index), afn(afn) {}
    //! Creates a GPIOPinID from the provided compact representation
    constexpr GPIOPinID(uint16_t id) : id(id) {}

    //! Checks if this is a valid GPIO pin
    constexpr bool IsValid() const { return !!id; }
    //! Gets the port index (0-14, 0=A)
    constexpr int Port() const { return port - 1; }
    //! Gets the pin index (0-15)
    constexpr int Pin() const { return index; }
    //! Gets the alt function index (0-15)
    constexpr int Alt() const { return afn; }

    //! Gets the numeric representation of the Pin ID
    constexpr operator uint16_t() const { return id; }

    friend class GPIOPin;
};

//! Alternate function lookup table type
typedef const GPIOPinID GPIOPinTable_t[];
typedef const GPIOPinID* GPIOPinTables_t[];
//! Alternate function lookup table definition
#define GPIO_PINS(...)    {__VA_ARGS__, GPIOPinID(0) }

//! Representation of a GPIO pin
class GPIOPin
{
    GPIOPort* port;
    uint32_t mask;

    enum {
        ModeOffset = 0,
        SpeedOffset = 4,
        PullOffset = 6,
        FlagsOffset = 8,
        AltOffset = 12,
    };

public:
    //! Creates a new GPIOPin instance from the specified @p port and @p mask
    /*!
     * @remark note that @p mask is not the port index, but a bitmask to be
     * used over port registers. Use the @link PA P<port>(<pin>) @endlink macros to obtain GPIOPin
     * instances.
     */
    constexpr GPIOPin(GPIOPort* port, uint32_t mask) : port(port), mask(mask) {}

    //! Gets the GPIOPort to which the GPIOPin belongs
    constexpr GPIOPort& Port() const { return *port; }
    //! Gets the bitmask of the GPIOPin
    constexpr const uint32_t& Mask() const { return mask; }
    //! Gets the index of the GPIOPin
    /*! If there are multiple bits set in the Mask, the index of the lowest one is returned */
    constexpr uint32_t Index() const { return __builtin_ctz(mask); }
    //! Gets the number of consecutive pins of a multi-pin GPIOPin
    constexpr uint32_t Size() const { return 32 - __builtin_ctz(mask) - __builtin_clz(mask); }

    //! Gets a compact location representation GPIOPinID of the GPIOPin
    /*! Use the @link pA p<port>(<pin>) @endlink macros to get GPIOPinID instances when possible */
    constexpr GPIOPinID GetID() const;
    //! Checks if this is a valid GPIO pin
    constexpr bool IsValid() const { return !!mask; }
    //! Gets the pin name
    /*! @warning This function is meant mostly for diagnostics and is not reentrant */
    const char* Name() const;

    //! Specifies the drive mode of the GPIOPin
    enum Mode
    {
        //! The pin is not driven in any way (still can be used as an analog input)
        Disabled = 0b11,
        Analog = 0b11,
        //! The pin is configured as a digital input
        Input = 0b00,
        //! The pin is configured as a digital output
        Output = 0b01,
        //! Alternate mode
        Alternate = 0b10,

        //! The pin is configured with only low-side driver enabled when output is low; an external pull-up is expected
        OpenDrain = 0b1 << 2,

        //! Pin is used to drive low-speed peripherals
        SpeedLow = 0b00 << 4,
        //! Pin is used to drive medium-speed peripherals
        SpeedMedium = 0b01 << 4,
        //! Pin is used to drive medium-speed peripherals
        SpeedHigh = 0b10 << 4,
        //! Pin is used to drive medium-speed peripherals
        SpeedVeryHigh = 0b11 << 4,

        //! Enable the internal pull-up
        PullUp = 0b01 << 6,
        //! Enable the internal pull-down
        PullDown = 0b10 << 6,

        //! The pin is set high before enabling output
        FlagSet = 1 << 8
    };

    //! Configures the GPIOPin as a digital input
    void ConfigureDigitalInput() const { Configure(Input); }
    //! Configures the GPIOPin as a digital input with internal pull-down (@p pull == @c false) or pull-up (@p pull == @c true) enabled
    void ConfigureDigitalInput(bool pull) const { Configure(Mode(Input | (pull ? PullUp : PullDown))); }
    //! Configures the GPIOPin as a digital output
    void ConfigureDigitalOutput(bool set = false) const { Configure(Mode(Output | (set * FlagSet))); }
    //! Configures the GPIOPin as an open drain output
    void ConfigureOpenDrain(bool set = true) const { Configure(Mode(Output | OpenDrain | (set * FlagSet))); }
    //! Configures the GPIOPin as an analog input
    void ConfigureAnalog() const { Configure(Mode(Analog)); }
    //! Configures the GPIOPin for an alternate function
    void ConfigureAlternate(GPIOPinTable_t table, Mode mode = Mode::Alternate) const;
    //! Disables the GPIOPin
    void Disable() const { Configure(Disabled); }

    //! Configures the GPIOPin with the specified drive @ref Mode
    void Configure(Mode mode) const;

    //! Enables edge interrupt generation
    /*! @returns the mask to the interrupt registers allocated for the pin */
    uint32_t EnableInterrupt(bool rising, bool falling) const;

    //! Gets the input state of the GPIOPin
    bool Get() const;
    //! Sets the output of the GPIOPin to a logical 1
    void Set() const;
    //! Reset the output of the GPIOPin to a logical 0
    void Res() const;
    //! Sets the output of the GPIOPin to the desired @p state
    void Set(bool state) const;
    //! Reads the input state of the GPIOPin range
    uint32_t Read() const;
    //! Sets the output of the GPIOPin range to the desired @p value
    void Write(uint32_t value) const;
    //! Toggles the output of the GPIOPin
    void Toggle() const;
#ifdef Ckernel
    //! Waits for the pin to have the specified state
    async_once(WaitFor, bool state, Timeout timeout = Timeout::Infinite);
#endif

    //! Gets the input state of the GPIOPin
    operator bool() const { return Get(); }

    //! Checks if two GPIOPin instnaces refer to the same pin
    ALWAYS_INLINE bool operator ==(const GPIOPin& other) const { return &port == &other.port && mask == other.mask; }
    //! Checks if two GPIOPin instnaces refer to different pins
    ALWAYS_INLINE bool operator !=(const GPIOPin& other) const { return &port != &other.port || mask != other.mask; }

    friend class GPIOPort;
};

//! Representation of a GPIO port (group of pins)
class GPIOPort : public GPIO_TypeDef
{
public:
#if GPIOA_BASE
    //! Gets a GPIOPin instance for the specified pin on port A
    static constexpr GPIOPin A(int pin) { return GPIOPin(GPIO_P(0), BIT(pin)); }
    //! Gets a GPIOPin instance for the specified pin on port A
    static constexpr GPIOPin A(int pin, int count) { return GPIOPin(GPIO_P(0), MASK(count) << pin); }
    //! Gets a GPIOPin instance for the specified pin on port A
    #define PA         GPIOPort::A
    //! Gets a GPIOPinID for the specified pin on port A
    #define pA(...)    GPIOPinID(0, __VA_ARGS__)
#endif

#if GPIOB_BASE
    //! Gets a GPIOPin instance for the specified pin on port B
    static constexpr GPIOPin B(int pin) { return GPIOPin(GPIO_P(1), BIT(pin)); }
    //! Gets a GPIOPin instance for the specified pin on port B
    static constexpr GPIOPin B(int pin, int count) { return GPIOPin(GPIO_P(1), MASK(count) << pin); }
    //! Gets a GPIOPin instance for the specified pin on port B
    #define PB         GPIOPort::B
    //! Gets a GPIOPinID for the specified pin on port B
    #define pB(...)    GPIOPinID(1, __VA_ARGS__)
#endif

#if GPIOC_BASE
    //! Gets a GPIOPin instance for the specified pin on port C
    static constexpr GPIOPin C(int pin) { return GPIOPin(GPIO_P(2), BIT(pin)); }
    //! Gets a GPIOPin instance for the specified pin on port C
    static constexpr GPIOPin C(int pin, int count) { return GPIOPin(GPIO_P(2), MASK(count) << pin); }
    //! Gets a GPIOPin instance for the specified pin on port C
    #define PC         GPIOPort::C
    //! Gets a GPIOPinID for the specified pin on port C
    #define pC(...)    GPIOPinID(2, __VA_ARGS__)
#endif

#if GPIOD_BASE
    //! Gets a GPIOPin instance for the specified pin on port D
    static constexpr GPIOPin D(int pin) { return GPIOPin(GPIO_P(3), BIT(pin)); }
    //! Gets a GPIOPin instance for the specified pin on port D
    static constexpr GPIOPin D(int pin, int count) { return GPIOPin(GPIO_P(3), MASK(count) << pin); }
    //! Gets a GPIOPin instance for the specified pin on port D
    #define PD         GPIOPort::D
    //! Gets a GPIOPinID for the specified pin on port D
    #define pD(...)    GPIOPinID(3, __VA_ARGS__)
#endif

#if GPIOE_BASE
    //! Gets a GPIOPin instance for the specified pin on port E
    static constexpr GPIOPin E(int pin) { return GPIOPin(GPIO_P(4), BIT(pin)); }
    //! Gets a GPIOPin instance for the specified pin on port E
    static constexpr GPIOPin E(int pin, int count) { return GPIOPin(GPIO_P(4), MASK(count) << pin); }
    //! Gets a GPIOPin instance for the specified pin on port E
    #define PE         GPIOPort::E
    //! Gets a GPIOPinID for the specified pin on port E
    #define pE(...)    GPIOPinID(4, __VA_ARGS__)
#endif

#if GPIOF_BASE
    //! Gets a GPIOPin instance for the specified pin on port F
    static constexpr GPIOPin F(int pin) { return GPIOPin(GPIO_P(5), BIT(pin)); }
    //! Gets a GPIOPin instance for the specified pin on port F
    static constexpr GPIOPin F(int pin, int count) { return GPIOPin(GPIO_P(5), MASK(count) << pin); }
    //! Gets a GPIOPin instance for the specified pin on port F
    #define PF         GPIOPort::F
    //! Gets a GPIOPinID for the specified pin on port F
    #define pF(...)    GPIOPinID(5, __VA_ARGS__)
#endif

#if GPIOG_BASE
    //! Gets a GPIOPin instance for the specified pin on port G
    static constexpr GPIOPin G(int pin) { return GPIOPin(GPIO_P(6), BIT(pin)); }
    //! Gets a GPIOPin instance for the specified pin on port G
    static constexpr GPIOPin G(int pin, int count) { return GPIOPin(GPIO_P(6), MASK(count) << pin); }
    //! Gets a GPIOPin instance for the specified pin on port G
    #define PG         GPIOPort::G
    //! Gets a GPIOPinID for the specified pin on port G
    #define pG(...)    GPIOPinID(6, __VA_ARGS__)
#endif

#if GPIOH_BASE
    //! Gets a GPIOPin instance for the specified pin on port H
    static constexpr GPIOPin H(int pin) { return GPIOPin(GPIO_P(7), BIT(pin)); }
    //! Gets a GPIOPin instance for the specified pin range on port H
    static constexpr GPIOPin H(int pin, int count) { return GPIOPin(GPIO_P(7), MASK(count) << pin); }
    //! Gets a GPIOPin instance for the specified pin on port H
    #define PH         GPIOPort::H
    //! Gets a GPIOPinID for the specified pin on port H
    #define pH(...)    GPIOPinID(7, __VA_ARGS__)
#endif

    //! Gets a GPIOPin instance for the specified pin on the GPIOPort
    GPIOPin Pin(int number) { return GPIOPin(this, BIT(number)); }
    //! Gets the zero-based index of the GPIOPort
    int Index() const { return GetIndex(this); }

    static constexpr int GetIndex(const GPIOPort* port) { return (((uintptr_t)port) - GPIOA_BASE) / (GPIOB_BASE - GPIOA_BASE); }

    volatile uint32_t* BitSetPtr() { return &BSRR; }
    volatile uint32_t* BitClearPtr() { return &BRR; }
    const volatile uint32_t* InputPtr() { return &IDR; }
    volatile uint32_t* OutputPtr() { return &ODR; }

    //! Suppress trace messages when reconfiguring pins
    static void SuppressConfigTrace() { _Trace(1); }
    //! Restore trace messages when reconfiguring pins
    static void RestoreConfigTrace() { _Trace(-1); }

private:
    struct AltSpec
    {
        constexpr AltSpec(unsigned pin, GPIOPin::Mode mode, uint8_t route, uint8_t loc)
            : spec(pin | mode << 8 | route << 16 | loc << 24) {}

        union
        {
            uint32_t spec;
            struct
            {
                uint8_t pin;
                GPIOPin::Mode mode;
                uint8_t route, loc;
            };
        };
    };

    void Configure(uint32_t mask, enum GPIOPin::Mode mode);
#ifdef Ckernel
    async_once(WaitFor, uint32_t indexAndState, Timeout timeout = Timeout::Infinite);
#endif

#if TRACE
    static bool _Trace(int n = 0) { static int noTrace = 0; return !(noTrace += n); }
#else
    static constexpr bool _Trace(int n = 0) { return false; }
#endif

    friend class GPIOPin;
};

DEFINE_FLAG_ENUM(enum GPIOPin::Mode);

class GPIOPin;

//! Represents an invalid/unused GPIO pin - it still gets a valid port, but no mask
#define Px       GPIOPin(GPIO_P(0), 0)

ALWAYS_INLINE constexpr GPIOPinID GPIOPin::GetID() const { return GPIOPinID(GPIOPort::GetIndex(port), Index()); }

ALWAYS_INLINE void GPIOPin::Configure(Mode mode) const { port->Configure(mask, mode); }
#ifdef Ckernel
ALWAYS_INLINE async_once(GPIOPin::WaitFor, bool state, Timeout timeout) { return async_forward(Port().WaitFor, (state << 4) | Index(), timeout); }
#endif

ALWAYS_INLINE void GPIOPin::Set() const { port->BSRR = mask; }
ALWAYS_INLINE void GPIOPin::Res() const { port->BRR = mask; }
ALWAYS_INLINE void GPIOPin::Toggle() const
{
    const uint32_t s = port->ODR;
    port->BSRR = (~s & mask) | ((s & mask) << 16);
}
ALWAYS_INLINE void GPIOPin::Set(bool state) const { state ? Set() : Res(); }
ALWAYS_INLINE uint32_t GPIOPin::Read() const
{
    return (port->IDR & mask) >> __builtin_ctz(mask);
}
ALWAYS_INLINE void GPIOPin::Write(uint32_t value) const
{
    value <<= __builtin_ctz(mask);
    port->BSRR = (value & mask) | ((~value & mask) << 16);
}
ALWAYS_INLINE bool GPIOPin::Get() const { return port->IDR & mask; }

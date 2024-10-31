/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/I2C.cpp
 */

#include "I2C.h"

#include <kernel/kernel.h>

#ifndef I2C_TIMEOUT
#define I2C_TIMEOUT	1000		// timeouts shouldn't normally occur
#endif

#define MYDBG(format, ...)      DBGL("I2C%d: " format, Index() + 1, ## __VA_ARGS__)

const GPIOPinTables_t I2C::afScl = { _I2C<1>::afScl, _I2C<2>::afScl, _I2C<3>::afScl };
const GPIOPinTables_t I2C::afSda = { _I2C<1>::afSda, _I2C<2>::afSda, _I2C<3>::afSda };

static constexpr uint32_t I2C_ISR_ERR = I2C_ISR_ARLO | I2C_ISR_NACKF;
static constexpr uint32_t I2C_ICR_ERR = I2C_ICR_BERRCF | I2C_ICR_ARLOCF | I2C_ICR_NACKCF | I2C_ICR_STOPCF;
static constexpr uint32_t I2C_ISR_OK = I2C_ISR_TC | I2C_ISR_TCR | I2C_ISR_STOPF;

async(I2C::Reset)
async_def_sync(unsigned i)
{
    Disable();
    while (IsEnabled());
    Enable();

    if (!IsBusy())
    {
        async_return(true);
    }

    MYDBG("bus busy after reset, trying to clock it out, ISR: %X", ISR);
    // TODO
}
async_end

static unsigned diff(unsigned a, unsigned b) { return a > b ? a - b : b - a; }

void I2C::OutputFrequency(unsigned freq)
{
    unsigned best = 0, bestPresc;

    // try finding the highest prescaler with the best frequency match
    for (unsigned presc = 16; presc > 0; presc--)
    {
        const unsigned clk = SystemCoreClock / presc;
        const unsigned tbit = clk / freq;
        const unsigned scll = tbit * 7 / 11;
        if (scll > 0x100 || scll < 5)
        {
            // cannot use, too short or too long
            continue;
        }
        unsigned res = clk / tbit;
        if (diff(freq, res) < diff(best, res))
        {
            best = res;
            bestPresc = presc;
            if (res == freq)
            {
                break;
            }
        }
    }

    if (!best)
    {
        DBGL("I2C%d: Cannot calculate timings for output freq %u", Index(), freq);
        ASSERT(false);
        return;
    }

    // TODO: finetuning
    const unsigned clk = SystemCoreClock / bestPresc;
    const unsigned tbit = clk / freq;
    const unsigned scll = tbit * 5 / 11;
    const unsigned sclh = tbit - scll;
    const unsigned sdadel = 2, scldel = 4;

    const uint32_t timingr =
        (bestPresc - 1) << I2C_TIMINGR_PRESC_Pos |
        (sclh - 1) << I2C_TIMINGR_SCLH_Pos |
        (scll - 1) << I2C_TIMINGR_SCLL_Pos |
        (scldel - 1) << I2C_TIMINGR_SCLDEL_Pos |
        (sdadel) << I2C_TIMINGR_SDADEL_Pos;

    DBGL("I2C%d: frequency = %d, PRESC=%d, H/L=%d/%d, SCLDEL=%d, SDADEL=%d, TIMINGR=%08X", Index(), best, bestPresc, sclh, scll, scldel, sdadel, timingr);
    TIMINGR = timingr;
}

static uint32_t s_locks;

async(I2C::Device::Acquire)
async_def()
{
    if (!active)
    {
        await_acquire(s_locks, BIT(Index()));
        active = true;
        reloaded = false;
    }
}
async_end

void I2C::Device::Release()
{
    if (active)
    {
        RESBIT(s_locks, Index());
        active = false;
        reloaded = false;
    }
}

async(I2C::Device::Read, Buffer data, Next next)
async_def(
    volatile uint32_t* reg;
    char* data;
    char* end;
    IRQ ev;

    void ReadHandler()
    {
        auto b = *reg;
        if (data < end)
        {
            *data++ = b;
        }
    }
)
{
    f.reg = &i2c.RXDR;
    f.data = data.begin();
    f.end = data.end();

    await(Acquire);

    f.ev = i2c.EventIRQ();
    f.ev.SetHandler(&f, &__FRAME::ReadHandler);
    f.ev.Enable();

    // clear any previous errors
    i2c.ICR = I2C_ICR_ERR;

    while (f.data < f.end)
    {
        unsigned nbytes = f.end - f.data;
        bool oversize = nbytes > 255;
        if (oversize)
        {
            nbytes = 255;
        }

        bool reload = oversize || next == Next::Continue;
        bool stop = !oversize && next == Next::Stop;

        i2c.CR2 = address << 1 | I2C_CR2_RD_WRN |
            !reloaded * I2C_CR2_START |
            reload * I2C_CR2_RELOAD |
            stop * I2C_CR2_AUTOEND |
            nbytes << I2C_CR2_NBYTES_Pos;

        reloaded = reload;

        // enable interrupt to pull data
        i2c.CR1 |= I2C_CR1_RXIE;

        bool done = await_mask_not_ms(i2c.ISR, I2C_ISR_ERR | I2C_ISR_OK, 0, I2C_TIMEOUT);

        // disable interrupt
        i2c.CR1 &= ~I2C_CR1_RXIE;

        wx = f.data - data.begin();

        if (!done || (i2c.ISR & I2C_ISR_ERR))
        {
            f.ev.ResetHandler();

            MYDBG("%s %X", done ? "ERROR" : "TIMEOUT", i2c.ISR);
            await(i2c.Reset);
            Release();

            async_return(false);
        }
    }

    f.ev.ResetHandler();
    if (next == Next::Stop)
    {
        // done, release the bus
        Release();
    }
    async_return(true);
}
async_end

async(I2C::Device::Write, Span data, Next next)
async_def(
    volatile uint32_t* reg;
    const char* data;
    const char* end;
    IRQ ev;

    void WriteHandler()
    {
        ASSERT(data < end)
        auto b = data < end ? *data++ : 0;
        *reg = b;
    }
)
{
    f.reg = &i2c.TXDR;
    f.data = data.begin();
    f.end = data.end();

    await(Acquire);

    f.ev = i2c.EventIRQ();
    f.ev.SetHandler(&f, &__FRAME::WriteHandler);
    f.ev.Enable();

    // clear any previous errors
    i2c.ICR = I2C_ICR_ERR;

    while (f.data < f.end)
    {
        unsigned nbytes = f.end - f.data;
        bool oversize = nbytes > 255;
        if (oversize)
        {
            nbytes = 255;
        }

        bool reload = oversize || next == Next::Continue;
        bool stop = !oversize && next == Next::Stop;

        i2c.CR2 = address << 1 |
            !reloaded * I2C_CR2_START |
            reload * I2C_CR2_RELOAD |
            stop * I2C_CR2_AUTOEND |
            nbytes << I2C_CR2_NBYTES_Pos;

        reloaded = reload;

        // enable interrupt to push data
        i2c.CR1 |= I2C_CR1_TXIE;

        bool done = await_mask_not_ms(i2c.ISR, I2C_ISR_ERR | I2C_ISR_OK, 0, I2C_TIMEOUT);

        // disable interrupt to push data
        i2c.CR1 &= ~I2C_CR1_TXIE;

        wx = f.data - data.begin();

        if (!done || (i2c.ISR & I2C_ISR_ERR))
        {
            f.ev.ResetHandler();

            MYDBG("%s %X", done ? "ERROR" : "TIMEOUT", i2c.ISR);
            await(i2c.Reset);
            Release();
            async_return(false);
        }
    }

    f.ev.ResetHandler();
    if (next == Next::Stop)
    {
        // done, release the bus
        Release();
    }
    async_return(true);
}
async_end

#pragma region Pin Definitions

template<> const GPIOPinTable_t _I2C<1>::afScl = GPIO_PINS(pB(6, 4), pB(8, 4), pG(14, 4));
template<> const GPIOPinTable_t _I2C<1>::afSda = GPIO_PINS(pB(7, 4), pB(9, 4), pG(13, 4));

template<> const GPIOPinTable_t _I2C<2>::afScl = GPIO_PINS(pB(10, 4), pB(13, 4), pF(1, 4));
template<> const GPIOPinTable_t _I2C<2>::afSda = GPIO_PINS(pB(11, 4), pB(14, 4), pF(0, 4));

template<> const GPIOPinTable_t _I2C<3>::afScl = GPIO_PINS(pC(0, 4), pG(7, 4));
template<> const GPIOPinTable_t _I2C<3>::afSda = GPIO_PINS(pC(1, 4), pG(8, 4));

#pragma endregion

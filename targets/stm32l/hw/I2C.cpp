/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/I2C.cpp
 */

#include "I2C.h"

#ifndef I2C_TIMEOUT
#define I2C_TIMEOUT	1000		// timeouts shouldn't normally occur
#endif

#define DBGERR(error)	DBGL("I2C%d: " error ": %X", Index(), ISR)

const GPIOPinTables_t I2C::afScl = { _I2C<1>::afScl, _I2C<2>::afScl, _I2C<3>::afScl };
const GPIOPinTables_t I2C::afSda = { _I2C<1>::afSda, _I2C<2>::afSda, _I2C<3>::afSda };

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

    DBGERR("bus busy after reset, trying to clock it out");
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
    const unsigned scll = tbit * 7 / 11;
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

async(I2C::_Read, Operation op, char* data)
async_def(uint32_t wx)
{
    CR2 = op.value;
    
    for (f.wx = 0; f.wx < op.length; f.wx++)
    {
        if (!await_mask_not_ms(ISR, I2C_ISR_RXNE, 0, I2C_TIMEOUT))
        {
            DBGERR("timeout waiting for RX data");
            await(Reset);
            break;
        }

        data[f.wx] = Receive();
    }

    async_return(f.wx);
}
async_end

async(I2C::_Write, Operation op, const char* data)
async_def(uint32_t wx)
{
    CR2 = op.value;

    for (f.wx = 0; f.wx < op.length; f.wx++)
    {
        Send(data[f.wx]);
        if (!await_mask_not_ms(ISR, I2C_ISR_TXE, 0, I2C_TIMEOUT))
        {
            DBGERR("timeout waiting for TX data");
            await(Reset);
            break;
        }
    }

    async_return(f.wx);
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

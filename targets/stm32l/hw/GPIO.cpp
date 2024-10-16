/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/GPIO.cpp
 */

#include <hw/GPIO.h>

const char* GPIOPin::Name() const
{
    if (!mask)
        return "Px";

    static char tmp[4];
    int index = Index();
    tmp[0] = 'A' + Port().Index();
    if (index < 10)
    {
        tmp[1] = '0' + index;
        tmp[2] = 0;
    }
    else
    {
        tmp[1] = '0' + index / 10;
        tmp[2] = '0' + index % 10;
        tmp[3] = 0;
    }
    return tmp;
}

void GPIOPin::ConfigureAlternate(GPIOPinTable_t table, GPIOPin::Mode mode) const
{
    auto pin = GetID().pinId;
    for (const auto* p = table; *p; p++)
    {
        if (p->pinId == pin)
        {
            
            Configure(mode | Alternate | Mode(p->afn << AltOffset));
            return;
        }
    }

    DBGC("gpio", "Requested alternate function not found on %s", Name());
    ASSERT(false); 
}

void GPIOPort::Configure(uint32_t mask, GPIOPin::Mode mode)
{
    if (!mask)
        return;

    // enable the corresponding peripheral
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN << Index();

    bool trace = _Trace();

    if (trace)
    {
        DBGC("gpio", "Configuring port %s (", GPIOPin(this, mask).Name());
        _DBG(STRINGS("input", "output", "alt", "analog")[(mode >> GPIOPin::ModeOffset) & MASK(2)]);
        if ((mode & MASK(2)) == GPIOPin::Alternate) { _DBG(" %d", mode >> GPIOPin::AltOffset); }
        if (mode & GPIOPin::OpenDrain) { _DBG(", open"); }
        _DBG(", ");
        _DBG(STRINGS("low", "medium", "high", "ultra")[(mode >> GPIOPin::SpeedOffset) & MASK(2)]);
        _DBG(" speed");
        if ((mode >> GPIOPin::PullOffset) & MASK(2))
        {
            _DBG(", pull-");
            _DBG(mode & GPIOPin::PullUp ? "up" :"down");
        }
    }

    if (mode & GPIOPin::FlagSet)
    {
        BSRR = mask;
        if (trace) _DBG(", set");
    }
    else
    {
        BRR = mask;
    }

    if (trace) _DBG(")\n");

    MODMASK(OTYPER, mask, mode & GPIOPin::OpenDrain ? mask : 0);

    do
    {
        unsigned bit = __builtin_ctz(mask);
        RESBIT(mask, bit);
        unsigned shift = bit * 2;
        unsigned shiftAfr = (bit & 7) * 4;
        MODMASK(AFR[bit > 7], MASK(4) << shiftAfr, ((mode >> GPIOPin::AltOffset) & MASK(4)) << shiftAfr);
        MODMASK(OSPEEDR, MASK(2) << shift, ((mode >> GPIOPin::SpeedOffset) & MASK(2)) << shift);
        MODMASK(PUPDR, MASK(2) << shift, ((mode >> GPIOPin::PullOffset) & MASK(2)) << shift);
        MODMASK(MODER, MASK(2) << shift, ((mode >> GPIOPin::ModeOffset) & MASK(2)) << shift);
    } while(mask);
}

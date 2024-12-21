/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/USART.cpp
 */

#include "USART.h"

#define MYDBG(format, ...)      DBGL("USART%d: " format, Index() + 1, ## __VA_ARGS__)

#pragma region Pin Definitions

template<> const GPIOPinTable_t USART1_t::afClk = GPIO_PINS(pA(8, 7), pB(5, 7), pG(13, 7));
template<> const GPIOPinTable_t USART1_t::afTx  = GPIO_PINS(pA(9, 7), pB(6, 7), pG(9, 7));
template<> const GPIOPinTable_t USART1_t::afRx  = GPIO_PINS(pA(10, 7), pB(7, 7), pG(10, 7));
template<> const GPIOPinTable_t USART1_t::afCts = GPIO_PINS(pA(11, 7), pB(4, 7), pG(11, 7));
template<> const GPIOPinTable_t USART1_t::afRts = GPIO_PINS(pA(12, 7), pB(3, 7), pG(12, 7));

template<> const GPIOPinTable_t USART2_t::afClk = GPIO_PINS(pA(4, 7), pD(7, 7));
template<> const GPIOPinTable_t USART2_t::afTx  = GPIO_PINS(pA(2, 7), pD(5, 7));
template<> const GPIOPinTable_t USART2_t::afRx  = GPIO_PINS(pA(3, 7), pD(6, 7));
template<> const GPIOPinTable_t USART2_t::afCts = GPIO_PINS(pA(0, 7), pD(3, 7));
template<> const GPIOPinTable_t USART2_t::afRts = GPIO_PINS(pA(1, 7), pD(4, 7));

template<> const GPIOPinTable_t USART3_t::afClk = GPIO_PINS(pB(0, 7), pB(12, 7), pC(12, 7), pD(10, 7));
template<> const GPIOPinTable_t USART3_t::afTx  = GPIO_PINS(pB(10, 7), pC(4, 7), pC(10, 7), pD(8, 7));
template<> const GPIOPinTable_t USART3_t::afRx  = GPIO_PINS(pB(11, 7), pC(5, 7), pC(11, 7), pD(9, 7));
template<> const GPIOPinTable_t USART3_t::afCts = GPIO_PINS(pA(6, 7), pB(13, 7), pD(11, 7));
template<> const GPIOPinTable_t USART3_t::afRts = GPIO_PINS(pB(1, 7), pB(14, 7), pD(2, 7), pD(12, 7));

#pragma endregion

static unsigned diff(unsigned a, unsigned b) { return a > b ? a - b : b - a; }

unsigned USART::BaudRate(unsigned baudRate)
{
    uint32_t pclk = SystemCoreClock;    // TODO: other clock sources

    uint32_t clkdiv = (pclk + (baudRate >> 1)) / baudRate;
    uint32_t clkdiv8 = ((pclk << 1) + (baudRate >> 1)) / baudRate;
    uint32_t finalBaud = pclk / clkdiv;
    uint32_t finalBaud8 = (pclk << 1) / clkdiv8;
    bool over8 = false;

    if (diff(finalBaud8, baudRate) < diff(finalBaud, baudRate))
    {
        over8 = true;
        clkdiv = (clkdiv8 & ~0xF) | ((clkdiv8 & 0xF) >> 1);
        finalBaud = finalBaud8;
    }

    MYDBG("Setting baud rate to %d (%.4q%% off %d, %d-bit oversampling)",
        finalBaud,
        int(((float)finalBaud / baudRate - 1) * 10000),
        baudRate,
        over8 ? 8 : 16);
    BRR = clkdiv;
    MODMASK(CR1, USART_CR1_OVER8, USART_CR1_OVER8 * over8);
    return finalBaud;
}

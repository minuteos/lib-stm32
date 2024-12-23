/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/TIM.cpp
 */

#include "TIM.h"

#pragma region Pin Definitions

template<> const GPIOPinTable_t _TIMChannel<1, 1>::afPin = GPIO_PINS(pA(8, 1), pE(9, 1));
template<> const GPIOPinTable_t _TIMChannel<1, 1>::afNeg = GPIO_PINS(pA(7, 1), pB(13, 1), pE(8, 1));
template<> const GPIOPinTable_t _TIMChannel<1, 2>::afPin = GPIO_PINS(pA(9, 1), pE(11, 1));
template<> const GPIOPinTable_t _TIMChannel<1, 2>::afNeg = GPIO_PINS(pB(0, 1), pB(14, 1), pE(10, 1));
template<> const GPIOPinTable_t _TIMChannel<1, 3>::afPin = GPIO_PINS(pA(10, 1), pE(13, 1));
template<> const GPIOPinTable_t _TIMChannel<1, 3>::afNeg = GPIO_PINS(pB(1, 1), pB(15, 1), pE(12, 1));
template<> const GPIOPinTable_t _TIMChannel<1, 4>::afPin = GPIO_PINS(pA(11, 1), pE(14, 1));

template<> const GPIOPinTable_t _TIMChannel<2, 1>::afPin = GPIO_PINS(pA(0, 1), pA(5, 1), pA(15, 1));
template<> const GPIOPinTable_t _TIMChannel<2, 2>::afPin = GPIO_PINS(pA(1, 1), pB(3, 1));
template<> const GPIOPinTable_t _TIMChannel<2, 3>::afPin = GPIO_PINS(pA(2, 1), pB(10, 1));
template<> const GPIOPinTable_t _TIMChannel<2, 4>::afPin = GPIO_PINS(pA(3, 1), pB(11, 1));

template<> const GPIOPinTable_t _TIMChannel<3, 1>::afPin = GPIO_PINS(pA(6, 2), pB(4, 2), pC(6, 2), pE(3, 2));
template<> const GPIOPinTable_t _TIMChannel<3, 2>::afPin = GPIO_PINS(pA(7, 2), pB(5, 2), pC(7, 2), pE(4, 2));
template<> const GPIOPinTable_t _TIMChannel<3, 3>::afPin = GPIO_PINS(pB(0, 2), pC(8, 2), pE(5, 2));
template<> const GPIOPinTable_t _TIMChannel<3, 4>::afPin = GPIO_PINS(pB(1, 2), pC(9, 2), pE(6, 2));

template<> const GPIOPinTable_t _TIMChannel<4, 1>::afPin = GPIO_PINS(pB(6, 2), pD(12, 2));
template<> const GPIOPinTable_t _TIMChannel<4, 2>::afPin = GPIO_PINS(pB(7, 2), pD(13, 2));
template<> const GPIOPinTable_t _TIMChannel<4, 3>::afPin = GPIO_PINS(pB(8, 2), pD(14, 2));
template<> const GPIOPinTable_t _TIMChannel<4, 4>::afPin = GPIO_PINS(pB(9, 2), pD(15, 2));

template<> const GPIOPinTable_t _TIMChannel<5, 1>::afPin = GPIO_PINS(pA(0, 2), pF(6, 2));
template<> const GPIOPinTable_t _TIMChannel<5, 2>::afPin = GPIO_PINS(pA(1, 2), pF(7, 2));
template<> const GPIOPinTable_t _TIMChannel<5, 3>::afPin = GPIO_PINS(pA(2, 2), pF(8, 2));
template<> const GPIOPinTable_t _TIMChannel<5, 4>::afPin = GPIO_PINS(pA(3, 2), pF(9, 2));

template<> const GPIOPinTable_t _TIMChannel<8, 1>::afPin = GPIO_PINS(pC(6, 3));
template<> const GPIOPinTable_t _TIMChannel<8, 1>::afNeg = GPIO_PINS(pA(5, 3), pA(7, 3));
template<> const GPIOPinTable_t _TIMChannel<8, 2>::afPin = GPIO_PINS(pC(7, 3));
template<> const GPIOPinTable_t _TIMChannel<8, 2>::afNeg = GPIO_PINS(pB(0, 3), pB(14, 3));
template<> const GPIOPinTable_t _TIMChannel<8, 3>::afPin = GPIO_PINS(pC(8, 3));
template<> const GPIOPinTable_t _TIMChannel<8, 3>::afNeg = GPIO_PINS(pB(1, 3), pB(15, 3));
template<> const GPIOPinTable_t _TIMChannel<8, 4>::afPin = GPIO_PINS(pC(9, 3));

template<> const GPIOPinTable_t _TIMChannel<15, 1>::afPin = GPIO_PINS(pA(2, 14), pB(14, 14), pF(9, 14), pG(10, 14));
template<> const GPIOPinTable_t _TIMChannel<15, 1>::afNeg = GPIO_PINS(pA(1, 14), pB(13, 14), pG(9, 14));
template<> const GPIOPinTable_t _TIMChannel<15, 2>::afPin = GPIO_PINS(pA(3, 14), pB(15, 14), pF(10, 14), pG(11, 14));

template<> const GPIOPinTable_t _TIMChannel<16, 1>::afPin = GPIO_PINS(pA(6, 14), pB(8, 14), pE(0, 14));
template<> const GPIOPinTable_t _TIMChannel<16, 1>::afNeg = GPIO_PINS(pB(6, 14));

template<> const GPIOPinTable_t _TIMChannel<17, 1>::afPin = GPIO_PINS(pA(7, 14), pB(9, 14), pE(1, 14));
template<> const GPIOPinTable_t _TIMChannel<17, 1>::afNeg = GPIO_PINS(pB(7, 14));

#pragma endregion

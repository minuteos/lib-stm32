/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/io/io.h
 */

#pragma once

#include_next <io/io.h>

// include all receive/transmit strategies
#include <io/DMAReceiver.h>
#include <io/DMAReceiverCircular.h>
#include <io/DMATransmitter.h>

#include <io/USARTInterrupt.h>
#include <io/USARTInterruptReceiver.h>
#include <io/USARTInterruptTransmitter.h>

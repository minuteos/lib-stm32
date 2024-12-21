/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/io/USARTInterrupt.h
 *
 * Handler for interrupt-based USART transfers
 *
 * Supports continuous transfers, but with per-transfer overhead caused
 * by interrupts being triggered. DMA should be used when possible.
 */

#pragma once

#include <hw/USART.h>

namespace io
{

class USARTInterrupt
{
public:
    static USARTInterrupt& Get(USART* usart);

    bool AddRxBuffer(Buffer buf) { return AddBuffer(false, buf.begin(), buf.end()); }
    char* const& RxPointer() const { return rx.p; }
    void ResetRx() { Reset(rx); }

    bool AddTxBuffer(Span buf) { return AddBuffer(true, (char*)buf.begin(), (char*)buf.end()); }
    char* const& TxPointer() const { return tx.p; }
    void ResetTx() { Reset(tx); }

private:
    USARTInterrupt(USART& usart, IRQ irq)
        : usart(usart), irq(irq), overrun(false), tx{}, rx{}
    {
    }

    struct Buf {
        char* p;
        char* e;
        char* p2;
        char* e2;

        bool CanAdd() const { return !p2; }
        operator char*() const { return p; }
        void Write(char b);
        void ReadInto(volatile uint16_t* p);
    };

    void Handler();
    bool AddBuffer(bool tx, char* p, char* e);
    void Reset(Buf& b);

    USART& usart;
    IRQ irq;
    bool overrun;
    Buf tx, rx;
};

}

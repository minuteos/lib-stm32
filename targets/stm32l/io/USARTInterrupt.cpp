/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/io/USARTInterrupt.cpp
 */

#include "USARTInterrupt.h"

//#define TRACE_USART_INTERRUPT    1

#if TRACE_USART_INTERRUPT
#define MYTRACE(ch) _DBGCHAR(ch)
#else
#define MYTRACE(ch)
#endif

namespace io
{

USARTInterrupt& USARTInterrupt::Get(USART* usart)
{
    auto irq = usart->IRQ();
    if (auto arg = irq.HandlerArgument())
    {
        return *(USARTInterrupt*)arg;
    }

    auto res = new(malloc_once(sizeof(USARTInterrupt))) USARTInterrupt(*usart, irq);
    irq.SetHandler(res, &USARTInterrupt::Handler);
    irq.Priority(CORTEX_MAXIMUM_PRIO);
    return *res;
}

OPTIMIZE void USARTInterrupt::Handler()
{
    auto status = usart.ISR;
    if (status & USART_ISR_RXNE)
    {
        MYTRACE('<');
        return rx.Write(usart.RDR);
    }
    if (status & USART_ISR_TXE)
    {
        if (tx.p == tx.e)
        {
            usart.CR1 &= ~USART_CR1_TXEIE;
        }
        else
        {
            return tx.ReadInto(&usart.TDR);
        }
    }
}

OPTIMIZE void USARTInterrupt::Buf::Write(char b)
{
    auto p = this->p, e = this->e;
    if (p == e) { MYTRACE('!'); return; }
    *p++ = b;
    if (p == e && p2)
    {
        this->p = p2;
        this->e = e2;
        p2 = e2 = NULL;
    }
    else
    {
        this->p = p;
    }
}

OPTIMIZE void USARTInterrupt::Buf::ReadInto(volatile uint32_t* reg)
{
    auto p = this->p, e = this->e;
    *reg = *p++;
    if (p == e && p2)
    {
        this->p = p2;
        this->e = e2;
        p2 = e2 = NULL;
    }
    else
    {
        this->p = p;
    }
}

bool USARTInterrupt::AddBuffer(bool tx, char* p, char* e)
{
    irq.Disable();
    auto& b = tx ? this->tx : this->rx;
    if (b.p == b.e)
    {
        // set first buffer
        b.p = p;
        b.e = e;
    }
    else if (!b.p2)
    {
        // set second buffer
        b.p2 = p;
        b.e2 = e;
    }
    else if (b.e2 == p)
    {
        // extend second buffer
        b.e2 = e;
    }
    else
    {
        irq.Enable();
        return false;
    }
    usart.CR1 |= tx ? USART_CR1_TXEIE : USART_CR1_RXNEIE;
    irq.Enable();
    return true;
}

void USARTInterrupt::Reset(Buf& b)
{
    irq.Disable();
    b = {};
    irq.Enable();
}

}
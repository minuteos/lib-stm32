/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/FLASH.cpp
 */

#include "FLASH.h"

static constexpr uint32_t FLASH_CR_PMASK = FLASH_CR_PG | FLASH_CR_PER | FLASH_CR_BKER | FLASH_CR_PNB;

bool _FLASH::WriteDouble(const volatile void* ptr, uint32_t lo, uint32_t hi)
{
    volatile uint32_t* wptr = (volatile uint32_t*)ptr;

    PLATFORM_CRITICAL_SECTION();
    while (SR & FLASH_SR_BSY);
    ASSERT(Locked());
    Unlock();
    SR = 0xFFFF;    // clear all errors
    MODMASK(CR, FLASH_CR_PMASK, FLASH_CR_PG);
    wptr[0] = lo;
    wptr[1] = hi;
    while (SR & FLASH_SR_BSY);
    CR &= ~FLASH_CR_PG;
    Lock();

    return wptr[0] == lo && wptr[1] == hi;
}

bool _FLASH::Write(const volatile void* ptr, Span data)
{
    auto len = data.Length();
    if (!len) { return true; }    // nothing to write

    volatile uint32_t* wptr = (volatile uint32_t*)ptr;
    auto rptr = data.Pointer<uint32_t>();

    PLATFORM_CRITICAL_SECTION();
    while (SR & FLASH_SR_BSY);
    ASSERT(Locked());
    Unlock();
    SR = 0xFFFF;    // clear all errors
    MODMASK(CR, FLASH_CR_PMASK, FLASH_CR_PG);

    size_t dwords = (len + 7) >> 3;
    for (; dwords; dwords--)
    {
        uint32_t w1 = *rptr++, w2 = *rptr++;
        if (dwords == 1 && (len &= 7))
        {
            // final dword with partial data, mask with ones so we don't store unknown bytes
            if (len > 4)
            {
                // mask part of second word
                w2 |= ~0u << ((len - 4) << 3);
            }
            else
            {
                // mask part of first word (<= 4) and the entire second word
                w1 |= ~0u << (len << 3);
                w2 = ~0u;
            }
        }
        *wptr++ = w1;
        *wptr++ = w2;
        while (SR & FLASH_SR_BSY);
        if (wptr[-2] != w1 || wptr[-1] != w2)
        {
            break;
        }
    }

    CR &= ~FLASH_CR_PG;
    Lock();
    return !dwords;
}

bool _FLASH::Erase(const volatile void* ptr, uint32_t length)
{
    // number of first and last page to erase
    int n = intptr_t(ptr) / PageSize;
    int ne = (intptr_t(ptr) + length - 1) / PageSize;

    while (n <= ne)
    {
        {
            PLATFORM_CRITICAL_SECTION();
            while (SR & FLASH_SR_BSY);
            ASSERT(Locked());
            Unlock();
            SR = 0xFFFF;    // clear all errors
            MODMASK(CR, FLASH_CR_PMASK,
                FLASH_CR_PER | (n << FLASH_CR_PNB_Pos) | FLASH_CR_STRT);
            Lock();
        }
        while (SR & FLASH_SR_BSY);

        if (!Span((const void*)(n * PageSize), PageSize).IsAllOnes())
        {
            break;
        }

        n++;
    }

    return n > ne;
}

bool _FLASH::TryErasePage(const volatile void* ptr)
{
    return Erase(ptr, PageSize);
}

async(_FLASH::ErasePage, const volatile void* ptr)
async_def(
    int n;
)
{
    f.n = intptr_t(ptr) / PageSize;

    for (;;)
    {
        await_mask(SR, FLASH_SR_BSY, 0);

        PLATFORM_CRITICAL_SECTION();
        if (SR & FLASH_SR_BSY) { continue; }    // try again
        ASSERT(Locked());
        Unlock();
        SR = 0xFFFF;    // clear all errors
        MODMASK(CR, FLASH_CR_PMASK,
            FLASH_CR_PER | (f.n << FLASH_CR_PNB_Pos) | FLASH_CR_STRT);
        Lock();
        break;
    }

    await_mask(SR, FLASH_SR_BSY, 0);

    async_return(Span((const void*)(f.n * PageSize), PageSize).IsAllOnes());
}
async_end


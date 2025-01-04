/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/FLASH.h
 */

#pragma once

#include <base/base.h>
#include <base/Span.h>

#include <kernel/kernel.h>

#undef FLASH
#define FLASH   CM_PERIPHERAL(_FLASH, FLASH_R_BASE)

class _FLASH : public FLASH_TypeDef
{
public:
    static constexpr size_t PageSize = 2048;

    constexpr size_t Size() const { return FLASH_SIZE; }

    bool WriteDouble(const volatile void* ptr, uint32_t lo, uint32_t hi);
    bool Write(const volatile void* ptr, Span data);
    bool Erase(const volatile void* ptr, uint32_t length);
    bool TryErasePage(const volatile void* ptr);
    async(ErasePage, const volatile void* ptr);

private:
    void Unlock()
    {
        KEYR = 0x45670123;
        KEYR = 0xCDEF89AB;
    };

    void Lock()
    {
        CR |= FLASH_CR_LOCK;
    }
};

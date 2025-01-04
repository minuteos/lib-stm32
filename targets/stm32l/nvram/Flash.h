/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/nvram/Flash.h
 *
 * Flash interface for lib-nvram on STM32 L-series
 */

#include <base/base.h>
#include <base/Span.h>

#include <kernel/kernel.h>

#include <hw/FLASH.h>

#define NVRAM_FLASH_DOUBLE_WRITE    1

namespace nvram
{

class Flash
{
public:
    static constexpr size_t PageSize = FLASH->PageSize;

    ALWAYS_INLINE static Span GetRange() { return Span((const void*)FLASH_BASE, FLASH->Size()); }

    static bool Write(const void* ptr, Span data) { return FLASH->Write(ptr, data); }
    static bool WriteDouble(const void* ptr, uint32_t lo, uint32_t hi) { return FLASH->WriteDouble(ptr, lo, hi); }
    static void ShredDouble(const void* ptr) { while (!FLASH->WriteDouble(ptr, 0, 0)); }
    static bool Erase(Span range) { return FLASH->Erase(range.Pointer(), range.Length()); }
    static async(ErasePageAsync, const void* ptr) { return async_forward(FLASH->ErasePage, ptr); }
};

}

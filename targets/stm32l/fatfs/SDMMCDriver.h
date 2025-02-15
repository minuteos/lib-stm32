/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/fatfs/SDMMCDriver.h
 */

#pragma once

#include <fatfs/fatfs.h>

#include <hw/SDMMC.h>

namespace fatfs
{

class SDMMCDriver : public DiskDriver
{
public:
    SDMMCDriver(SDMMC* sd, DMAChannel* dma)
        : sd(*sd), dma(*dma) {}

    static void Register(SDMMC* sd);

    virtual async(Init) final override;
    async(Test);

    const SDMMC::CardInfo& CardInfo() const { return ci; }

protected:
    virtual async(Read, void* buf, LBA_t sectorStart, size_t sectorCount) final override;
    virtual async(Write, const void* buf, LBA_t sectorStart, size_t sectorCount) final override;
    virtual async(IoCtl, uint8_t cmd, void* buff) final override;

private:
    SDMMC& sd;
    DMAChannel& dma;
    SDMMC::CardInfo ci;
    unsigned addrMul;
};

}

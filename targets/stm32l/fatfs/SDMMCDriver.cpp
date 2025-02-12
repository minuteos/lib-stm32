/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/fatfs/SDMMCDriver.cpp
 */

#include "SDMMCDriver.h"

#define MYDBG(...)  DBGCL("SDMMC", __VA_ARGS__)

//#define SDMMC_TRACE   1

#if SDMMC_TRACE
#define MYTRACE(...)  MYDBG(__VA_ARGS__)
#else
#define MYTRACE(...)
#endif

namespace fatfs
{

async(SDMMCDriver::Init)
async_def(
    bool v2;
    uint32_t init;
    Timeout timeout;
)
{
    if (Status() & STA_NOINIT)
    {
        // enable SDMMC clock, reset it
        RCC->APB2RSTR |= RCC_APB2RSTR_SDMMC1RST;
        RCC->APB2ENR |= RCC_APB2ENR_SDMMC1EN;
        RCC->APB2RSTR &= ~RCC_APB2RSTR_SDMMC1RST;

        await(sd.Initialize);

        Status(STA_NODISK);
    }

    if (Status() & STA_NODISK)
    {
        if (await(sd.IdentifyCard, ci))
        {
            addrMul = ci.blockAddressing ? 1 : FF_MAX_SS;
            if (sd.Command_SelectCard(ci.rca.rca))
            {
                Status(0);
                async_return(RES_OK);
            }
        }

        Status(STA_NODISK | STA_NOINIT);
        async_return(RES_ERROR);
    }

    async_return(RES_OK);
}
async_end

async(SDMMCDriver::Test)
async_def()
{
    if (!await(sd.WaitNotBusy, Timeout::Seconds(1)))
    {
        MYDBG("Timeout waiting for card to become available");
        Status(STA_NODISK | STA_NOINIT);
        async_return(RES_ERROR);
    }

    if (!sd.Command_SendStatus(ci.rca.rca))
    {
        MYDBG("Failed to get card status");
        Status(STA_NODISK | STA_NOINIT);
        async_return(RES_ERROR);
    }

    async_return(RES_OK);
}
async_end

async(SDMMCDriver::Read, void* buf, LBA_t sectorStart, size_t sectorCount)
async_def(
    size_t i;
    LBA_t sec;
    void* buf;
    unsigned t0, t1;
)
{
    SDMMC::CommandResult cr;

    f.sec = sectorStart;
    f.buf = buf;
    for (f.i = 0; f.i < sectorCount; f.i++)
    {
        if (!await(sd.WaitNotBusy, Timeout::Seconds(1)))
        {
            MYDBG("Timeout waiting for card to become available");
            Status(STA_NODISK | STA_NOINIT);
            async_return(RES_ERROR);
        }

        // TODO: multi-block read
        sd.ConfigureDmaRead(dma, Buffer(f.buf, FF_MAX_SS));
        if (!(cr = sd.Command_ReadSingleBlock(f.sec * addrMul)))
        {
            sd.AbortDmaTransfer(dma);
            MYDBG("Failed to start reading sector %X: %X", f.sec, cr);
            Status(STA_NODISK | STA_NOINIT);
            async_return(RES_ERROR);
        }

        MYTRACE("Reading sector %X, RS: %X", f.sec, sd.RESP1);
        if (!await(sd.WaitNotBusy, Timeout::Seconds(1)))
        {
            sd.AbortDmaTransfer(dma);
            MYDBG("Timeout while reading sector %X", f.sec);
            Status(STA_NODISK | STA_NOINIT);
            async_return(RES_ERROR);
        }

        auto dr = sd.CompleteDmaTransfer(dma);
        if (!dr)
        {
            MYDBG("Error while reading sector %X: %X", f.sec, dr);
            Status(STA_NODISK | STA_NOINIT);
            async_return(RES_ERROR);
        }
        MYTRACE("Read sector %X", f.sec);

        f.buf = (char*)f.buf + FF_MAX_SS;
        f.sec++;
    }

    async_return(RES_OK);
}
async_end

async(SDMMCDriver::Write, const void* buf, LBA_t sectorStart, size_t sectorCount)
async_def(
    size_t i;
    LBA_t sec;
    const void* buf;
)
{
    f.sec = sectorStart;
    f.buf = buf;
    for (f.i = 0; f.i < sectorCount; f.i++)
    {
        if (!await(sd.WaitNotBusy, Timeout::Seconds(1)))
        {
            MYDBG("Timeout waiting for card to become available");
            Status(STA_NODISK | STA_NOINIT);
            async_return(RES_ERROR);
        }

        // TODO: multi-block write
        ASSERT(!dma.IsEnabled());

        if (!sd.Command_WriteBlock(f.sec * addrMul))
        {
            MYDBG("Failed to start writing sector %X: %X", f.sec, sd.STA);
            Status(STA_NODISK | STA_NOINIT);
            async_return(RES_ERROR);
        }

        MYTRACE("Writing sector %X from %p, RS: %X", f.sec, f.buf, sd.RESP1);
        sd.ConfigureDmaWrite(dma, Span(f.buf, FF_MAX_SS));
        if (!await(sd.WaitNotBusy, Timeout::Seconds(1)))
        {
            sd.AbortDmaTransfer(dma);
            MYDBG("Timeout while writing sector %X", f.sec);
            Status(STA_NODISK | STA_NOINIT);
            async_return(RES_ERROR);
        }

        auto dr = sd.CompleteDmaTransfer(dma);
        if (!dr)
        {
            MYDBG("Error while writing sector %X: %X", f.sec, sd.STA);
            Status(STA_NODISK | STA_NOINIT);
            async_return(RES_ERROR);
        }
        MYTRACE("Written sector %X: %X", f.sec, sd.STA);

        f.buf = (char*)f.buf + FF_MAX_SS;
        f.sec++;
    }

    async_return(RES_OK);
}
async_end

async(SDMMCDriver::IoCtl, uint8_t cmd, void* buff)
async_def_sync()
{
}
async_end

}

/*
 * Copyright (c) 2025 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/SDMMC.cpp
 */

#include "SDMMC.h"

#define MYDBG(...)      DBGCL("SDMMC", __VA_ARGS__)

//#define SDMMC_TRACE   1

#if SDMMC_TRACE
#define MYTRACE(...)    MYDBG(__VA_ARGS__)
#else
#define MYTRACE(...)
#endif

template<> const GPIOPinTable_t _SDMMC<1>::afClk = GPIO_PINS(pC(12, 12));
template<> const GPIOPinTable_t _SDMMC<1>::afCmd = GPIO_PINS(pD(2, 12));
template<> const GPIOPinTable_t _SDMMC<1>::afD0 = GPIO_PINS(pC(8, 12));
template<> const GPIOPinTable_t _SDMMC<1>::afD1 = GPIO_PINS(pC(9, 12));
template<> const GPIOPinTable_t _SDMMC<1>::afD2 = GPIO_PINS(pC(10, 12));
template<> const GPIOPinTable_t _SDMMC<1>::afD3 = GPIO_PINS(pC(11, 12));
template<> const GPIOPinTable_t _SDMMC<1>::afD4 = GPIO_PINS(pB(8, 12));
template<> const GPIOPinTable_t _SDMMC<1>::afD5 = GPIO_PINS(pB(9, 12));
template<> const GPIOPinTable_t _SDMMC<1>::afD6 = GPIO_PINS(pC(6, 12));
template<> const GPIOPinTable_t _SDMMC<1>::afD7 = GPIO_PINS(pC(7, 12));

SDMMC::CommandResult SDMMC::Command(uint32_t cmd, uint32_t arg)
{
    uint32_t state;

    if (int(cmd) < 0)
    {
        // app command
        cmd = ~cmd;
        auto res = Command(CmdApp, cmd >> 16);
        if (!res) { return res; }
    }

    ICR = SDMMC_STA_CMASK | SDMMC_STA_DMASK;
    __DSB();
    ARG = arg;
    CMD = (cmd | SDMMC_CMD_CPSMEN) & 0xFFF;
    __DSB();
    while (!(state = (STA & SDMMC_STA_CMASK)));
    MYTRACE("%d %X %d %X = %X %X", !!(cmd & RespNoCmd), (cmd >> 6) & 0x3F, cmd & 0x3F, arg, STA, RESP1);

    auto expectResp = ((cmd & SDMMC_CMD_WAITRESP) == RespShort) && !(cmd & RespNoCmd) ? cmd & SDMMC_CMD_CMDINDEX : 0x3F;
    if (RESPCMD != expectResp)
    {
        // RESPCMD mismatch, report as CRC failure
        MYTRACE("RESPCMD %d != %d", RESPCMD, expectResp);
        return SDMMC_STA_CCRCFAIL;
    }

    if ((cmd & RespNoCRC) && (state & SDMMC_STA_CCRCFAIL))
    {
        // convert CRC failure into success
        state ^= SDMMC_STA_CCRCFAIL | SDMMC_STA_CMDREND;
    }

    return state;
}

async(SDMMC::Initialize)
async_def()
{
    // power off
    Configure(CardClockEnable(false));
    Configure(Power(false));

    async_delay_ms(1);

    // set lowest clock frequency for init phase
    Configure(CardClockDivisor(256) | PowerSaving() | BusWidth(BusWidth1) | CardClockDephase(false) | FlowControl());
    Configure(Power());

    async_delay_ms(1);

    Configure(CardClockEnable());

    async_delay_ms(1);
}
async_end

async(SDMMC::IdentifyCard, CardInfo& info)
async_def(
    uint32_t init;
    bool v2;
    Timeout timeout;
)
{
    // card ID procedure
    if (!Command_GoIdle()) { async_return(false); }

    f.v2 = true;
    if (!Command_SendIfCond())
    {
        f.v2 = false;
        // V1.x card, go back to idle
        if (!Command_GoIdle()) { async_return(false); }
    }

    // query voltage ranges
    if (!AppCommand_SendOpCond(SD_OpCondHS * f.v2)) { async_return(false); }

    // at this point, we're definitely talking to a real card,
    // log any errors encountered

    // wait for init to complete, sending back whatever voltages the card supports
    f.init = RESP1 & (SD_OpCondHS | SD_OpCondVoltages);
    if (!AppCommand_SendOpCond(f.init))
    {
        MYDBG("Failed to start card init: %X", STA);
        async_return(false);
    }

    f.timeout = Timeout::Milliseconds(1500).MakeAbsolute();

    for (;;)
    {
        async_yield();
        if (!AppCommand_SendOpCond(f.init))
        {
            MYDBG("Error during card init: %X", STA);
            async_return(false);
        }

        if (RESP1 & SD_OpCondInitDone)
        {
            info.blockAddressing = RESP1 & SD_OpCondHS;
            break;
        }

        if (f.timeout.Elapsed())
        {
            MYDBG("Card init never completed");
            async_return(false);
        }
    }

    if (!Command_AllSendCid())
    {
        MYDBG("Failed to retrieve CID: %08X", STA);
        async_return(false);
    }

    info.cid = ResultCid();

    if (!Command_SendRelativeAddr())
    {
        MYDBG("Failed to retrieve RCA: %08X", STA);
        async_return(false);
    }
    info.rca = ResultRca();

    // try switching to high speed
    Configure(CardClockDivisor(1));

    if (!Command_SendCsd(info.rca.rca))
    {
        MYDBG("Failed to retrieve CSD or communicate with card at full speed: %08X", STA);
        async_return(false);
    }
    info.csd = ResultCsd();

#if TRACE
    auto& cid = info.cid;
    auto& rca = info.rca;
    auto& csd = info.csd;
    MYDBG("Card identification successful");
    MYDBG("CID: %02X:%.2s %.5s R%d.%d SN %08X MFG %02d-%02d",
        cid.mfg, cid.appId, cid.prod,
        cid.bcdRev >> 4, cid.bcdRev & 0xF,
        cid.psn, cid.Year(), cid.month);
    MYDBG("RCA: %04X %c%c%c %d", rca.rca,
        rca.error ? 'E' : '-',
        rca.crcError ? 'C' : '-',
        rca.illegalCmd ? 'I' : '-',
        rca.status);
    MYDBG("CSD: %s, %d blocks (%d MB)", info.blockAddressing ? "SDHC" : "SDSC", csd.BlockCount(), csd.CapacityMB());
#endif

    async_return(true);
}
async_end

/*
400E00325B59000039B37F800A4000F8

01 000000 | 00001110 | 00000000 | 00110010 |
01011011|0101 1001 | 0 0 0 0 00 00|00000000 | 00 111 001 | 101 100 11|0 1 1111111 0000000 0 00 010 1001 0 00000 0 0 0 0 00 0 0 11111000
*/
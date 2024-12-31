/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32/hw/DMA.cpp
 */

#include "DMA.h"

#define MYDBG(...)  DBGCL("DMA", __VA_ARGS__)

DMAChannel* DMA::ClaimChannel(uint32_t specs)
{
    while (specs)
    {
        ChannelSpec spec = { .spec = uint8_t(specs) };
        auto dma = spec.dma ? DMA2 : DMA1;
        auto n = spec.ch - 1;   // zero-based index, the spec is one-based
        auto& ch = dma->CH[n];
        if (!ch.CCR)
        {
            auto off = n << 2;
            dma->EnableClock();
            MODMASK(dma->CSELR, MASK(4) << off, spec.map << off);
            ch.CCR = DMA_CCR_MEM2MEM;
            MYDBG("DMA %d channel %d claimed for function %d", spec.dma + 1, spec.ch, spec.map);
            return &ch;
        }
        specs >>= 8;
    }

    MYDBG("WARNING! Failed to claim a DMA channel");
    return NULL;
}

class IRQ DMAChannel::IRQ() const
{
    // need a lookup table as the numbers aren't completely contiguous
    return LOOKUP_TABLE(IRQn_Type,
            DMA1_Channel1_IRQn, DMA1_Channel2_IRQn, DMA1_Channel3_IRQn, DMA1_Channel4_IRQn, DMA1_Channel5_IRQn, DMA1_Channel6_IRQn, DMA1_Channel7_IRQn, {},
            DMA2_Channel1_IRQn, DMA2_Channel2_IRQn, DMA2_Channel3_IRQn, DMA2_Channel4_IRQn, DMA2_Channel5_IRQn, DMA2_Channel6_IRQn, DMA2_Channel7_IRQn
            )
        [DMA().Index() << 3 | Index()];
}

struct DMAChannelLink : DMA_Channel_TypeDef
{
    //! this field overlaps the reserved channel field to make the size the same as a full channel structure
    //! we use it to store the full length of the currently running transfer
    uint32_t CNDTR0;
};

//! A structure holding the fields of the transfer to be automatically linked after the current one
struct DMALinkInfo
{
    DMAChannelLink CH[7];
};

static inline DMAChannelLink& GetLink(DMAChannel* ch)
{
    static DMALinkInfo s_linkInfo[2];
    return *(DMAChannelLink*)(uintptr_t(&s_linkInfo[ch->DMA().Index()]) + (uintptr_t(ch) & 0xFF));
}

struct DMALinkStatus
{
    char* end;
    uint32_t cndtr;
};

//! Gets the status of the currently active transfer started via buffers linking
static Packed<DMALinkStatus> GetStatus(DMAChannel* ch)
{
    bool wasEnabled = ch->IsEnabled();
    ch->Disable();

    char* buf = (char*)ch->CMAR;
    auto cndtr = ch->CNDTR;
    buf += GetLink(ch).CNDTR0;

    if (wasEnabled)
    {
        ch->Enable();
    }

    return pack(DMALinkStatus { buf, cndtr });
}

size_t DMAChannel::TryLinkBuffer(Span buf)
{
    auto irq = IRQ();
    irq.SetHandler(this, &DMAChannel::LinkHandler);
    irq.Priority(CORTEX_MAXIMUM_PRIO);
    irq.Disable();

    auto& link = GetLink(this);

    if (!link.CCR)
    {
        // configure the linked buffer with the same CCR and CPAR
        link.CCR = CCR | DMA_CCR_TCIE | DMA_CCR_EN;
        link.CPAR = CPAR;
        link.CMAR = uint32_t(buf.Pointer());

        // respect the maximum transfer size
        buf = buf.Left(DMA_CNDTR_NDT);
        link.CNDTR = buf.Length();
    }
    else if (link.CMAR + link.CNDTR == uint32_t(buf.Pointer()))
    {
        // extend the link buffer by as much as possible
        buf = buf.Left(DMA_CNDTR_NDT - link.CNDTR);
        link.CNDTR += buf.Length();
    }
    else
    {
        buf = {};
    }

    irq.Enable();

    if (!IsEnabled())
    {
        irq.Trigger();
    }

    return buf.Length();
}

void DMAChannel::LinkHandler()
{
    DMA().IFCR = DMA_IFCR_CTCIF1 << (Index() << 2);
    auto& link = GetLink(this);
    ASSERT(CNDTR == 0);
    Disable();
    if (link.CCR)
    {
        link.CNDTR0 = CNDTR = link.CNDTR;
        CPAR = link.CPAR;
        CMAR = link.CMAR;
        CCR = link.CCR;
        link.CCR = 0;
    }
}

void DMAChannel::Unlink()
{
    Disable();
    GetLink(this) = {};
}

const char* DMAChannel::LinkPointer()
{
    auto s = unpack<DMALinkStatus>(GetStatus(this));
    return s.end - s.cndtr;
}

#if Ckernel

async_once(DMAChannel::LinkPointerNot, const char* p, Timeout timeout)
{
    auto s = unpack<DMALinkStatus>(GetStatus(this));
    uint32_t cndtr = s.end - p;
    return async_forward(WaitMaskNot, CNDTR, ~0u, cndtr, timeout);
}

#endif
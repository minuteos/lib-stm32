/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/DMA.h
 */

#pragma once

#include <base/base.h>
#include <base/Span.h>

#include <hw/RCC.h>
#include <hw/IRQ.h>

#if Ckernel
#include <kernel/kernel.h>
#endif

struct DMADescriptor
{
    uint32_t CCR, CNDTR, CPAR, CMAR;

    enum
    {
        MaximumTransferSize = (DMA_CNDTR_NDT_Msk >> DMA_CNDTR_NDT_Pos) + 1,
    };

    enum Flags
    {
        Start = DMA_CCR_EN,

        InterruptComplete = DMA_CCR_TCIE,
        InterruptHalf = DMA_CCR_HTIE,
        InterruptError = DMA_CCR_TEIE,

        P2M = 0,
        P2P = 0,
        M2P = DMA_CCR_DIR,
        M2M = DMA_CCR_MEM2MEM,

        Circular = DMA_CCR_CIRC,
        IncrementMemory = DMA_CCR_MINC,
        IncrementPeripheral = DMA_CCR_PINC,

        PUnitByte = 0 << DMA_CCR_PSIZE_Pos,
        PUnitHalfWord = 1 << DMA_CCR_PSIZE_Pos,
        PUnitWord = 2 << DMA_CCR_PSIZE_Pos,

        MUnitByte = 0 << DMA_CCR_MSIZE_Pos,
        MUnitHalfWord = 1 << DMA_CCR_MSIZE_Pos,
        MUnitWord = 2 << DMA_CCR_MSIZE_Pos,

        UnitByte = PUnitByte | MUnitByte,
        UnitHalfWord = PUnitHalfWord | MUnitHalfWord,
        UnitWord = PUnitWord | MUnitWord,

        PrioLow = 0 << DMA_CCR_PL_Pos,
        PrioMedium = 1 << DMA_CCR_PL_Pos,
        PrioHigh = 2 << DMA_CCR_PL_Pos,
        PrioVeryHigh = 3 << DMA_CCR_PL_Pos,
    };

    static constexpr DMADescriptor Transfer(volatile const void* source, volatile void* destination, size_t count, Flags flags)
    {
        ASSERT(!((count << DMA_CNDTR_NDT_Pos) & ~DMA_CNDTR_NDT_Msk));

        return {
            flags,
            count << DMA_CNDTR_NDT_Pos,
            (uint32_t)(flags & DMA_CCR_DIR ? destination : source),
            (uint32_t)(flags & DMA_CCR_DIR ? source : destination),
        };
    }
};

DEFINE_FLAG_ENUM(DMADescriptor::Flags);

#ifdef DMA1
#undef DMA1
#define DMA1    CM_PERIPHERAL(DMA, DMA1_BASE)
#endif

#ifdef DMA2
#undef DMA2
#define DMA2    CM_PERIPHERAL(DMA, DMA2_BASE)
#endif

struct DMAChannel : DMA_Channel_TypeDef
{
    uint32_t : 32;  // reserved word, not included in DMA_Channel_TypeDef

    ALWAYS_INLINE struct DMA& DMA() const { return *(struct DMA*)((uint32_t)this & ~0xFF); }

    //! Gets the zero-based channel Index
    ALWAYS_INLINE constexpr int Index() const { return (((uint32_t)this - DMA1_Channel1_BASE) & 0xFF) / sizeof(DMAChannel); }

    //! Enables the DMA channel
    ALWAYS_INLINE void Enable() { CCR |= DMA_CCR_EN; }
    //! Disables the DMA channel
    ALWAYS_INLINE void Disable() { CCR &= ~DMA_CCR_EN; }
    //! Returns true if the channel is enabled
    ALWAYS_INLINE bool IsEnabled() const { return CCR & DMA_CCR_EN; }
    //! Releases the channel (disables it and makes it available for other use)
    ALWAYS_INLINE void Release() { CCR = 0; }

    //! Gets the IRQ for the current DMA channel
    class IRQ IRQ() const;

    //! Attempts to link a buffer, returning the number of bytes successfully linked
    size_t TryLinkBuffer(Span buf);
    //! Unlinks all buffers and stops the transfer to the current one
    void Unlink();
    //! Retrieves the memory location where next transfer will take place
    const char* LinkPointer();

#if Ckernel
    ALWAYS_INLINE async_once(WaitForEnabled, bool state)
    async_once_def()
    {
        await_mask(CCR, DMA_CCR_EN, state ? DMA_CCR_EN : 0);
    }
    async_end

    async_once(WaitForComplete);
    //! Waits until the transfer pointer moves away from the specified
    async_once(LinkPointerNot, const char* p, Timeout timeout = Timeout::Infinite);
#endif

private:
    void LinkHandler();
};

struct DMA : DMA_TypeDef
{
    // the fields below are split into separate structures in the CMSIS
    // defs for some reason
    DMAChannel CH[7];            /*!< DMA channels 1-7 */
    uint32_t __reserved[5];      /*!< Reserved */
      __IO uint32_t CSELR;       /*!< DMA channel selection register              */

    //! Enables peripheral clock
    ALWAYS_INLINE void EnableClock() { RCC->EnableDMA(Index()); }

    //! Gets the zero-based index of the peripheral
    ALWAYS_INLINE constexpr unsigned Index() const { return (uintptr_t(this) >> 10) & 1; }

    //! Gets the specified channel
    ALWAYS_INLINE constexpr DMAChannel& Channel(unsigned channel) { channel--; ASSERT(channel < 7); return CH[channel]; }

    template<typename ...Channels> ALWAYS_INLINE void ClearInterrupts(Channels... channels)
    {
        IFCR = ((0xF << ((channels - 1) << 2)) | ...);
    }

    ALWAYS_INLINE bool TransferComplete(unsigned channel) { channel--; ASSERT(channel < 7); return ISR & 2 << (channel << 2); }

#if Ckernel
    ALWAYS_INLINE async_once(WaitForComplete, unsigned channel)
    async_once_def()
    {
        channel--;
        ASSERT(channel < 7);
        await_mask_not(ISR, 2 << (channel < 2), 0);
    }
    async_end
#endif

    struct ChannelSpec
    {
        union
        {
            struct
            {
                uint8_t dma : 1;
                uint8_t ch : 3;
                uint8_t map : 4;
            };
            uint8_t spec;
        };
    };

    static DMAChannel* ClaimChannel(const ChannelSpec& spec) { return ClaimChannel(spec.spec); }
    static DMAChannel* ClaimChannel(const ChannelSpec& spec, const ChannelSpec& altSpec) { return ClaimChannel(spec.spec | altSpec.spec << 8); }

private:
    static DMAChannel* ClaimChannel(uint32_t specs);
};

#if Ckernel

ALWAYS_INLINE async_once(DMAChannel::WaitForComplete) { return async_forward(DMA().WaitForComplete, Index() + 1); }

#endif
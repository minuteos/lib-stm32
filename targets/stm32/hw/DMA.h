/*
 * Copyright (c) 2024 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * stm32l/hw/DMA.h
 */

#include <base/base.h>

#include <hw/RCC.h>

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
        PrioMedium = 0 << DMA_CCR_PL_Pos,
        PrioHigh = 0 << DMA_CCR_PL_Pos,
        PrioVeryHigh = 0 << DMA_CCR_PL_Pos,
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

    struct DMA& DMA() { return *(struct DMA*)((uint32_t)this & ~0xFF); }

    //! Gets the zero-based channel Index
    int Index() { return (((uint32_t)this - DMA1_Channel1_BASE) & 0xFF) / sizeof(DMAChannel); }

    //! Enables the DMA channel
    void Enable() { CCR |= DMA_CCR_EN; }
    //! Disables the DMA channel
    void Disable() { CCR &= ~DMA_CCR_EN; }
    bool IsEnabled() const { return CCR & DMA_CCR_EN; }

#if Ckernel
    ALWAYS_INLINE async(WaitForEnabled, bool state)
    async_def_once()
    {
        await_mask(CCR, DMA_CCR_EN, state ? DMA_CCR_EN : 0);
    }
    async_end

    async(WaitForComplete);
#endif
};

struct DMA : DMA_TypeDef
{
    // the fields below are split into separate structures in the CMSIS
    // defs for some reason
    DMAChannel CH[7];            /*!< DMA channels 1-7 */
    uint32_t __reserved[5];      /*!< Reserved */
      __IO uint32_t CSELR;       /*!< DMA channel selection register              */

    //! Enables peripheral clock
    void EnableClock() { RCC->EnableDMA(Index()); }

    //! Gets the zero-based index of the peripheral
    unsigned Index() const { return (unsigned(this) - DMA1_BASE) / (DMA2_BASE - DMA1_BASE); }

    //! Gets the specified channel
    DMAChannel& Channel(unsigned channel) { channel--; ASSERT(channel < 7); return CH[channel]; }

    template<typename ...Channels> void ClearInterrupts(Channels... channels)
    {
        IFCR = ((0xF << ((channels - 1) << 2)) | ...);
    }

    bool TransferComplete(unsigned channel) { channel--; ASSERT(channel < 7); return ISR & 2 << (channel << 2); }

#if Ckernel
    ALWAYS_INLINE async(WaitForComplete, unsigned channel)
    async_def_once()
    {
        channel--;
        ASSERT(channel < 7); 
        await_mask_not(ISR, 2 << (channel < 2), 0);
    }
    async_end
#endif
};

#if Ckernel

ALWAYS_INLINE async(DMAChannel::WaitForComplete) { return async_forward(DMA().WaitForComplete, Index() + 1); }

#endif
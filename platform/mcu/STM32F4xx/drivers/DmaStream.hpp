/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef DMA_STREAM_H
#define DMA_STREAM_H

#include <kernel/scheduler/scheduler.h>
#include <stm32f4xx.h>
#include <miosix.h>
#include <functional>

/**
 * Enumerating type describing the memory and peripheral data sizes allowed
 * for a DMA transfer.
 */
enum class DataSize
{
    _8BIT  = 0,     ///< 8-bit data size
    _16BIT = 1,     ///< 16-bit data size
    _32BIT = 2,     ///< 32-bit data size
};

/**
 * Stream handler class for STM32F4 DMA Stream peripheral.
 */
class StreamHandler
{
public:

    /**
     * Constructor.
     *
     * @param s: pointer to the DMA stream peripheral.
     * @param IRQn: DMA stream IRQ number.
     */
    StreamHandler(DMA_Stream_TypeDef *s, IRQn_Type IRQn) : IRQn(IRQn), stream(s)
    { }

    /**
     * Destructor.
     */
    ~StreamHandler()
    {
        stream->CR = 0;
        NVIC_DisableIRQ(IRQn);
    }

    /**
     * Start a DMA stream.
     *
     * @param periph: pointer to source/target peripheral.
     * @param memory: pointer to source/target memory.
     * @param size: transfer size, in elements.
     * @param circ: if true the stream runs in circular double buffered mode.
     */
    void start(volatile void *periph, void *memory, const size_t size,
               const bool circ = false)
    {
        uint32_t circFlags = DMA_SxCR_CIRC      // Circular buffer mode
                           | DMA_SxCR_HTIE;     // Half transfer interrupt

        if(circ)
            stream->CR |= circFlags;
        else
            stream->CR &= ~circFlags;

        transferSize = size;
        stream->NDTR = size;
        stream->PAR  = reinterpret_cast< uint32_t >(periph);
        stream->M0AR = reinterpret_cast< uint32_t >(memory);
        stream->CR  |= DMA_SxCR_EN;
    }

    /**
     * Get a pointer to the currently "idle" section of a DMA stream that is,
     * the section not being read by the DMA.
     * A call to this function is meaningful only if the stream is running in
     * circular double buffered mode.
     *
     * @return address of the idle section or NULL if the stream is not in
     * circular mode.
     */
    void *idleBuf()
    {
        if((stream->CR & DMA_SxCR_CIRC) == 0)
            return NULL;

        miosix::FastInterruptDisableLock dLock;
        uint32_t curPos = stream->NDTR;
        uint32_t idle   = stream->M0AR;
        uint32_t size   = (stream->CR >> 13U) & 0x03;

        if(curPos > (transferSize / 2))
            idle += (transferSize / 2) * size * 2;

        return reinterpret_cast< void * >(idle);
    }

    /**
     * Syncronize the execution flow with the stream transfer, blocking function.
     * For cirular buffer mode the calling thread is put to sleep until the DMA
     * reaches either the half or the end of the transfer, whichever comes first.
     * For linear buffer mode the calling thread is put to sleep until the DMA
     * reaches the end of the transfer.
     *
     * @return true if thread was effectively put to sleep, false otherwise.
     */
    bool sync()
    {
        using namespace miosix;

        // Enter in critical section until the end of the function
        FastInterruptDisableLock dLock;

        Thread *curThread = Thread::IRQgetCurrentThread();
        if((waiting != 0) && (waiting != curThread))
            return false;

        waiting = curThread;

        do
        {
            Thread::IRQwait();
            {
                // Re-enable interrupts while waiting for IRQ
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
        while(waiting != 0);

        waiting = 0;
        return true;
    }

    /**
     * Stop an ongoing DMA stream.
     * The stream does not stop immediately but only when it reaches the half
     * or the end of the transfer.
     */
    inline void stop()
    {
        stopTransfer = true;
    }

    /**
     * Forcefully stop an ongoing DMA stream.
     * Calling this function causes the immediate stop of an ongoing stream.
     */
    inline void halt()
    {
        stopTransfer = true;
        NVIC_SetPendingIRQ(IRQn);
    }

    /**
     * Query che the current status of the stream.
     *
     * @return true if the stream is active, false otherwise.
     */
    inline bool running()
    {
        return (stream->CR & DMA_SxCR_EN) ? true : false;
    }

    /**
     * Register a function to be called on stream end.
     *
     * @param callback: std::function for the stream end callback.
     */
    inline void setEndTransferCallback(std::function<void()>&& callback)
    {
        streamEndCallback = callback;
    }

    /**
     * Stream interrupt handler function.
     *
     * @param irqFlags: IRQ status flags.
     */
    void IRQhandler(const uint32_t irqFlags)
    {
        (void) irqFlags;

        using namespace miosix;

        if(((stream->CR & DMA_SxCR_CIRC) == 0) || (stopTransfer == true))
        {
            stream->CR &= ~DMA_SxCR_EN;
            stopTransfer = false;

            if(streamEndCallback)
                streamEndCallback();
        }

        // Wake up eventual pending threads
        if(waiting == 0) return;
        waiting->IRQwakeup();
        if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
            Scheduler::IRQfindNextThread();
        waiting = 0;
    }

private:

    bool                  stopTransfer;
    size_t                transferSize;
    const IRQn_Type       IRQn;
    std::function<void()> streamEndCallback;
    DMA_Stream_TypeDef   *stream;
    miosix::Thread       *waiting;
};

/**
 * Stream class for STM32F4 DMA Stream peripheral.
 *
 * @tparam DMA: base address of the DMA peripheral.
 * @tparam STN: DMA stream number.
 * @tparam CH: data channel of the DMA stream.
 * @tparam PL: priority level of the DMA stream.
 */
template < uint32_t DMA, uint8_t STN, uint8_t CH, uint8_t PL >
class DmaStream
{
public:

    /**
     * Initialize a DMA stream without starting it.
     *
     * @param irqPrio: priority of the DMA stream interrupts.
     * @param ds: data size for both source and destination locations.
     * @param memToPer: if set to true, transfer goes from memory to peripheral.
     * @return a StreamHandler object to manage the DMA stream.
     */
    static StreamHandler init(const uint8_t irqPrio, const DataSize ds,
                              const bool memToPer = false)
    {
        enableClock();

        uint32_t dir = memToPer ? DMA_SxCR_DIR_0 : 0;
        uint32_t TS  = static_cast < uint8_t >(ds);
        auto stream  = getStream();
        stream->CR   = (CH << 25)       // Channel
                     | (PL << 16)       // Priority
                     | (TS << 13)       // Source transfer size
                     | (TS << 11)       // Destination transfer size
                     | DMA_SxCR_MINC    // Increment source pointer
                     | dir              // Direction
                     | DMA_SxCR_TCIE    // Transfer complete interrupt
                     | DMA_SxCR_TEIE;   // Transfer error interrupt

        // Enable DMA interrupts
        IRQn_Type irq = IRQn();
        NVIC_ClearPendingIRQ(irq);
        NVIC_SetPriority(irq, irqPrio);
        NVIC_EnableIRQ(irq);

        return StreamHandler(stream, irq);
    }

    /**
     * Low-level shutdown of a DMA stream.
     */
    static inline void terminate()
    {
        NVIC_DisableIRQ(IRQn());
        getStream()->CR = 0;
    }

    /**
     * Stream IRQ handler, forwards the call to the handler inside a StreamHandler
     * class.
     *
     * @param hdl: pointer to the StreamHandler class managing the stream.
     */
    static inline void IRQhandleInterrupt(StreamHandler *hdl)
    {
        uint32_t flags = readIrqFlags();
        hdl->IRQhandler(flags);
        clearIrqFlags(flags);
    }

private:

    /**
     * Enable DMA AHB clock.
     */
    static inline constexpr void enableClock()
    {
        if(reinterpret_cast< DMA_TypeDef *>(DMA) == DMA1)
            RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
        else
            RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

        RCC_SYNC();
    }

    /**
     * Obtain pointer to the DMA stream peripheral given the DMA and the stream
     * number.
     */
    static inline constexpr DMA_Stream_TypeDef *getStream()
    {
        DMA_Stream_TypeDef *base;
        if(reinterpret_cast< DMA_TypeDef *>(DMA) == DMA1)
            base = DMA1_Stream0;
        else
            base = DMA2_Stream0;

        return base + STN;
    }

    /**
     * Clear the IRQ flags of a DMA stream according to a given mask.
     */
    static inline constexpr void clearIrqFlags(const uint32_t mask)
    {
        uint32_t shift  = (STN % 4) * 8;

        if(STN < 4)
            reinterpret_cast< DMA_TypeDef *>(DMA)->LIFCR = (mask << shift);
        else
            reinterpret_cast< DMA_TypeDef *>(DMA)->HIFCR = (mask << shift);
    }

    /**
     * Get the current IRQ flags of a DMA stream.
     */
    static inline constexpr uint32_t readIrqFlags()
    {
        uint32_t shift  = (STN % 4) * 8;
        uint32_t flags  = 0;

        if(STN < 4)
            flags = reinterpret_cast< DMA_TypeDef *>(DMA)->LISR;
        else
            flags = reinterpret_cast< DMA_TypeDef *>(DMA)->HISR;

        return (flags >> shift) & 0x7D;
    }

    /**
     * Get the IRQ number of a DMA stream.
     */
    static inline constexpr IRQn_Type IRQn()
    {
        //
        // NOTE: there is no better implementation than the one below because
        // DMA stream IRQ numbers are not contiguous...
        //
        if(reinterpret_cast< DMA_TypeDef *>(DMA) == DMA1)
        {
            switch(STN)
            {
                case 0: return DMA1_Stream0_IRQn; break;
                case 1: return DMA1_Stream1_IRQn; break;
                case 2: return DMA1_Stream2_IRQn; break;
                case 3: return DMA1_Stream3_IRQn; break;
                case 4: return DMA1_Stream4_IRQn; break;
                case 5: return DMA1_Stream5_IRQn; break;
                case 6: return DMA1_Stream6_IRQn; break;
                case 7: return DMA1_Stream7_IRQn; break;
            }
        }
        else
        {
            switch(STN)
            {
                case 0: return DMA2_Stream0_IRQn; break;
                case 1: return DMA2_Stream1_IRQn; break;
                case 2: return DMA2_Stream2_IRQn; break;
                case 3: return DMA2_Stream3_IRQn; break;
                case 4: return DMA2_Stream4_IRQn; break;
                case 5: return DMA2_Stream5_IRQn; break;
                case 6: return DMA2_Stream6_IRQn; break;
                case 7: return DMA2_Stream7_IRQn; break;
            }
        }

        return DMA1_Stream0_IRQn;
    }
};

#endif /* DMA_STREAM_H */

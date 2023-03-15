/***************************************************************************
 *   Copyright (C) 2022 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#include <kernel/scheduler/scheduler.h>
#include <audio_stream.h>
#include <toneGenerator_MDx.h>
#include <data_conversion.h>
#include <timers.h>
#include <miosix.h>

static int    priority     = PRIO_BEEP;
static bool   running      = false;   // Stream is running
static bool   circularMode = false;   // Circular mode enabled
static bool   reqFinish    = false;   // Pending termination request
static size_t bufLen       = 0;       // Buffer length
static stream_sample_t *bufAddr = 0;  // Start address of data buffer, fixed.
static stream_sample_t *idleBuf = 0;

using namespace miosix;
static Thread *dmaWaiting = 0;


/**
 * \internal
 * Stop an ongoing transfer, deactivating timers and DMA stream.
 */
static inline void stopTransfer()
{
    TIM7->CR1         = 0;                      // Stop TIM7
    DMA1_Stream2->CR &= ~DMA_SxCR_EN;           // Stop DMA transfer
    TIM3->CCER       &= ~TIM_CCER_CC3E;         // Turn off compare channel
    RCC->APB1ENR     &= ~RCC_APB1ENR_TIM7EN;    // Turn off TIM7 APB clock
    __DSB();

    // Re-activate "beeps"
    toneGen_unlockBeep();

    // Finally, clear flags and restore priority level
    running      = false;
    reqFinish    = false;
    circularMode = false;
    priority     = PRIO_BEEP;
}

/**
 * \internal
 * Actual implementation of DMA1 Stream2 interrupt handler.
 */
void __attribute__((used)) DMA_Handler()
{
    if(DMA1->LISR & DMA_LISR_HTIF2)
        idleBuf = bufAddr;
    else
        idleBuf = bufAddr + (bufLen / 2);

    // Stop transfer for linear buffer mode or pending termination request.
    if((circularMode == false) || (reqFinish == true))
    {
        stopTransfer();
    }

    // Clear interrupt flags
    DMA1->LIFCR =  DMA_LIFCR_CTCIF2
                |  DMA_LIFCR_CHTIF2
                |  DMA_LIFCR_CTEIF2;

    // Finally, wake up eventual pending threads
    if(dmaWaiting == 0) return;
    dmaWaiting->IRQwakeup();
    if(dmaWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    dmaWaiting = 0;
}

void __attribute__((naked)) DMA1_Stream2_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z11DMA_Handlerv");
    restoreContext();
}

streamId outputStream_start(const enum AudioSink destination,
                            const enum AudioPriority prio,
                            stream_sample_t * const buf,
                            const size_t length,
                            const enum BufMode mode,
                            const uint32_t sampleRate)
{
    // Sanity check
    if((buf == NULL) || (length == 0) || (sampleRate == 0)) return -1;

    // This device cannot sink to buffers
    if(destination == SINK_MCU) return -1;

    // Check if an output stream is already opened and, in case, handle priority.
    if(running)
    {
        if(prio < priority) return -1;          // Lower priority, reject.
        if(prio > priority) stopTransfer();     // Higher priority, takes over.
        while(running) ;                        // Same priority, wait.
    }

    // Thread-safe block: assign priority, set stream as running and lock "beeps"
    __disable_irq();
    priority = prio;
    running  = true;
    toneGen_lockBeep();
    __enable_irq();

    /*
     * Convert buffer elements from int16_t to unsigned 8 bit values, as
     * required by tone generator. Processing can be done in-place because the
     * API mandates that the function caller does not modify the buffer content
     * once this function has been called.
     */
    S16toU8(buf, length);
    bufAddr = buf;
    bufLen  = length;
    idleBuf = bufAddr + (bufLen / 2);

    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    __DSB();

    /*
     * Timebase for triggering of DMA transfers.
     * Bus frequency for TIM7 is 84MHz.
     */
    tim_setUpdateFreqency(TIM7, sampleRate, 84000000);
    TIM7->CNT  = 0;
    TIM7->EGR  = TIM_EGR_UG;
    TIM7->DIER = TIM_DIER_UDE;

    /*
     * DMA stream for sample transfer, destination is TIM3 CCR3
     */
    DMA1_Stream2->NDTR = length;
    DMA1_Stream2->PAR  = reinterpret_cast< uint32_t >(&(TIM3->CCR3));
    DMA1_Stream2->M0AR = reinterpret_cast< uint32_t >(buf);
    DMA1_Stream2->CR = DMA_SxCR_CHSEL_0       // Channel 1
                     | DMA_SxCR_PL            // Very high priority
                     | DMA_SxCR_MSIZE_0       // 16 bit source size
                     | DMA_SxCR_PSIZE_0       // 16 bit destination size
                     | DMA_SxCR_MINC          // Increment source pointer
                     | DMA_SxCR_DIR_0         // Memory to peripheral
                     | DMA_SxCR_TCIE          // Transfer complete interrupt
                     | DMA_SxCR_TEIE;         // Transfer error interrupt

    if(mode == BUF_CIRC_DOUBLE)
    {
        DMA1_Stream2->CR |= DMA_SxCR_CIRC     // Circular buffer mode
                         |  DMA_SxCR_HTIE;    // Half transfer interrupt
        circularMode = true;
    }

    DMA1_Stream2->CR |= DMA_SxCR_EN;           // Enable transfer

    // Enable DMA interrupts
    NVIC_ClearPendingIRQ(DMA1_Stream2_IRQn);
    NVIC_SetPriority(DMA1_Stream2_IRQn, 10);
    NVIC_EnableIRQ(DMA1_Stream2_IRQn);

    // Enable compare channel
    TIM3->CCR3 = buf[0];
    TIM3->CCER |= TIM_CCER_CC3E;
    TIM3->CR1  |= TIM_CR1_CEN;

    // Start timer for DMA transfer triggering
    TIM7->CR1 = TIM_CR1_CEN;

    return 0;
}

stream_sample_t *outputStream_getIdleBuffer(const streamId id)
{
    (void) id;

    if(!circularMode) return nullptr;

    return idleBuf;
}

bool outputStream_sync(const streamId id, const bool bufChanged)
{
    (void) id;

    if(circularMode && bufChanged)
    {
        stream_sample_t *ptr = outputStream_getIdleBuffer(id);
        S16toU8(ptr, bufLen/2);
    }

    // Enter in critical section until the end of the function
    FastInterruptDisableLock dLock;

    Thread *curThread = Thread::IRQgetCurrentThread();
    if((dmaWaiting != 0) && (dmaWaiting != curThread)) return false;
    dmaWaiting = curThread;

    do
    {
        Thread::IRQwait();
        {
            // Re-enable interrupts while waiting for IRQ
            FastInterruptEnableLock eLock(dLock);
            Thread::yield();
        }
    }
    while((dmaWaiting != 0) && (running == true));

    dmaWaiting = 0;

    return true;
}

void outputStream_stop(const streamId id)
{
    (void) id;

    reqFinish = true;
}

void outputStream_terminate(const streamId id)
{
    (void) id;

    __disable_irq();

    stopTransfer();

    DMA1->LIFCR =  DMA_LIFCR_CTCIF2
                |  DMA_LIFCR_CHTIF2
                |  DMA_LIFCR_CTEIF2;

    __enable_irq();
}

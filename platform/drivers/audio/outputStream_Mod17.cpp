/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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
#include <peripherals/gpio.h>
#include <data_conversion.h>
#include <hwconfig.h>
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
    // Stop DMA transfers
    DMA1_Stream5->CR = 0;
    DMA1_Stream6->CR = 0;

    TIM7->CR1    = 0;              // Shutdown timer
    DAC->SR      = 0;              // Clear status flags
    DAC->CR      = DAC_CR_EN1;     // Keep only channel 1 active
    DAC->DHR12R1 = 1365;           // Set channel 1 (RTX) to about 1.1V when idle

    // Clear flags and restore priority level
    running      = false;
    reqFinish    = false;
    circularMode = false;
    priority     = PRIO_BEEP;
}

/**
 * \internal
 * Actual implementation of DMA interrupt handler.
 */
void __attribute__((used)) DMA_Handler()
{
    // Manage half transfer interrupt
    if((DMA1->HISR & DMA_HISR_HTIF5) || (DMA1->HISR & DMA_HISR_HTIF6))
        idleBuf = bufAddr;
    else
        idleBuf = bufAddr + (bufLen / 2);

    // Stop transfer for linear buffer mode or pending termination request.
    if((circularMode == false) || (reqFinish == true))
    {
        stopTransfer();
    }

    // Clear interrupt flags for stream 5 and 6
    uint32_t mask = DMA_HISR_TEIF5
                  | DMA_HISR_TCIF5
                  | DMA_HISR_HTIF5
                  | DMA_HISR_TEIF6
                  | DMA_HISR_TCIF6
                  | DMA_HISR_HTIF6;

    DMA1->HIFCR = DMA1->HISR & mask;

    // Finally, wake up eventual pending threads
    if(dmaWaiting == 0) return;
    dmaWaiting->IRQwakeup();
    if(dmaWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    dmaWaiting = 0;
}

// DMA 1, Stream 5: data transfer for RTX sink
void __attribute__((used)) DMA1_Stream5_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z11DMA_Handlerv");
    restoreContext();
}

// DMA 1, Stream 6: data transfer for speaker sink
void __attribute__((used)) DMA1_Stream6_IRQHandler()
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
    __enable_irq();

    /*
     * Convert buffer elements from int16_t to unsigned 8 bit values, as
     * required by tone generator. Processing can be done in-place because the
     * API mandates that the function caller does not modify the buffer content
     * once this function has been called.
     */
    S16toU12(buf, length);
    bufAddr = buf;
    bufLen  = length;
    idleBuf = bufAddr + (bufLen / 2);

    // Configure GPIOs
    gpio_setMode(BASEBAND_TX, INPUT_ANALOG);    /* Baseband TX */
    gpio_setMode(AUDIO_SPK,   INPUT_ANALOG);    /* Spk output  */

    /*
     * Enable peripherals
     */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    RCC->APB1ENR |= RCC_APB1ENR_DACEN
                 |  RCC_APB1ENR_TIM7EN;
    __DSB();

    /*
     * Configure DAC and DMA stream
     */
    uint32_t circular = 0;
    if(mode == BUF_CIRC_DOUBLE)
    {
        circular = DMA_SxCR_CIRC     // Circular buffer mode
                 | DMA_SxCR_HTIE;    // Half transfer interrupt
        circularMode = true;
    }

    if(destination == SINK_RTX)
    {
        DAC->CR |= DAC_CR_DMAEN1     // Enable DMA mode
                |  DAC_CR_TSEL1_1    // TIM7 TRGO as trigger source
                |  DAC_CR_TEN1       // Enable trigger input
                |  DAC_CR_EN1;       // Enable DAC

        DMA1_Stream5->NDTR = length;
        DMA1_Stream5->PAR  = reinterpret_cast< uint32_t >(&(DAC->DHR12R1));
        DMA1_Stream5->M0AR = reinterpret_cast< uint32_t >(buf);
        DMA1_Stream5->CR = DMA_SxCR_CHSEL      // Channel 7
                         | DMA_SxCR_PL         // Very high priority
                         | DMA_SxCR_MSIZE_0    // 16 bit source size
                         | DMA_SxCR_PSIZE_0    // 16 bit destination size
                         | DMA_SxCR_MINC       // Increment source pointer
                         | DMA_SxCR_TCIE       // Transfer complete interrupt
                         | DMA_SxCR_TEIE       // Transfer error interrupt
                         | DMA_SxCR_DIR_0      // Memory to peripheral
                         | circular            // Circular mode
                         | DMA_SxCR_EN;        // Start transfer

        NVIC_ClearPendingIRQ(DMA1_Stream5_IRQn);
        NVIC_SetPriority(DMA1_Stream5_IRQn, 10);
        NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    }
    else
    {
        DAC->CR |= DAC_CR_DMAEN2     // Enable DMA mode
                |  DAC_CR_TSEL2_1    // TIM7 TRGO as trigger source
                |  DAC_CR_TEN2       // Enable trigger input
                |  DAC_CR_EN2;       // Enable DAC

        DMA1_Stream6->NDTR = length;
        DMA1_Stream6->PAR  = reinterpret_cast< uint32_t >(&(DAC->DHR12R2));
        DMA1_Stream6->M0AR = reinterpret_cast< uint32_t >(buf);
        DMA1_Stream6->CR = DMA_SxCR_CHSEL      // Channel 7
                         | DMA_SxCR_PL         // Very high priority
                         | DMA_SxCR_MSIZE_0    // 16 bit source size
                         | DMA_SxCR_PSIZE_0    // 16 bit destination size
                         | DMA_SxCR_MINC       // Increment source pointer
                         | DMA_SxCR_TCIE       // Transfer complete interrupt
                         | DMA_SxCR_TEIE       // Transfer error interrupt
                         | DMA_SxCR_DIR_0      // Memory to peripheral
                         | circular            // Circular mode
                         | DMA_SxCR_EN;        // Start transfer

        NVIC_ClearPendingIRQ(DMA1_Stream6_IRQn);
        NVIC_SetPriority(DMA1_Stream6_IRQn, 10);
        NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    }

    /*
     * TIM7 for conversion triggering via TIM7_TRGO, that is counter reload.
     * APB1 frequency is 42MHz but timer runs at 84MHz, tick rate is 1MHz,
     * reload register is configured based on desired sample rate.
     */
    tim_setUpdateFreqency(TIM7, sampleRate, 84000000);
    TIM7->CNT = 0;
    TIM7->EGR = TIM_EGR_UG;
    TIM7->CR2 = TIM_CR2_MMS_1;
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
        S16toU12(ptr, bufLen/2);
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

    FastInterruptDisableLock dLock;

    stopTransfer();

    DMA1->HIFCR = DMA_HIFCR_CTEIF5
                | DMA_HIFCR_CTCIF5
                | DMA_HIFCR_CHTIF5;

    DMA1->HIFCR = DMA_HIFCR_CTEIF6
                | DMA_HIFCR_CTCIF6
                | DMA_HIFCR_CHTIF6;
}

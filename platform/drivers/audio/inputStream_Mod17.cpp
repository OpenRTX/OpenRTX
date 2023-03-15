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
#include <toneGenerator_MDx.h>
#include <peripherals/gpio.h>
#include <hwconfig.h>
#include <stdbool.h>
#include <miosix.h>
#include <timers.h>

using namespace miosix;


static bool             inUse    = false;       // Flag to determine if the input stream is already open.
static Thread          *sWaiting = 0;           // Thread waiting on interrupt.
static stream_sample_t *bufAddr  = 0;           // Start address of data buffer, fixed.
static stream_sample_t *bufCurr  = 0;           // Buffer address to be returned to application.
static size_t           bufLen   = 0;           // Buffer length.
static uint8_t          bufMode  = BUF_LINEAR;  // Buffer management mode.

void __attribute__((used)) DmaHandlerImpl()
{
    if(DMA2->LISR & (DMA_LISR_TCIF2 | DMA_LISR_HTIF2))
    {
        switch(bufMode)
        {
            case BUF_LINEAR:
                // Finish, stop DMA and ADC
                DMA2_Stream2->CR  &= ~DMA_SxCR_EN;
                ADC2->CR2         &= ~ADC_CR2_ADON;
                break;

            case BUF_CIRC_DOUBLE:
                // Return half of the buffer but do not stop the DMA
                if(DMA2->LISR & DMA_LISR_HTIF2)
                    bufCurr = bufAddr;                   // Return first half
                else
                    bufCurr = bufAddr + (bufLen / 2);    // Return second half
                break;

            default:
                break;
        }

        // Wake up the thread
        if(sWaiting != 0)
        {
            sWaiting->IRQwakeup();
            Priority prio = sWaiting->IRQgetPriority();
            if(prio > Thread::IRQgetCurrentThread()->IRQgetPriority())
                Scheduler::IRQfindNextThread();
            sWaiting = 0;
        }
    }

    DMA2->LIFCR |= DMA_LIFCR_CTEIF2    // Clear transfer error flag (not handled)
                |  DMA_LIFCR_CHTIF2    // Clear half transfer flag
                |  DMA_LIFCR_CTCIF2;   // Clear transfer completed flag
}

void __attribute__((naked)) DMA2_Stream2_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z14DmaHandlerImplv");
    restoreContext();
}


streamId inputStream_start(const enum AudioSource source,
                           const enum AudioPriority prio,
                           stream_sample_t * const buf,
                           const size_t bufLength,
                           const enum BufMode mode,
                           const uint32_t sampleRate)
{
    (void) prio;    // TODO: input stream does not have priority

    // Check if buffer is in CCM area or not, since DMA cannot access CCM RAM
    if(reinterpret_cast< uint32_t >(buf) < 0x20000000) return -1;

   /*
    * Critical section for inUse flag management, makes the code below
    * thread-safe.
    */
    {
        FastInterruptDisableLock dLock;
        if(inUse) return -1;
        inUse = true;
    }

    bufMode = mode;
    bufAddr = buf;
    bufLen  = bufLength;

    RCC->APB2ENR |= RCC_APB2ENR_ADC2EN;    // Enable ADC
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;    // Enable conv. timebase timer
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;    // Enable DMA
    __DSB();

    /*
     * TIM2 for conversion triggering via TIM2_TRGO, that is counter reload.
     * AP1 frequency is 42MHz but timer runs at 84MHz, tick rate is 1MHz,
     * reload register is configured based on desired sample rate.
     */
    tim_setUpdateFreqency(TIM2, sampleRate, 84000000);

    TIM2->CNT = 0;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->CR2 = TIM_CR2_MMS_1;
    TIM2->CR1 = TIM_CR1_CEN;

    /* DMA2 Stream 2 common configuration:
     * - channel 1: ADC2
     * - high priority
     * - half-word transfer, both memory and peripheral
     * - increment memory
     * - peripheral-to-memory transfer
     */
    DMA2_Stream2->PAR  = reinterpret_cast< uint32_t >(&(ADC2->DR));
    DMA2_Stream2->M0AR = reinterpret_cast< uint32_t >(buf);
    DMA2_Stream2->NDTR = bufLength;
    DMA2_Stream2->CR = DMA_SxCR_CHSEL_0     // Channel 1
                     | DMA_SxCR_MSIZE_0     // Memory size: 16 bit
                     | DMA_SxCR_PSIZE_0     // Peripheral size: 16 bit
                     | DMA_SxCR_PL_1        // High priority
                     | DMA_SxCR_MINC;       // Increment memory

    /*
     * Configure DMA and memory pointers according to buffer management mode.
     * In linear and circular mode all the buffer is returned, in double circular
     * buffer mode the buffer pointer is managed inside the DMA ISR.
     */
    switch(mode)
    {
        case BUF_LINEAR:
            DMA2_Stream2->CR |= DMA_SxCR_TCIE;  // Interrupt on transfer end
            bufCurr = bufAddr;                  // Return all the buffer
            break;

        case BUF_CIRC_DOUBLE:
            DMA2_Stream2->CR |= DMA_SxCR_CIRC   // Circular mode
                             |  DMA_SxCR_HTIE   // Interrupt on half transfer
                             |  DMA_SxCR_TCIE;  // Interrupt on transfer end
            break;

        default:
            inUse = false;    // Invalid setting, release flag and return error.
            return -1;
            break;
    }

    // Configure NVIC interrupt
    NVIC_ClearPendingIRQ(DMA2_Stream2_IRQn);
    NVIC_SetPriority(DMA2_Stream2_IRQn, 10);
    NVIC_EnableIRQ(DMA2_Stream2_IRQn);

    /*
     * ADC2 configuration.
     *
     * ADC clock is APB2 frequency divided by 4, giving 21MHz.
     * Channel sample time set to 84 cycles, total conversion time is 100
     * cycles: this leads to a maximum sampling frequency of 210kHz.
     * Convert one channel only, no overrun interrupt, 12-bit resolution,
     * no analog watchdog, discontinuous mode, no end of conversion interrupts.
     */
    ADC->CCR   |= ADC_CCR_ADCPRE_0;
    ADC2->SMPR2 = ADC_SMPR2_SMP2_2
                | ADC_SMPR2_SMP1_2;
    ADC2->SQR1  = 0;                // Convert one channel
    ADC2->CR1 |= ADC_CR1_DISCEN;
    ADC2->CR2 |= ADC_CR2_EXTEN_0    // Trigger on rising edge
              |  ADC_CR2_EXTSEL_1
              |  ADC_CR2_EXTSEL_2   // 0b0110 TIM2_TRGO trig. source
              |  ADC_CR2_DDS        // Enable DMA data transfer
              |  ADC_CR2_DMA;

    /*
     * Select ADC channel according to signal source:
     * - CH1, audio from RTX on PA1
     * - CH2, audio from microphone on PA2
     */
    switch(source)
    {
        case SOURCE_MIC:
            gpio_setMode(AUDIO_MIC, INPUT_ANALOG);
            ADC2->SQR3 = 2;
            break;

        case SOURCE_RTX:
            gpio_setMode(BASEBAND_RX, INPUT_ANALOG);
            ADC2->SQR3 = 1;
            break;

        default:
            inUse = false;    //  Unsupported source, release flag and return error.
            return -1;
            break;
    }

    if(mode == BUF_CIRC_DOUBLE)
    {
        DMA2_Stream2->CR |= DMA_SxCR_EN;    // Enable DMA
        ADC2->CR2        |= ADC_CR2_ADON;   // Enable ADC
    }

    return 0;
}

dataBlock_t inputStream_getData(streamId id)
{
    dataBlock_t block;
    block.data = NULL;
    block.len  = 0;

    // Invalid stream ID, return an empty data block
    if(id < 0) return block;

    if(bufMode == BUF_LINEAR)
    {
        // Reload DMA configuration then start DMA and ADC, stopped in ISR
        DMA2_Stream2->PAR  = reinterpret_cast< uint32_t >(&(ADC2->DR));
        DMA2_Stream2->M0AR = reinterpret_cast< uint32_t >(bufAddr);
        DMA2_Stream2->NDTR = bufLen;
        DMA2_Stream2->CR  |= DMA_SxCR_EN;
        ADC2->CR2         |= ADC_CR2_ADON;
    }

    /*
     * Put the calling thread in waiting status until data is ready.
     */
    {
        FastInterruptDisableLock dLock;
        sWaiting = Thread::IRQgetCurrentThread();
        do
        {
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }

        }while((sWaiting != 0) && (inUse == true));
    }

    block.data = bufCurr;
    block.len  = bufLen;
    if(bufMode == BUF_CIRC_DOUBLE) block.len /= 2;

    return block;
}

void inputStream_stop(streamId id)
{
    if(id < 0) return;

    TIM2->CR1        = 0;   // Shut down timebase
    ADC2->CR2        = 0;   // Shut down ADC
    DMA2_Stream2->CR = 0;   // Shut down DMA transfer

    RCC->APB2ENR &= ~RCC_APB2ENR_ADC2EN;    // Disable ADC
    RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN;    // Disable conv. timebase timer
    RCC->AHB1ENR &= ~RCC_AHB1ENR_DMA2EN;    // Disable DMA
    __DSB();

    // Critical section: release inUse flag and invalidate (partial) data.
    // Releasing the "inUse" flag cause the wake up of pending threads.
    FastInterruptDisableLock dLock;
    bufCurr = 0;
    bufLen  = 0;
    inUse   = false;
}

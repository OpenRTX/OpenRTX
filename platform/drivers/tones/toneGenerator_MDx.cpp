/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "toneGenerator_MDx.h"
#include <miosix.h>
#include <hwconfig.h>
#include <interfaces/gpio.h>
#include <kernel/scheduler/scheduler.h>

/*
 * Sine table for PWM-based sinewave generation, containing 256 samples over one
 * period of a 35Hz sinewave. This gives a PWM base frequency of 8.96kHz.
 */
static const uint8_t sineTable[] =
{
    128,131,134,137,140,143,146,149,152,155,158,162,165,167,170,173,176,179,182,
    185,188,190,193,196,198,201,203,206,208,211,213,215,218,220,222,224,226,228,
    230,232,234,235,237,238,240,241,243,244,245,246,248,249,250,250,251,252,253,
    253,254,254,254,255,255,255,255,255,255,255,254,254,254,253,253,252,251,250,
    250,249,248,246,245,244,243,241,240,238,237,235,234,232,230,228,226,224,222,
    220,218,215,213,211,208,206,203,201,198,196,193,190,188,185,182,179,176,173,
    170,167,165,162,158,155,152,149,146,143,140,137,134,131,128,124,121,118,115,
    112,109,106,103,100,97,93,90,88,85,82,79,76,73,70,67,65,62,59,57,54,52,49,47,
    44,42,40,37,35,33,31,29,27,25,23,21,20,18,17,15,14,12,11,10,9,7,6,5,5,4,3,2,
    2,1,1,1,0,0,0,0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,10,11,12,14,15,17,18,20,21,23,
    25,27,29,31,33,35,37,40,42,44,47,49,52,54,57,59,62,65,67,70,73,76,79,82,85,
    88,90,93,97,100,103,106,109,112,115,118,121,124
};

static const uint32_t baseSineFreq = 35;

uint32_t toneTableIndex = 0; // Current sine table index for CTCSS generator
uint32_t toneTableIncr  = 0; // CTCSS sine table index increment per tick

uint32_t beepTableIndex = 0; // Current sine table index for "beep" generator
uint32_t beepTableIncr  = 0; // "beep" sine table index increment per tick
uint32_t beepTimerCount = 0; // Downcounter for timed "beep"
uint8_t  beepVolume     = 0; // "beep" volume level

bool tonesLocked = false;    // If true tone channel is in use by FSK/playback

using namespace miosix;
Thread *dmaWaiting = 0;

/*
 * TIM14 interrupt handler, used to manage generation of CTCSS and "beep" tones.
 */
void __attribute__((used)) TIM8_TRG_COM_TIM14_IRQHandler()
{
    TIM14->SR = 0;

    toneTableIndex += toneTableIncr;
    beepTableIndex += beepTableIncr;

    TIM3->CCR2 = sineTable[(toneTableIndex >> 16) & 0xFF];

    if(!tonesLocked)
    {
        TIM3->CCR3 = (sineTable[(beepTableIndex >> 16) & 0xFF] * beepVolume) >> 8;
    }

    if(beepTimerCount > 0)
    {
        beepTimerCount--;
        if(beepTimerCount == 0)
        {
            TIM3->CCER &= ~TIM_CCER_CC3E;
        }
    }

    // Shutdown timers if both compare channels are inactive
    if((TIM3->CCER & (TIM_CCER_CC2E | TIM_CCER_CC3E)) == 0)
    {
        TIM3->CR1  &= ~TIM_CR1_CEN;
        TIM14->CR1 &= ~TIM_CR1_CEN;
    }
}

/*
 * DMA1 Stream2 interrupt handler, for audio playback and FSK modulation.
 */
void __attribute__((used)) DMA_Handler()
{
    DMA1->LIFCR |= DMA_LIFCR_CTCIF2         // Clear interrupt flags
                | DMA_LIFCR_CTEIF2;

    TIM7->CR1   = 0;                        // End of transfer, stop TIM7
    TIM3->CCER &= ~TIM_CCER_CC3E;           // Turn off compare channel

    RCC->AHB1ENR &= ~RCC_AHB1ENR_DMA1EN;    // Turn off DMA
    RCC->APB1ENR &= ~RCC_APB1ENR_TIM7EN;    // Turn off TIM7
    __DSB();

    tonesLocked = false;                    // Finally, unlock tones

    if(dmaWaiting == 0) return;             // Wake up eventual pending threads
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

void toneGen_init()
{
    /*
     * Configure GPIOs:
     * - CTCSS output is on PC7 (on MD380), that is TIM3-CH2, AF2
     * - "beep" output is on PC8 (on MD380), that is TIM3-CH3, AF2
     */
    gpio_setMode(CTCSS_OUT, ALTERNATE);
    gpio_setMode(BEEP_OUT,  ALTERNATE);
    gpio_setAlternateFunction(CTCSS_OUT, 2);
    gpio_setAlternateFunction(BEEP_OUT,  2);

    /*
     * TIM3 configuration:
     * - APB1 frequency = 42MHz but timer run at twice of this frequency: with
     *   1:3 prescaler we have Ftick = 28MHz
     * - ARR = 255 (8-bit PWM), gives a PWM frequency of 109.375kHz
     */
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    __DSB();

    TIM3->ARR   = 0xFF;
    TIM3->PSC   = 2;
    TIM3->CCMR1 = TIM_CCMR1_OC2M_2  // CH2 in PWM mode 1, preload enabled
                | TIM_CCMR1_OC2M_1
                | TIM_CCMR1_OC2PE;
    TIM3->CCMR2 = TIM_CCMR2_OC3M_2  // The same for CH3
                | TIM_CCMR2_OC3M_1
                | TIM_CCMR2_OC3PE;
    TIM3->CR1  |= TIM_CR1_ARPE;     // Enable auto preload on reload

    /*
     * TIM14 configuration:
     * - APB1 frequency = 42MHz but timer run at twice of this frequency.
     * - ARR = 9375 gives an update rate of 8.96kHz
     */
    RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
    __DSB();

    TIM14->PSC  = 0;                /* 1:1 prescaler               */
    TIM14->ARR  = 9375;
    TIM14->CNT  = 0;
    TIM14->EGR  = TIM_EGR_UG;       /* Update registers            */
    TIM14->DIER = TIM_DIER_UIE;     /* Interrupt on counter update */

    NVIC_SetPriority(TIM8_TRG_COM_TIM14_IRQn, 10);
    NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);
}

void toneGen_terminate()
{
    RCC->APB1ENR &= ~(RCC_APB1ENR_TIM3EN  |
                      RCC_APB1ENR_TIM14EN |
                      RCC_APB1ENR_TIM7EN);
    RCC->AHB1ENR &= ~RCC_AHB1ENR_DMA1EN;
    __DSB();

    dmaWaiting->wakeup();

    gpio_setMode(CTCSS_OUT, INPUT);
    gpio_setMode(BEEP_OUT,  INPUT);
}

void toneGen_setToneFreq(float toneFreq)
{
    /*
     * Convert to 16.16 fixed point number, then divide by  the frequency of
     * sinewave stored in the PWM table
     */
    float dividend = toneFreq * 65536.0f;
    toneTableIncr = ((uint32_t) dividend)/baseSineFreq;
}

void toneGen_toneOn()
{
    TIM3->CCER |= TIM_CCER_CC2E;
    TIM3->CR1  |= TIM_CR1_CEN;
    TIM14->CR1 |= TIM_CR1_CEN;
}

void toneGen_toneOff()
{
    TIM3->CCER &= ~TIM_CCER_CC2E;
}

void toneGen_beepOn(const float beepFreq, const uint8_t volume,
                    const uint32_t duration)
{
    {
        // Do not generate "beep" if the PWM channel is busy, critical section
        FastInterruptDisableLock dLock;
        if(tonesLocked) return;
    }

    float dividend = beepFreq * 65536.0f;
    beepTableIncr  = ((uint32_t) dividend)/baseSineFreq;
    beepVolume     = volume;

    /*
     * Duration is in milliseconds, while counter update rate is 8.96kHz.
     * Thus, the value for downcounter is (duration * 8960)/1000.
     */
    beepTimerCount = (duration * 8960)/1000;

    TIM3->CCER |= TIM_CCER_CC3E;
    TIM3->CR1  |= TIM_CR1_CEN;
    TIM14->CR1 |= TIM_CR1_CEN;
}

void toneGen_beepOff()
{
    /*
     * Prevent disabling of tones if PWM channel is occupied by FSK/playback.
     * Locking interrupts to avoid race conditions.
     */
    FastInterruptDisableLock dLock;
    if(tonesLocked) return;
    TIM3->CCER &= ~TIM_CCER_CC3E;
}

void toneGen_encodeAFSK1200(const uint8_t* buf, const size_t len)
{
    (void) buf;
    (void) len;
}

void toneGen_playAudioStream(const uint16_t* buf, const size_t len,
                             const uint32_t sampleRate)
{
    if((buf == NULL) || (len == 0) || (sampleRate == 0)) return;

    {
        // Critical section to avoid race conditions on "tonesLocked"
        FastInterruptDisableLock dLock;
        tonesLocked    = true;
        beepTimerCount = 0;
    }

    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    __DSB();

    /*
     * Timebase for triggering of DMA transfers.
     * Bus frequency is 84MHz.
     */
    uint32_t ratio = (84000000/sampleRate) - 1;

    TIM7->CNT  = 0;
    TIM7->PSC  = 0;
    TIM7->ARR  = ratio;
    TIM7->EGR  = TIM_EGR_UG;
    TIM7->DIER = TIM_DIER_UDE;

    /*
     * DMA stream for sample transfer
     */
    DMA1_Stream2->NDTR = len;
    DMA1_Stream2->PAR  = ((uint32_t) &(TIM3->CCR3));
    DMA1_Stream2->M0AR = ((uint32_t) buf);
    DMA1_Stream2->CR = DMA_SxCR_CHSEL_0       // Channel 1
                     | DMA_SxCR_PL            // Very high priority
                     | DMA_SxCR_MSIZE_0       // 16 bit source size
                     | DMA_SxCR_PSIZE_0       // 16 bit destination size
                     | DMA_SxCR_MINC          // Increment source pointer
                     | DMA_SxCR_DIR_0         // Memory to peripheral
                     | DMA_SxCR_TCIE          // Transfer complete interrupt
                     | DMA_SxCR_TEIE          // Transfer error interrupt
                     | DMA_SxCR_EN;           // Enable transfer

    NVIC_ClearPendingIRQ(DMA1_Stream2_IRQn);
    NVIC_SetPriority(DMA1_Stream2_IRQn, 10);
    NVIC_EnableIRQ(DMA1_Stream2_IRQn);

    // Enable compare channel
    TIM3->CCR3 = 0;
    TIM3->CCER |= TIM_CCER_CC3E;
    TIM3->CR1  |= TIM_CR1_CEN;

    // Start timer for DMA transfer triggering
    TIM7->CR1 = TIM_CR1_CEN;
}

bool toneGen_waitForStreamEnd()
{
    /*
     * Critical section. Check if:
     *  - there is no transfer in progress, in which case waiting is useless.
     *  - there is another thread pending on transfer completion.
     *
     * If both these conditions are false, put the calling thread in waiting
     * status until transfer completes.
     */
    {
        FastInterruptDisableLock dLock;
        Thread *curThread = Thread::IRQgetCurrentThread();
        if(tonesLocked == false) return false;
        if((dmaWaiting != 0) && (dmaWaiting != curThread)) return false;
        dmaWaiting = curThread;
        do
        {
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        } while(dmaWaiting);
    }

    // Outside critical section, here we finished waiting and everything is ok.
    return true;
}

void toneGen_stopAudioStream()
{
    // Critical section to avoid race conditions
    FastInterruptDisableLock dLock;

    // Stop DMA triggering timer and TIM3 compare channel
    TIM7->CR1   = 0;
    TIM3->CCER &= ~TIM_CCER_CC3E;

    // Shut down TIM7 and DMA
    RCC->AHB1ENR &= ~RCC_AHB1ENR_DMA1EN;
    RCC->APB1ENR &= ~RCC_APB1ENR_TIM7EN;

    // Unlock tones and wake up the thread waiting for completion
    tonesLocked = false;
    if(dmaWaiting) dmaWaiting->IRQwakeup();
}

bool toneGen_toneBusy()
{
    /*
     * Tone section is busy whenever CC3E bit in TIM3 CCER register is set.
     * Lock interrupts before reading the register to avoid race conditions.
     */
    FastInterruptDisableLock dLock;
    return (TIM3->CCER & TIM_CCER_CC3E) ? true : false;
}

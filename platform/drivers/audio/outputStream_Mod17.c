/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <interfaces/audio_stream.h>
#include <interfaces/gpio.h>
#include <hwconfig.h>
#include <stdbool.h>

int  priority = PRIO_BEEP;
bool running  = false;

void stopTransfer()
{
    /* Shutdown timer */
    TIM7->CR1 &= ~TIM_CR1_CEN;

    /* Disable DAC channels and clear underrun flags */
    DAC->CR   &= ~(DAC_CR_EN1 | DAC_CR_EN2);
    DAC->SR   |= DAC_SR_DMAUDR1 | DAC_SR_DMAUDR2;

    /* Stop DMA transfers */
    DMA1_Stream5->CR &= ~DMA_SxCR_EN;
    DMA1_Stream6->CR &= ~DMA_SxCR_EN;

    running = false;
}

/*
 * DMA 1, Stream 5: data transfer for RTX sink
 */
void __attribute__((used)) DMA1_Stream5_IRQHandler()
{
    stopTransfer();

    DMA1->HIFCR |= DMA_HIFCR_CTCIF5
                |  DMA_HIFCR_CTEIF5;

    NVIC_DisableIRQ(DMA1_Stream5_IRQn);
}

/*
 * DMA 1, Stream 6: data transfer for speaker sink
 */
void __attribute__((used)) DMA1_Stream6_IRQHandler()
{
    stopTransfer();

    DMA1->HIFCR |= DMA_HIFCR_CTCIF6
                |  DMA_HIFCR_CTEIF6;

    NVIC_DisableIRQ(DMA1_Stream6_IRQn);
}

streamId outputStream_start(const enum AudioSink destination,
                            const enum AudioPriority prio,
                            stream_sample_t * const buf,
                            const size_t length,
                            const uint32_t sampleRate)
{
    if(destination == SINK_MCU) return -1;     /* This device cannot sink to buffer             */
    if(running)                                /* Check if a stream is already running          */
    {
        if(prio < priority) return -1;         /* Requested priority is lower than current      */
        if(prio > priority) stopTransfer();    /* Stop an ongoing stream with lower priority    */
        while(running) ;                       /* Same priority, wait for current stream to end */
    }

    /* This assigment must be thread-safe */
    __disable_irq();
    priority = prio;
    running  = true;
    __enable_irq();

    /*
     * Convert buffer elements from int16_t to unsigned 16 bit values ranging
     * from 0 to 4096, as required by DAC. Processing can be done in-place
     * because the API mandates that the function caller does not modify the
     * buffer content once this function has been called. Code below exploits
     * Cortex M4 SIMD instructions for fast execution.
     */
    uint32_t *data = ((uint32_t *) buf);
    for(size_t i = 0; i < length/2; i++)
    {
        uint32_t value = __SADD16(data[i], 0x80008000);
        data[i]        = (value >> 4) & 0x0FFF0FFF;
    }

    /* Handle last element in case of odd buffer length */
    if((length % 2) != 0)
    {
        int16_t value   = buf[length - 1] + 32768;
        buf[length - 1] = ((uint16_t) value) >> 4;
    }

    /* Configure GPIOs */
    gpio_setMode(GPIOA, 4, INPUT_ANALOG);    /* Baseband TX */
    gpio_setMode(GPIOA, 5, INPUT_ANALOG);    /* Spk output  */

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
    if(destination == SINK_RTX)
    {
        DAC->CR = DAC_CR_DMAEN1     /* Enable DMA mode             */
                | DAC_CR_TSEL1_1    /* TIM7 TRGO as trigger source */
                | DAC_CR_TEN1       /* Enable trigger input        */
                | DAC_CR_EN1;       /* Enable DAC                  */

        DMA1_Stream5->NDTR = length;
        DMA1_Stream5->PAR  = ((uint32_t) &(DAC->DHR12R1));
        DMA1_Stream5->M0AR = ((uint32_t) buf);
        DMA1_Stream5->CR = DMA_SxCR_CHSEL      /* Channel 7                   */
                         | DMA_SxCR_PL         /* Very high priority          */
                         | DMA_SxCR_MSIZE_0    /* 16 bit source size          */
                         | DMA_SxCR_PSIZE_0    /* 16 bit destination size     */
                         | DMA_SxCR_MINC       /* Increment source pointer    */
                         | DMA_SxCR_TCIE       /* Transfer complete interrupt */
                         | DMA_SxCR_TEIE       /* Transfer error interrupt    */
                         | DMA_SxCR_DIR_0      /* Memory to peripheral        */
                         | DMA_SxCR_EN;        /* Start transfer              */

        NVIC_ClearPendingIRQ(DMA1_Stream5_IRQn);
        NVIC_SetPriority(DMA1_Stream5_IRQn, 10);
        NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    }
    else
    {
        DAC->CR = DAC_CR_DMAEN2     /* Enable DMA mode             */
                | DAC_CR_TSEL2_1    /* TIM7 TRGO as trigger source */
                | DAC_CR_TEN2       /* Enable trigger input        */
                | DAC_CR_EN2;       /* Enable DAC                  */

        DMA1_Stream6->NDTR = length;
        DMA1_Stream6->PAR  = ((uint32_t) &(DAC->DHR12R2));
        DMA1_Stream6->M0AR = ((uint32_t) buf);
        DMA1_Stream6->CR = DMA_SxCR_CHSEL      /* Channel 7                   */
                         | DMA_SxCR_PL         /* Very high priority          */
                         | DMA_SxCR_MSIZE_0    /* 16 bit source size          */
                         | DMA_SxCR_PSIZE_0    /* 16 bit destination size     */
                         | DMA_SxCR_MINC       /* Increment source pointer    */
                         | DMA_SxCR_TCIE       /* Transfer complete interrupt */
                         | DMA_SxCR_TEIE       /* Transfer error interrupt    */
                         | DMA_SxCR_DIR_0      /* Memory to peripheral        */
                         | DMA_SxCR_EN;        /* Start transfer              */

        NVIC_ClearPendingIRQ(DMA1_Stream6_IRQn);
        NVIC_SetPriority(DMA1_Stream6_IRQn, 10);
        NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    }

    /*
     * TIM7 for conversion triggering via TIM7_TRGO, that is counter reload.
     * AP1 frequency is 42MHz but timer runs at 84MHz, tick rate is 1MHz,
     * reload register is configured based on desired sample rate.
     */
    TIM7->PSC = 83;
    TIM7->ARR = (1000000/sampleRate) - 1;
    TIM7->CNT = 0;
    TIM7->EGR = TIM_EGR_UG;
    TIM7->CR2 = TIM_CR2_MMS_1;
    TIM7->CR1 = TIM_CR1_CEN;

    return 0;
}

void outputStream_stop(streamId id)
{
    (void) id;

    if(!running) return;

    stopTransfer();
}

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

#include <kernel/scheduler/scheduler.h>
#include <DmaStream.hpp>
#include <Timer.hpp>
#include <miosix.h>
#include <errno.h>
#include "stm32_adc.h"

struct AdcPeriph
{
    ADC_TypeDef   *adc;
    StreamHandler *stream;
    Timer          tim;
};

using Dma2_Stream0 = DmaStream< DMA2_BASE, 0, 0, 2 >; // DMA 2, Stream 2, channel 0, high priority (ADC1)
using Dma2_Stream1 = DmaStream< DMA2_BASE, 1, 2, 2 >; // DMA 2, Stream 2, channel 2, high priority (ADC3)
using Dma2_Stream2 = DmaStream< DMA2_BASE, 2, 1, 2 >; // DMA 2, Stream 2, channel 1, high priority (ADC2)

StreamHandler Dma2_Stream0_hdl = Dma2_Stream0::init(10, DataSize::_16BIT, false);
StreamHandler Dma2_Stream1_hdl = Dma2_Stream1::init(10, DataSize::_16BIT, false);
StreamHandler Dma2_Stream2_hdl = Dma2_Stream2::init(10, DataSize::_16BIT, false);

static struct streamCtx *AdcContext[3];

static constexpr AdcPeriph periph[] =
{
    {(ADC_TypeDef *) ADC1_BASE, &Dma2_Stream0_hdl, Timer(TIM2_BASE)},
    {(ADC_TypeDef *) ADC2_BASE, &Dma2_Stream2_hdl, Timer(TIM2_BASE)},
    {(ADC_TypeDef *) ADC3_BASE, &Dma2_Stream1_hdl, Timer(TIM2_BASE)},
};


/**
 * \internal
 * Stop an ongoing transfer, deactivating timers and DMA stream.
 */
static void stopTransfer(const uint8_t instNum)
{
    periph[instNum].tim.stop();
    periph[instNum].adc->CR2 &= ~ADC_CR2_ADON;
    AdcContext[instNum]->running = 0;
}

/**
 * \internal
 * Actual implementation of DMA interrupt handler.
 */
void __attribute__((used)) ADC_DMA_Handler(uint32_t instNum)
{
    switch(instNum)
    {
        case 0:
            Dma2_Stream0::IRQhandleInterrupt(periph[instNum].stream);
            break;

        case 1:
            Dma2_Stream2::IRQhandleInterrupt(periph[instNum].stream);
            break;

        case 2:
            Dma2_Stream1::IRQhandleInterrupt(periph[instNum].stream);
            break;
    }
}

// DMA 2, Stream 0: data transfer for ADC1
void __attribute__((used)) DMA2_Stream0_IRQHandler()
{
    saveContext();
    asm volatile("mov r0, #0");
    asm volatile("bl _Z15ADC_DMA_Handlerm");
    restoreContext();
}

// DMA 2, Stream 2: data transfer for ADC2
void __attribute__((used)) DMA2_Stream2_IRQHandler()
{
    saveContext();
    asm volatile("mov r0, #1");
    asm volatile("bl _Z15ADC_DMA_Handlerm");
    restoreContext();
}

// DMA 2, Stream 1: data transfer for ADC3
void __attribute__((used)) DMA2_Stream1_IRQHandler()
{
    saveContext();
    asm volatile("mov r0, #2");
    asm volatile("bl _Z15ADC_DMA_Handlerm");
    restoreContext();
}



void stm32adc_init(const uint8_t instance)
{
    // Enable peripherals
    switch(instance)
    {
        case 0:
            RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
            break;

        case 1:
            RCC->APB2ENR |= RCC_APB2ENR_ADC2EN;
            break;

        case 2:
            RCC->APB2ENR |= RCC_APB2ENR_ADC3EN;
            break;
    }

    /*
     * Use TIM2 as trigger source for all the ADCs.
     * TODO: change this with a dedicated timer for each ADC.
     */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    uint32_t adcTrig = ADC_CR2_EXTSEL_1     // 0b0110 TIM2_TRGO trig. source
                     | ADC_CR2_EXTSEL_2;

    __DSB();

    /*
     * ADC configuration.
     *
     * ADC clock is APB2 frequency divided by 4, giving 21MHz.
     * Channel sample time set to 84 cycles, total conversion time is 100
     * cycles: this leads to a maximum sampling frequency of 210kHz.
     * Convert one channel only, no overrun interrupt, 12-bit resolution,
     * no analog watchdog, discontinuous mode, no end of conversion interrupts.
     */
    ADC_TypeDef *adc = periph[instance].adc;

    ADC->CCR  |= ADC_CCR_ADCPRE_0;
    adc->SMPR2 = ADC_SMPR2_SMP2_2
               | ADC_SMPR2_SMP1_2;
    adc->SQR1  = 0;                 // Convert one channel
    adc->CR1  |= ADC_CR1_DISCEN;
    adc->CR2  |= ADC_CR2_EXTEN_0    // Trigger on rising edge
              |  adcTrig            // Trigger source
              |  ADC_CR2_DDS        // Continuous DMA requests
              |  ADC_CR2_DMA;       // Enable DMA data transfer

    // Register end-of-transfer callbacks
    periph[instance].stream->setEndTransferCallback(std::bind(stopTransfer, instance));
}

void stm32adc_terminate()
{
    // Terminate streams before shutting of the peripherals
    for(int i = 0; i < 2; i++)
    {
        if(AdcContext[i] != NULL)
        {
            if(AdcContext[i]->running != 0)
                periph[i].stream->halt();
        }
    }

    // TODO: turn off peripherals
    RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN;
    __DSB();
}

static int stm32adc_start(const uint8_t instance, const void *config, struct streamCtx *ctx)
{
    if(ctx == NULL)
        return -EINVAL;

    if((ctx->running != 0) || (periph[instance].stream->running()))
        return -EBUSY;

    __disable_irq();
    ctx->running = 1;
    __enable_irq();

    ctx->priv = (void *) &periph[instance];
    AdcContext[instance] = ctx;

    // Set conversion channel and enable the ADC
    periph[instance].adc->SQR3 = (uint32_t) config;
    periph[instance].adc->CR2 |= ADC_CR2_ADON;

    // Start DMA stream
    bool circ = false;
    if(ctx->bufMode == BUF_CIRC_DOUBLE)
        circ = true;

    periph[instance].stream->start(&(periph[instance].adc->DR), ctx->buffer,
                                   ctx->bufSize, circ);

    // Configure ADC trigger
    periph[instance].tim.setUpdateFrequency(ctx->sampleRate);
    periph[instance].tim.start();

    return 0;
}

static int stm32adc_data(struct streamCtx *ctx, stream_sample_t **buf)
{
    AdcPeriph *p = reinterpret_cast< AdcPeriph * >(ctx->priv);

    if(ctx->bufMode == BUF_CIRC_DOUBLE)
    {
        *buf = reinterpret_cast< stream_sample_t *>(p->stream->idleBuf());
        return ctx->bufSize/2;
    }

    // Linear mode: return the full buffer
    *buf = ctx->buffer;
    return ctx->bufSize;
}

static int stm32adc_sync(struct streamCtx *ctx, uint8_t dirty)
{
    (void) dirty;

    AdcPeriph *p = reinterpret_cast< AdcPeriph * >(ctx->priv);
    bool ok = p->stream->sync();
    if(ok)
        return 0;

   return -1;
}

static void stm32adc_stop(struct streamCtx *ctx)
{
    if(ctx->running == 0)
        return;

    reinterpret_cast< AdcPeriph * >(ctx->priv)->stream->stop();
}

static void stm32adc_halt(struct streamCtx *ctx)
{
    if(ctx->running == 0)
        return;

    reinterpret_cast< AdcPeriph * >(ctx->priv)->stream->halt();
}

#pragma GCC diagnostic ignored "-Wpedantic"
const struct audioDriver stm32_adc_audio_driver =
{
    .start     = stm32adc_start,
    .data      = stm32adc_data,
    .sync      = stm32adc_sync,
    .stop      = stm32adc_stop,
    .terminate = stm32adc_halt
};
#pragma GCC diagnostic pop

/***************************************************************************
 *   Copyright (C) 2023 - 2024 by Federico Amedeo Izzo IU2NUO,             *
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
#include <peripherals/gpio.h>
#include <data_conversion.h>
#include <DmaStream.hpp>
#include <Timer.hpp>
#include <miosix.h>
#include <errno.h>
#include "stm32_dac.h"

struct ChannelState
{
    struct streamCtx  *ctx;         // Current stream context
    uint32_t          idleLevel;    // Output idle level
    StreamHandler     stream;       // DMA stream handler
};

struct DacChannel
{
    volatile uint32_t  *dacReg;     // DAC data register
    Timer               tim;        // TIM peripheral for DAC trigger
};


using Dma1_Stream5 = DmaStream< DMA1_BASE, 5, 7, 3 >; // DMA 1, Stream 5, channel 7, very high priority
using Dma1_Stream6 = DmaStream< DMA1_BASE, 6, 7, 3 >; // DMA 1, Stream 6, channel 7, very high priority

static constexpr DacChannel channels[] =
{
    {&(DAC->DHR12R1), Timer(TIM6_BASE)},
    {&(DAC->DHR12R2), Timer(TIM7_BASE)},
};

#pragma GCC diagnostic ignored "-Wpedantic"
struct ChannelState chState[] =
{
    {
        .ctx = NULL,
        .idleLevel = 0,
        .stream = Dma1_Stream5::init(10, DataSize::_16BIT, 1)
    },
    {
        .ctx = NULL,
        .idleLevel = 0,
        .stream = Dma1_Stream6::init(10, DataSize::_16BIT, 1)
    }
};
#pragma GCC diagnostic pop


/**
 * \internal
 * Stop an ongoing transfer, deactivating timers and DMA stream.
 */
static void stopTransfer(const uint8_t chNum)
{
    channels[chNum].tim.stop();
    *channels[chNum].dacReg = chState[chNum].idleLevel;
    chState[chNum].ctx->running = 0;
}

/**
 * \internal
 * Actual implementation of DMA interrupt handler.
 */
void __attribute__((used)) DMA_Handler(uint32_t chNum)
{
    if(chNum == 0)
        Dma1_Stream5::IRQhandleInterrupt(&chState[chNum].stream);
    else
        Dma1_Stream6::IRQhandleInterrupt(&chState[chNum].stream);
}

// DMA 1, Stream 5: data transfer for RTX sink
void __attribute__((used)) DMA1_Stream5_IRQHandler()
{
    saveContext();
    asm volatile("mov r0, #0");
    asm volatile("bl _Z11DMA_Handlerm");
    restoreContext();
}

// DMA 1, Stream 6: data transfer for speaker sink
void __attribute__((used)) DMA1_Stream6_IRQHandler()
{
    saveContext();
    asm volatile("mov r0, #1");
    asm volatile("bl _Z11DMA_Handlerm");
    restoreContext();
}



void stm32dac_init(const uint8_t instance, const uint16_t idleLevel)
{
    // Enable peripherals
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;

    switch(instance)
    {
        case STM32_DAC_CH1:
        {
            RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
            __DSB();

            DAC->DHR12R1 = idleLevel;
            DAC->CR |= DAC_CR_DMAEN1    // Enable DMA
                    | 0x00              // TIM6 as trigger source for CH1
                    | DAC_CR_TEN1       // Enable trigger input
                    | DAC_CR_EN1;       // Enable CH1
        }
            break;

        case STM32_DAC_CH2:
        {
            RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
            __DSB();

            DAC->DHR12R2 = idleLevel;
            DAC->CR |= DAC_CR_DMAEN2    // Enable DMA
                    | DAC_CR_TSEL2_1    // TIM7 as trigger source for CH2
                    | DAC_CR_TEN2       // Enable trigger input
                    | DAC_CR_EN2;       // Enable CH2
        }
            break;

        default:
            break;
    }

    chState[instance].idleLevel = idleLevel;
    chState[instance].stream.setEndTransferCallback(std::bind(stopTransfer, instance));
}

void stm32dac_terminate()
{
    // Terminate streams before shutting of the peripherals
    for(int i = 0; i < 2; i++)
    {
        if(chState[i].ctx != NULL)
        {
            if(chState[i].ctx->running != 0)
                chState[i].stream.halt();
        }
    }

    RCC->APB1ENR &= ~(RCC_APB1ENR_DACEN  |
                      RCC_APB1ENR_TIM6EN |
                      RCC_APB1ENR_TIM7EN);
    __DSB();
}

static int stm32dac_start(const uint8_t instance, const void *config,
                          struct streamCtx *ctx)
{
    (void) config;

    if((ctx == NULL) || (ctx->running != 0))
        return -EINVAL;

    if(chState[instance].stream.running())
        return -EBUSY;

    __disable_irq();
    ctx->running = 1;
    __enable_irq();

    ctx->priv = &chState[instance];
    chState[instance].ctx = ctx;

    /*
     * Convert buffer elements from int16_t to unsigned 12 bit values as required
     * by the DAC. Processing can be done in-place because the API mandates that
     * the function caller does not modify the buffer content once this function
     * has been called.
     */
    S16toU12(ctx->buffer, ctx->bufSize);

    bool circ = false;
    if(ctx->bufMode == BUF_CIRC_DOUBLE)
        circ = true;

    chState[instance].stream.start(channels[instance].dacReg, ctx->buffer,
                                  ctx->bufSize, circ);

    // Configure DAC trigger
    channels[instance].tim.setUpdateFrequency(ctx->sampleRate);
    channels[instance].tim.start();

    return 0;
}

static int stm32dac_idleBuf(struct streamCtx *ctx, stream_sample_t **buf)
{
    ChannelState *state = reinterpret_cast< ChannelState * >(ctx->priv);
    *buf = reinterpret_cast< stream_sample_t *>(state->stream.idleBuf());

    return ctx->bufSize/2;
}

static int stm32dac_sync(struct streamCtx *ctx, uint8_t dirty)
{
    ChannelState *state = reinterpret_cast< ChannelState * >(ctx->priv);

    if((ctx->bufMode == BUF_CIRC_DOUBLE) && (dirty != 0))
    {
        void *ptr = state->stream.idleBuf();
        S16toU12(reinterpret_cast< int16_t *>(ptr), ctx->bufSize/2);
    }

   bool ok = state->stream.sync();
   if(ok) return 0;

   return -1;
}

static void stm32dac_stop(struct streamCtx *ctx)
{
    if(ctx->running == 0)
        return;

    reinterpret_cast< ChannelState * >(ctx->priv)->stream.stop();
}

static void stm32dac_halt(struct streamCtx *ctx)
{
    if(ctx->running == 0)
        return;

    reinterpret_cast< ChannelState * >(ctx->priv)->stream.halt();
}

#pragma GCC diagnostic ignored "-Wpedantic"
const struct audioDriver stm32_dac_audio_driver =
{
    .start     = stm32dac_start,
    .data      = stm32dac_idleBuf,
    .sync      = stm32dac_sync,
    .stop      = stm32dac_stop,
    .terminate = stm32dac_halt
};
#pragma GCC diagnostic pop

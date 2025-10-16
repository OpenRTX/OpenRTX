/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <kernel/scheduler/scheduler.h>
#include "core/data_conversion.h"
#include "DmaStream.hpp"
#include "Timer.hpp"
#include <miosix.h>
#include <errno.h>
#include "stm32_pwm.h"


using Dma1_Stream2 = DmaStream< DMA1_BASE, 2, 1, 3 >; // DMA 1, Stream 2, channel 1, very high priority


static const struct PwmChannelCfg *config;                                          // Config
static struct streamCtx     *context;                                               // Current stream context
static StreamHandler         stream(Dma1_Stream2::init(10, DataSize::_16BIT, 1));   // DMA stream handler
static Timer                 tim(TIM7_BASE);                                        // Trigger timebase


/**
 * \internal
 * Stop an ongoing transfer, deactivating timers and DMA stream.
 */
static void stopTransfer()
{
    tim.stop();
    config->stopCbk();
    context->running = 0;
}

/**
 * \internal
 * Actual implementation of DMA interrupt handler.
 */
void __attribute__((used)) DMA_Handler()
{
    Dma1_Stream2::IRQhandleInterrupt(&stream);
}

void __attribute__((naked)) DMA1_Stream2_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z11DMA_Handlerv");
    restoreContext();
}



void stm32pwm_init()
{
    // Enable peripherals
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    __DSB();

    // Init DMA stream
    stream.setEndTransferCallback(stopTransfer);
}

void stm32pwm_terminate()
{
    if(stream.running())
        stream.halt();

    RCC->APB1ENR &= ~RCC_APB1ENR_TIM7EN;
    __DSB();
}

static int stm32pwm_start(const uint8_t instance, const void *cfg,
                          struct streamCtx *ctx)
{
    (void) instance;

    if(ctx == NULL)
        return -EINVAL;

    if((ctx->running != 0) || stream.running())
        return -EBUSY;

    __disable_irq();
    ctx->running = 1;
    __enable_irq();

    context = ctx;
    config  = reinterpret_cast< const struct PwmChannelCfg *>(cfg);

    /*
     * Convert buffer elements from int16_t to unsigned 12 bit values as required
     * by the DAC. Processing can be done in-place because the API mandates that
     * the function caller does not modify the buffer content once this function
     * has been called.
     */
    S16toU8(ctx->buffer, ctx->bufSize);

    bool circ = false;
    if(ctx->bufMode == BUF_CIRC_DOUBLE)
        circ = true;

    stream.start(config->pwmReg, ctx->buffer, ctx->bufSize, circ);
    config->startCbk();

    // Configure trigger
    tim.setUpdateFrequency(ctx->sampleRate);
    tim.enableDmaTrigger(true);
    tim.start();

    return 0;
}

static int stm32pwm_idleBuf(struct streamCtx *ctx, stream_sample_t **buf)
{
    *buf = reinterpret_cast< stream_sample_t *>(stream.idleBuf());

    return ctx->bufSize/2;
}

static int stm32pwm_sync(struct streamCtx *ctx, uint8_t dirty)
{
    if((ctx->bufMode == BUF_CIRC_DOUBLE) && (dirty != 0))
    {
        void *ptr = stream.idleBuf();
        S16toU8(reinterpret_cast< int16_t *>(ptr), ctx->bufSize/2);
    }

   bool ok = stream.sync();
   if(ok) return 0;

   return -1;
}

static void stm32pwm_stop(struct streamCtx *ctx)
{
    if(ctx->running == 0)
        return;

    stream.stop();
}

static void stm32pwm_halt(struct streamCtx *ctx)
{
    if(ctx->running == 0)
        return;

    stream.halt();
}

#pragma GCC diagnostic ignored "-Wpedantic"
const struct audioDriver stm32_pwm_audio_driver =
{
    .start     = stm32pwm_start,
    .data      = stm32pwm_idleBuf,
    .sync      = stm32pwm_sync,
    .stop      = stm32pwm_stop,
    .terminate = stm32pwm_halt
};
#pragma GCC diagnostic pop

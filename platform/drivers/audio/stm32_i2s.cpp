/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <kernel/scheduler/scheduler.h>
#include <miosix.h>
#include <errno.h>
#include <memory>
#include "peripherals/gpio.h"
#include "interfaces/delays.h"
#include "DmaStream.hpp"
#include "hwconfig.h"
#include "stm32_i2s.h"

/*
 * The DMA fills a buffer of interleaved left and right halfwords, twice the
 * size of the stream's: data() copies the configured channel of the idle half
 * into the stream buffer.
 */

static constexpr uint32_t WS_POLLS = 200000;    // About 10 ms
static constexpr uint32_t WS_EDGE_POLLS = 2500; // About 125 us, a frame
static constexpr uint32_t SYNC_TIMEOUT_MS = 100;

using Dma1_Stream0 = DmaStream<DMA1_BASE, 0, 0, 2>; // SPI3_RX, high priority

static StreamHandler stream = Dma1_Stream0::init(10, DataSize::_16BIT, false);
static std::unique_ptr<int16_t[]> stereo;
static size_t stereoSize;
static struct streamCtx *streamCtx;
static size_t channel; // STM32_I2S_LEFT or STM32_I2S_RIGHT
static void *lastIdle;

/**
 * Stop the I2S peripheral at the end of a transfer.
 */
static void stopTransfer()
{
    SPI3->I2SCFGR &= ~SPI_I2SCFGR_I2SE;
    if (streamCtx != NULL)
        streamCtx->running = 0;
}

/**
 * DMA1 Stream 0 interrupt body.
 */
void __attribute__((used)) I2S_DMA_Handler()
{
    Dma1_Stream0::IRQhandleInterrupt(&stream);
}

// DMA 1, Stream 0: SPI3_RX
void __attribute__((used)) DMA1_Stream0_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15I2S_DMA_Handlerv");
    restoreContext();
}

/**
 * Wait for a level on the WS line.
 *
 * @param level: the level to wait for.
 * @param polls: how many times to poll WS before giving up.
 * @return false if WS did not reach the level in time.
 */
static bool wsWait(const uint8_t level, const uint32_t polls)
{
    for (uint32_t i = 0; i < polls; i++) {
        if (gpio_readPin(I2S3_WS) == level)
            return true;
    }

    return false;
}

void stm32i2s_init()
{
    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
    __DSB();

    gpio_setMode(I2S3_WS, ALTERNATE | ALTERNATE_FUNC(6));
    gpio_setMode(I2S3_CK, ALTERNATE | ALTERNATE_FUNC(6));
    gpio_setMode(I2S3_SD, ALTERNATE | ALTERNATE_FUNC(6));

    // Philips standard, 16-bit data in a 32-bit channel, clock idle high,
    // slave receive
    SPI3->I2SCFGR = 0;
    SPI3->I2SPR = 2;
    SPI3->I2SCFGR = SPI_I2SCFGR_I2SMOD | SPI_I2SCFGR_CHLEN | SPI_I2SCFGR_CKPOL
                  | SPI_I2SCFGR_I2SCFG_0;
    SPI3->CR2 = SPI_CR2_RXDMAEN;

    stream.setEndTransferCallback(stopTransfer);
}

void stm32i2s_terminate()
{
    if ((streamCtx != NULL) && (streamCtx->running != 0))
        stream.halt();

    SPI3->I2SCFGR &= ~SPI_I2SCFGR_I2SE;
    RCC->APB1ENR &= ~RCC_APB1ENR_SPI3EN;
    __DSB();
}

static int stm32i2s_start(const uint8_t instance, const void *config,
                          struct streamCtx *ctx)
{
    (void)instance;
    (void)config;

    if (ctx == NULL)
        return -EINVAL;

    if ((ctx->running != 0) || stream.running())
        return -EBUSY;

    if (ctx->sampleRate != 8000)
        return -EINVAL;

    // DMA can't reach the CCM, where .bss lives: the buffer is on the heap
    const size_t size = 2 * ctx->bufSize;
    if (size != stereoSize) {
        stereo.reset(new (std::nothrow) int16_t[size]);
        stereoSize = (stereo != nullptr) ? size : 0;
    }

    if (stereo == nullptr)
        return -ENOMEM;

    channel = reinterpret_cast<uintptr_t>(config);
    ctx->running = 1;
    streamCtx = ctx;

    // Drop stale data and a pending overrun (read DR, then SR)
    (void)SPI3->DR;
    (void)SPI3->SR;

    const bool circ = (ctx->bufMode == BUF_CIRC_DOUBLE);
    stream.start(&SPI3->DR, stereo.get(), size, circ);
    lastIdle = stream.idleBuf();

    // Philips slave: enable just after a WS rising edge, so that the first
    // frame starts with the left channel. WS toggling first says the codec
    // clocks, so that interrupts stay off for one frame at most.
    bool ok = wsWait(0, WS_POLLS) && wsWait(1, WS_POLLS);
    if (ok) {
        miosix::FastInterruptDisableLock dLock;
        ok = wsWait(0, WS_EDGE_POLLS) && wsWait(1, WS_EDGE_POLLS);
        if (ok)
            SPI3->I2SCFGR |= SPI_I2SCFGR_I2SE;
    }

    if (ok == false) {
        stream.halt();
        ctx->running = 0;
        return -EIO;
    }

    return 0;
}

static int stm32i2s_data(struct streamCtx *ctx, stream_sample_t **buf)
{
    const int16_t *in = stereo.get();
    size_t frames = ctx->bufSize;
    stream_sample_t *out = ctx->buffer;

    if (ctx->bufMode == BUF_CIRC_DOUBLE) {
        in = reinterpret_cast<int16_t *>(stream.idleBuf());
        frames = ctx->bufSize / 2;
        out += (in - stereo.get()) / 2;
    }

    for (size_t i = 0; i < frames; i++)
        out[i] = in[2 * i + channel];

    *buf = out;
    return frames;
}

static int stm32i2s_sync(struct streamCtx *ctx, uint8_t dirty)
{
    (void)dirty;

    // The codec clocks the transfer and may stop: poll for the next half of
    // the buffer, or for the end of a linear transfer, and give up after a
    // while instead of waiting for an interrupt that may never come
    for (uint32_t i = 0; i < SYNC_TIMEOUT_MS; i++) {
        if (ctx->bufMode == BUF_CIRC_DOUBLE) {
            void *idle = stream.idleBuf();
            if (idle != lastIdle) {
                lastIdle = idle;
                return 0;
            }

            if (ctx->running == 0)
                return -1;
        } else if (ctx->running == 0) {
            return 0;
        }

        sleepFor(0, 1);
    }

    return -1;
}

static void stm32i2s_stop(struct streamCtx *ctx)
{
    if (ctx->running == 0)
        return;

    stream.stop();
}

static void stm32i2s_halt(struct streamCtx *ctx)
{
    if (ctx->running == 0)
        return;

    stream.halt();
}

#pragma GCC diagnostic ignored "-Wpedantic"
const struct audioDriver stm32_i2s_audio_driver = {
    .start = stm32i2s_start,
    .data = stm32i2s_data,
    .sync = stm32i2s_sync,
    .stop = stm32i2s_stop,
    .terminate = stm32i2s_halt,
};
#pragma GCC diagnostic pop

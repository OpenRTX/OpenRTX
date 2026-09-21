/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * Modulator.cpp — Bell 202 AFSK1200 modulation for APRS.
 *
 * The transmit half of the AFSK chain, mirroring what Decoder and
 * Demodulator undo on the way in: HDLC framing with bit stuffing, NRZI
 * encoding, and two audio tones.
 *
 * The tone generator is a phase accumulator indexing a sine table.  Phase is
 * never reset between symbols, which is the whole point of continuous-phase
 * FSK: a jump at a symbol boundary radiates energy far outside the channel
 * and is exactly what a neighbouring repeater hears as splatter.  Keeping
 * the accumulator in integer arithmetic means the RTX thread needs no
 * floating point, which matters on targets that have none.
 */

#include "protocols/APRS/Modulator.hpp"

#include "core/crc.h"

#include <cstring>

#if defined(PLATFORM_LINUX)
#include <cstdio>
#endif

using namespace APRS;

namespace
{

/** HDLC flag: the one byte allowed to carry six consecutive ones. */
constexpr uint8_t HDLC_FLAG = 0x7e;

/** Bell 202 tone frequencies. Mark is the NRZI "1" level. */
constexpr uint32_t MARK_HZ = 1200;
constexpr uint32_t SPACE_HZ = 2200;

/**
 * Phase increment per sample for a tone, as a 32-bit fixed-point fraction of
 * a full turn.  Computed at compile time: 2^32 * freq / sampleRate, rounded.
 */
constexpr uint32_t phaseStep(uint32_t freq, uint32_t sampleRate)
{
    return (uint32_t)(((uint64_t)freq << 32) / sampleRate);
}

/**
 * Quarter-period sine table, 64 entries of a unit sine scaled to 32767.
 *
 * Only a quarter is stored because the other three are reflections of it;
 * the full table would cost four times the flash for no extra resolution.
 * 64 entries give a phase resolution of 1.4 degrees, whose worst-case
 * amplitude error is well under the quantisation of the output stage.
 */
constexpr int16_t SINE_QUARTER[64] = {
    0,     804,   1608,  2410,  3212,  4011,  4808,  5602,  6393,  7179,  7962,
    8739,  9512,  10278, 11039, 11793, 12539, 13279, 14010, 14732, 15446, 16151,
    16846, 17530, 18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594, 23170,
    23731, 24279, 24811, 25329, 25832, 26319, 26790, 27245, 27683, 28105, 28510,
    28898, 29268, 29621, 29956, 30273, 30571, 30852, 31113, 31356, 31580, 31785,
    31971, 32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757
};

/**
 * Sine of a phase given as the top eight bits of the accumulator, scaled to
 * +/- 32767.  The quarter table is mirrored horizontally for the second and
 * fourth quadrants and vertically for the lower half.
 */
int16_t sineLookup(uint8_t phase)
{
    const uint8_t quadrant = (uint8_t)(phase >> 6);
    const uint8_t index = (uint8_t)(phase & 0x3f);

    switch (quadrant) {
        case 0:
            return SINE_QUARTER[index];
        case 1:
            return SINE_QUARTER[63 - index];
        case 2:
            return (int16_t)(-SINE_QUARTER[index]);
        default:
            return (int16_t)(-SINE_QUARTER[63 - index]);
    }
}

} // namespace

Modulator::Modulator() :
    idleBuffer(nullptr), idleUsed(0), outStream(0), outPath(0),
    txRunning(false), phase(0), level(1), onesRun(0)
{
}

Modulator::~Modulator()
{
    terminate();
}

void Modulator::init()
{
    basebandBuffer = std::make_unique<int16_t[]>(2 * BLOCK_SAMPLES);
    idleBuffer = basebandBuffer.get();
    idleUsed = 0;
    txRunning = false;
    phase = 0;
    level = 1;
    onesRun = 0;
}

void Modulator::terminate()
{
    if (txRunning) {
        audioStream_terminate(outStream);
        txRunning = false;
    }

    audioPath_release(outPath);
    basebandBuffer.reset();
    idleBuffer = nullptr;
}

bool Modulator::start()
{
    if (txRunning)
        return true;

    if (basebandBuffer == nullptr)
        return false;

    /* Start every transmission from a known state so two frames sent back to
     * back cannot inherit a stuffing count or an NRZI level from each
     * other. */
    idleUsed = 0;
    phase = 0;
    level = 1;
    onesRun = 0;
    idleBuffer = basebandBuffer.get();

#ifndef PLATFORM_LINUX
    outPath = audioPath_request(SOURCE_MCU, SINK_RTX, PRIO_TX);
    if (outPath < 0)
        return false;

    outStream = audioStream_start(outPath, basebandBuffer.get(),
                                  2 * BLOCK_SAMPLES, TX_SAMPLE_RATE,
                                  STREAM_OUTPUT | BUF_CIRC_DOUBLE);
    if (outStream < 0) {
        audioPath_release(outPath);
        return false;
    }

    idleBuffer = outputStream_getIdleBuffer(outStream);
#endif

    txRunning = true;
    return true;
}

void Modulator::stop()
{
    if (!txRunning)
        return;

    /* Finish the block in progress rather than cutting it off: the output
     * stage plays whole blocks, so a partial one would otherwise be
     * transmitted with stale samples from the previous pass through the
     * buffer. */
    while (idleUsed != 0)
        pushSample(0);

#ifndef PLATFORM_LINUX
    audioStream_stop(outStream);
    audioPath_release(outPath);
#endif

    txRunning = false;
    idleBuffer = basebandBuffer.get();
    idleUsed = 0;
}

void Modulator::pushSample(int16_t sample)
{
    if (idleBuffer == nullptr)
        return;

    idleBuffer[idleUsed++] = sample;

    if (idleUsed < BLOCK_SAMPLES)
        return;

    emitBlock(idleBuffer, BLOCK_SAMPLES);
    idleUsed = 0;
}

void Modulator::sendSymbol()
{
    const uint32_t step = level ? phaseStep(MARK_HZ, TX_SAMPLE_RATE) :
                                  phaseStep(SPACE_HZ, TX_SAMPLE_RATE);

    for (size_t i = 0; i < SAMPLES_PER_SYMBOL; i++) {
        const int32_t s = sineLookup((uint8_t)(phase >> 24));
        pushSample((int16_t)((s * TX_AMPLITUDE) / 32767));
        phase += step;
    }
}

void Modulator::sendNrziBit(uint8_t bit)
{
    /* NRZI: a zero flips the transmitted level, a one holds it.  Encoding
     * transitions rather than levels is what keeps a long run of ones from
     * looking like silence to the receiver's clock recovery. */
    if (bit == 0)
        level ^= 1u;

    sendSymbol();
}

void Modulator::sendByte(uint8_t byte)
{
    /* AX.25 sends the least significant bit first. */
    for (uint8_t i = 0; i < 8; i++) {
        const uint8_t bit = (uint8_t)((byte >> i) & 1u);

        sendNrziBit(bit);

        if (bit != 0) {
            onesRun++;
            if (onesRun == 5) {
                /* Five ones in a row: insert a zero so the data can never
                 * reproduce the six-ones flag pattern.  The receiver
                 * discards it on sight. */
                sendNrziBit(0);
                onesRun = 0;
            }
        } else {
            onesRun = 0;
        }
    }
}

void Modulator::sendFlag()
{
    /* The flag is the one place six consecutive ones are legal, so it is
     * sent with stuffing off and leaves the run counter clear for whatever
     * follows. */
    for (uint8_t i = 0; i < 8; i++)
        sendNrziBit((uint8_t)((HDLC_FLAG >> i) & 1u));

    onesRun = 0;
}

void Modulator::sendFlags(unsigned ms)
{
    /* One flag is eight symbols at APRS_SYMBOL_RATE. */
    const unsigned flags = (ms * APRS_SYMBOL_RATE) / (8u * 1000u);

    for (unsigned i = 0; i < (flags ? flags : 1u); i++)
        sendFlag();
}

void Modulator::sendFrame(const uint8_t *data, size_t len)
{
    if ((data == nullptr) || (len == 0))
        return;

    onesRun = 0;

    for (size_t i = 0; i < len; i++)
        sendByte(data[i]);

    /* The frame check sequence covers everything between the flags and goes
     * out least significant byte first, which is what the decoder's
     * crc_hdlc() comparison expects to find there. */
    const uint16_t fcs = crc_hdlc(data, len);
    sendByte((uint8_t)(fcs & 0xff));
    sendByte((uint8_t)(fcs >> 8));

    /* A closing flag terminates the frame; it also opens the next one if
     * another frame follows immediately. */
    sendFlag();
}

#ifndef PLATFORM_LINUX
void Modulator::emitBlock(const stream_sample_t *samples, size_t len)
{
    (void)samples;
    (void)len;

    if (!txRunning)
        return;
    if (audioPath_getStatus(outPath) != PATH_OPEN)
        return;

    /* Wait for the output stage to finish the other half of the buffer
     * before claiming it for the next block. */
    outputStream_sync(outStream, true);
    idleBuffer = outputStream_getIdleBuffer(outStream);
}
#else
void Modulator::emitBlock(const stream_sample_t *samples, size_t len)
{
    /* The emulator has no transmit audio device, so the baseband goes to a
     * file the e2e test can pick up — the same arrangement M17::Modulator
     * uses, and for the same reason. */
    FILE *out = fopen("/tmp/aprs_output.raw", "ab");
    if (out == nullptr)
        return;

    fwrite(samples, sizeof(*samples), len, out);
    fclose(out);
}
#endif

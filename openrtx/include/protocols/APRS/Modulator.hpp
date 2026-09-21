/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef APRS_MODULATOR_H
#define APRS_MODULATOR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstddef>
#include <cstdint>
#include <memory>
#include "core/audio_path.h"
#include "core/audio_stream.h"
#include "protocols/APRS/constants.h"

/* Targets may tune the transmit level in their own hwconfig.h; see
 * Modulator::TX_AMPLITUDE for what it controls. */
#ifndef CONFIG_APRS_TX_AMPLITUDE
#define CONFIG_APRS_TX_AMPLITUDE 12000
#endif

namespace APRS
{

/**
 * Bell 202 AFSK1200 modulator: the inverse of the demodulator chain.
 *
 * Takes a frame the way APRS::Decoder hands one back — addresses through
 * info field, no frame check sequence — and puts it on the air as HDLC:
 * opening flags, the frame with its FCS appended, closing flags, with bit
 * stuffing and NRZI encoding in between, carried by 1200 Hz and 2200 Hz
 * tones.
 *
 * Baseband generation is streamed rather than buffered whole.  A frame at
 * 1200 baud is up to a second of audio, which is far more than any embedded
 * target can hold at the output sample rate, so tone samples are produced a
 * block at a time into the idle half of a double buffer and handed to the
 * output stream as it drains — the same shape as M17::Modulator.
 *
 * Everything runs in integer arithmetic off a sine lookup table: the RTX
 * thread has no FPU time to spare on targets that lack an FPU entirely.
 */
class Modulator
{
public:
    /**
     * Constructor.
     */
    Modulator();

    /**
     * Destructor.
     */
    virtual ~Modulator();

    /**
     * Allocate the baseband buffer and initialise the modulator.
     */
    void init();

    /**
     * Shut the modulator down and release its buffer.
     */
    void terminate();

    /**
     * Open the output path and start the baseband stream.
     *
     * @return true if the modulator was started.
     */
    bool start();

    /**
     * Emit the opening flag sequence.
     *
     * Transmitting flags while the receiving station's squelch opens and its
     * demodulator settles is what TNCs call TXDelay: the frame that follows
     * is only decodable if the receiver is already tracking the bit clock
     * when it starts.
     *
     * @param ms: length of the flag sequence, in milliseconds.
     */
    void sendFlags(unsigned ms);

    /**
     * Emit one frame: its bytes, its frame check sequence, and the closing
     * flag that terminates it.
     *
     * @param data: raw AX.25 frame, without its frame check sequence.
     * @param len: frame length in bytes.
     */
    void sendFrame(const uint8_t *data, size_t len);

    /**
     * Flush whatever is left in the idle buffer and stop the stream.
     */
    void stop();

    /**
     * Baseband sample rate the modulator generates at.
     *
     * Matches what M17 transmits at, because it is the same output path: MCU
     * baseband into the radio's modulator input.
     */
    static constexpr size_t TX_SAMPLE_RATE = 48000;

    /**
     * Samples per Bell 202 symbol at TX_SAMPLE_RATE.
     */
    static constexpr size_t SAMPLES_PER_SYMBOL = TX_SAMPLE_RATE
                                               / APRS_SYMBOL_RATE;

    /**
     * Symbols per baseband block.
     *
     * Eight symbols is 6.7 ms of audio: long enough that the stream handover
     * happens 150 times a second rather than 1200, short enough that two
     * blocks are a 1280-byte buffer an embedded target can spare.
     */
    static constexpr size_t BLOCK_SYMBOLS = 8;

    /**
     * Baseband block size, in samples.
     */
    static constexpr size_t BLOCK_SAMPLES = BLOCK_SYMBOLS * SAMPLES_PER_SYMBOL;

    /**
     * Peak amplitude of the generated tones.
     *
     * This is what sets FM deviation on a real radio: the baseband goes to
     * the modulator input, so a louder tone deviates further.  APRS wants
     * roughly 3 kHz deviation, and the value that produces it depends on the
     * target's output stage, which is why a target may override it in its
     * own hwconfig.h.  The default is deliberately conservative — under-
     * deviating is merely quiet, over-deviating splatters into the adjacent
     * channel.
     */
    static constexpr int32_t TX_AMPLITUDE = CONFIG_APRS_TX_AMPLITUDE;

protected:
    /**
     * Hand one filled block of baseband to the output stage.
     *
     * Virtual so a test can capture the baseband without an audio device:
     * the software loopback test overrides it, collects the samples, and
     * feeds them straight back into APRS::Demodulator, which checks the
     * whole modulator against the whole demodulator in one process.  The
     * call happens 150 times a second, so the indirection costs nothing
     * measurable on the RTX thread.
     *
     * @param samples: block of BLOCK_SAMPLES baseband samples.
     * @param len: number of samples in the block.
     */
    virtual void emitBlock(const stream_sample_t *samples, size_t len);

private:
    /**
     * Append one sample to the idle buffer, handing the buffer to the output
     * stream once it is full.
     */
    void pushSample(int16_t sample);

    /**
     * Generate one symbol's worth of tone at the current level.
     *
     * Phase carries across symbols: a discontinuity at a symbol boundary
     * spreads the signal far outside the channel and is exactly what
     * continuous-phase FSK exists to avoid.
     */
    void sendSymbol();

    /**
     * Send one bit through NRZI: a zero flips the transmitted level, a one
     * holds it.  This is what makes the signal self-clocking.
     */
    void sendNrziBit(uint8_t bit);

    /**
     * Send one byte, least significant bit first, inserting a stuffed zero
     * after five consecutive ones so no flag pattern can occur inside data.
     */
    void sendByte(uint8_t byte);

    /**
     * Send one HDLC flag (0x7e) with stuffing disabled — the flag is the one
     * place six consecutive ones are allowed, and is what marks a frame
     * boundary.
     */
    void sendFlag();

    std::unique_ptr<int16_t[]> basebandBuffer; ///< Double baseband buffer.
    stream_sample_t *idleBuffer; ///< Half of it, free for generation.
    size_t idleUsed;             ///< Samples written into idleBuffer.
    streamId outStream;          ///< Baseband output stream ID.
    pathId outPath;              ///< Baseband output path ID.
    bool txRunning;              ///< Stream is open.

    uint32_t phase;              ///< Tone phase accumulator.
    uint8_t level;               ///< Current NRZI level: 1 is mark, 0 is space.
    uint8_t onesRun; ///< Consecutive one bits sent, for bit stuffing.
};

} /* APRS */

#endif /* APRS_MODULATOR_H */

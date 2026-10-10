/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17REPLAY_H
#define M17REPLAY_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <array>
#include <cstdint>
#include <cstdio>

#include "protocols/M17/Demodulator.hpp"
#include "protocols/M17/FrameDecoder.hpp"
#include "protocols/M17/PacketFrame.hpp"

/**
 * Replay of a recorded M17 baseband through the receive chain.
 *
 * A recording (raw signed 16-bit little-endian samples, mono) is fed to the
 * exact M17 demodulator and frame decoder used on the radios, mirroring the
 * receive path of OpMode_M17: samples are consumed in 20 ms blocks, the frame
 * decoder is reset whenever a lock is acquired, and frames are consumed only
 * while locked.
 *
 * Counted for any M17 transmission:
 *  - lock acquisitions and, when verbose, the time of every lock and unlock;
 *  - link setup frames, split by CRC outcome;
 *  - stream frames (voice or data), the frame numbers skipped while locked
 *    and the end-of-stream frames seen. A clean stream has zero skipped;
 *  - packets of every protocol, reassembled by frame counter up to the
 *    end-of-frame marker and CRC checked. When verbose the text of SMS
 *    packets is printed.
 *
 * Used by the command line tool in m17_replay_tool.cpp and by the unit test in
 * tests/unit/M17_replay.cpp. A replay cannot show effects of real-time
 * scheduling on the radio or of the RF front end; for those see the Linux
 * emulator, which replays /tmp/baseband.raw in real time through the full
 * receive path (SOURCE_RTX in platform/drivers/audio/audio_linux.c), and
 * ENABLE_DEMOD_LOG with scripts/plot_m17_demod_csv.py for per-sample
 * demodulator internals.
 */
class M17Replay
{
public:
    /**
     * Counters filled during a replay.
     */
    struct Counts {
        uint32_t locks;          ///< Lock acquisitions.
        uint32_t lsfValid;       ///< Link setup frames with a valid CRC.
        uint32_t lsfInvalid;     ///< Link setup frames with a bad CRC.
        uint32_t streamFrames;   ///< Stream frames decoded.
        uint32_t streamWithLsf;  ///< Stream frames decoded with a valid LSF.
        uint32_t streamMissed;   ///< Frame numbers skipped while locked.
        uint32_t streamEnds;     ///< Stream frames with the end-of-stream bit.
        uint32_t packetFrames;   ///< Packet frames decoded.
        uint32_t packetsOk;      ///< Packets completed with a valid CRC.
        uint32_t packetsCrc;     ///< Packets completed with a bad CRC.
        uint32_t packetsAborted; ///< Packets dropped on a counter mismatch.
    };

    /**
     * Constructor.
     *
     * @param invertPhase: invert the baseband polarity before demodulation.
     * @param verbose: print time-stamped lock, LSF and packet events.
     */
    M17Replay(const bool invertPhase, const bool verbose);

    /**
     * Destructor.
     */
    ~M17Replay();

    /**
     * Replay a recording from a given time to its end. The counters are
     * cleared first;
     * the demodulator keeps its state across calls, so for independent
     * measurements use one object per recording.
     *
     * @param path: file with raw signed 16-bit little-endian mono samples.
     * @param sampleRate: sample rate of the file, 24000 or 48000 Hz. A 48 kHz
     * file is decimated 2:1, which is enough since the transmit filter limits
     * the baseband well below 12 kHz.
     * @param start: time in seconds at which to start reading the file.
     * @return true if the file was read to its end, false if it could not be
     * opened or the sample rate is not supported.
     */
    bool replay(const char *path, const uint32_t sampleRate,
                const double start = 0.0);

    /**
     * Replay samples held in memory, as replay() does for a file.
     *
     * @param samples: signed 16-bit mono samples.
     * @param numSamples: number of samples.
     * @param sampleRate: 24000 or 48000 Hz.
     * @return false if the sample rate is not supported.
     */
    bool replay(const int16_t *samples, const size_t numSamples,
                const uint32_t sampleRate);

    /**
     * Get the counters of the last replay.
     *
     * @return a reference to the counters.
     */
    const Counts &counts() const;

private:
    /**
     * Feed one block to the demodulator and handle lock changes and the
     * frame it may have completed, as OpMode_M17::rxState() does.
     *
     * @param block: BLOCK_SAMPLES samples at the demodulator rate.
     * @param time: time of the block in the recording, in seconds.
     */
    void processBlock(const int16_t *block, const double time);

    /**
     * Account for a decoded link setup frame.
     *
     * @param time: time of the frame in the recording, in seconds.
     */
    void handleLinkSetup(const double time);

    /**
     * Account for a decoded stream frame, tracking the frame number sequence.
     *
     * @param time: time of the frame in the recording, in seconds.
     */
    void handleStream(const double time);

    /**
     * Append a decoded packet frame to the packet under reassembly and check
     * the packet CRC on the last frame.
     *
     * @param time: time of the frame in the recording, in seconds.
     */
    void handlePacket(const double time);

    /**
     * Print a finished packet with its outcome and, for SMS, its text.
     *
     * @param time: time of the packet in the recording, in seconds.
     * @param outcome: "ok", "crc" or "aborted".
     */
    void printPacket(const double time, const char *outcome);

    static constexpr uint32_t SAMPLE_RATE = 24000; ///< Demodulator rate.
    static constexpr size_t BLOCK_SAMPLES = 480;   ///< 20 ms, as the radios.
    static constexpr size_t MAX_PACKET = 33 * M17::PacketFrame::DATA_SIZE;

    bool invertPhase;      ///< Invert baseband polarity.
    bool verbose;          ///< Print per-event lines.
    bool locked;           ///< Demodulator lock state after the last block.
    bool haveStreamFn;     ///< lastStreamFn holds a valid frame number.
    uint16_t lastStreamFn; ///< Frame number of the last stream frame.
    size_t packetLen;      ///< Bytes of the packet under reassembly.
    uint8_t packetNext;    ///< Expected counter of the next packet frame.
    Counts count;          ///< Counters of the current replay.

    M17::Demodulator demod;
    M17::FrameDecoder decoder;
    M17::frame_t frame;
    std::array<uint8_t, MAX_PACKET> packet;
};

#endif // M17REPLAY_H

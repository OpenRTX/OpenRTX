/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * M17 baseband replay.
 *
 * Replays a recorded baseband (raw signed 16-bit little-endian samples, mono)
 * through the exact M17 demodulator and frame decoder used on the radios and
 * counts what was received. The processing mirrors the receive path of
 * OpMode_M17: samples are consumed in 20 ms blocks, the frame decoder is reset
 * whenever a lock is acquired, and frames are consumed only while locked.
 *
 * Measured for any M17 transmission:
 *  - lock acquisitions and, when verbose, the time of every lock and unlock;
 *  - link setup frames, split by CRC outcome;
 *  - stream frames (voice or data), the frame numbers skipped while locked
 *    and the end-of-stream frames seen. A clean stream has zero skipped;
 *  - packets of every protocol, reassembled by frame counter up to the
 *    end-of-frame marker and CRC checked. When verbose the text of SMS
 *    packets is printed.
 *
 * Used by the command line tool in m17_replay.cpp and by the unit test in
 * tests/unit/M17_replay.cpp, which replays the committed 48 kHz voice
 * recording tests/unit/assets/M17_test_baseband_dc.raw and requires the link
 * setup frame and the full stream to decode.
 *
 * Related: the Linux emulator replays /tmp/baseband.raw in real time through
 * the full receive path and user interface (SOURCE_RTX in
 * platform/drivers/audio/audio_linux.c), and ENABLE_DEMOD_LOG together with
 * scripts/plot_m17_demod_csv.py plots per-sample demodulator internals. A
 * replay cannot show effects of real-time scheduling on the radio or of the
 * RF front end.
 */

#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>

#include "core/crc.h"
#include "protocols/M17/Constants.hpp"
#include "protocols/M17/Demodulator.hpp"
#include "protocols/M17/FrameDecoder.hpp"
#include "protocols/M17/LinkSetupFrame.hpp"
#include "protocols/M17/PacketFrame.hpp"
#include "protocols/M17/StreamFrame.hpp"

namespace replay
{

constexpr uint32_t SAMPLE_RATE = 24000; // rate the demodulator runs at
constexpr size_t BLOCK_SAMPLES = 480;   // 20 ms, as the radios process it
constexpr size_t MAX_PACKET = 33 * M17::PacketFrame::DATA_SIZE;

// Summary counters, in print order.
static const char *const KEYS[] = {
    "locks",         "lsf_valid",       "lsf_invalid",   "stream_frames",
    "stream_missed", "stream_ends",     "packet_frames", "packets_ok",
    "packets_crc",   "packets_aborted",
};

using Counts = std::map<std::string, uint32_t>;

struct Options {
    const char *path = nullptr;
    uint32_t rate = SAMPLE_RATE; // 24000, or 48000 decimated 2:1
    bool invert = false;
    bool verbose = false;
};

struct Reassembly {
    std::array<uint8_t, MAX_PACKET> buf{};
    size_t len = 0;
    uint8_t next = 0;

    void reset()
    {
        len = 0;
        next = 0;
    }
};

// Read one 20 ms block at the demodulator rate, keeping every step-th input
// sample. The transmit filter limits the baseband well below 12 kHz, so a
// 48 kHz file can be decimated 2:1 by dropping samples.
inline bool readBlock(FILE *fp, size_t step, int16_t *block)
{
    int16_t in[2];
    for (size_t i = 0; i < BLOCK_SAMPLES; i++) {
        if (std::fread(in, sizeof(int16_t), step, fp) != step)
            return false;
        block[i] = in[0];
    }
    return true;
}

// Print a finished packet. Payload byte 0 is the protocol identifier and the
// last two bytes are the CRC; SMS (0x05) carries NUL-terminated text between
// them, printed even on a bad CRC since it usually identifies the message.
inline void printPacket(double t, const Reassembly &r, const char *outcome)
{
    std::string text;
    if (r.len >= 3 && r.buf[0] == 0x05) {
        text.assign(reinterpret_cast<const char *>(&r.buf[1]), r.len - 3);
        while (!text.empty() && text.back() == '\0')
            text.pop_back();
        for (char &c : text)
            if (static_cast<unsigned char>(c) < 0x20)
                c = '?';
    }
    std::printf("%8.3f  packet %-7s len=%3zu type=0x%02x %s%s%s\n", t, outcome,
                r.len, r.len ? r.buf[0] : 0, text.empty() ? "" : "\"",
                text.c_str(), text.empty() ? "" : "\"");
}

// Replay the recording named in opt and fill count. Returns false if the file
// cannot be opened. opt.rate must be 24000 or 48000.
inline bool run(const Options &opt, Counts &count)
{
    using namespace M17;

    FILE *fp = std::fopen(opt.path, "rb");
    if (fp == nullptr) {
        std::perror(opt.path);
        return false;
    }

    for (const char *k : KEYS)
        count[k] = 0;

    Reassembly pkt;
    Demodulator demod;
    FrameDecoder decoder;
    demod.init();

    bool locked = false;
    bool frameReady = false;
    bool haveStreamFn = false;
    uint16_t lastStreamFn = 0;
    frame_t frame;
    int16_t block[BLOCK_SAMPLES];
    const size_t step = opt.rate / SAMPLE_RATE;

    for (size_t b = 0; readBlock(fp, step, block); b++) {
        const double t = static_cast<double>(b * BLOCK_SAMPLES) / SAMPLE_RATE;

        for (size_t i = 0; i < BLOCK_SAMPLES; i++) {
            demod.sample(block[i], opt.invert);
            if (demod.newFrameReady()) {
                frame = demod.getFrame();
                frameReady = true;
            }
        }

        // Lock handling as in OpMode_M17::rxState(): the frame decoder is
        // reset when a lock is acquired, and frames are consumed only while
        // locked.
        bool lock = demod.isLocked();
        if (lock && !locked) {
            decoder.reset();
            pkt.reset();
            haveStreamFn = false;
            count["locks"]++;
            if (opt.verbose)
                std::printf("%8.3f  lock\n", t);
        }
        if (!lock && locked && opt.verbose)
            std::printf("%8.3f  unlock\n", t);
        locked = lock;

        if (!frameReady)
            continue;
        frameReady = false;
        if (!locked)
            continue;

        switch (decoder.decodeFrame(frame)) {
            case FrameType::LINK_SETUP: {
                LinkSetupFrame lsf = decoder.getLsf();
                if (!lsf.valid()) {
                    count["lsf_invalid"]++;
                    break;
                }
                count["lsf_valid"]++;
                if (opt.verbose) {
                    Callsign src = lsf.getSource();
                    Callsign dst = lsf.getDestination();
                    std::printf("%8.3f  lsf %s -> %s\n", t,
                                static_cast<const char *>(src),
                                static_cast<const char *>(dst));
                }
                break;
            }
            case FrameType::STREAM: {
                count["stream_frames"]++;
                // Frame numbers count up modulo 0x8000 within one lock; a
                // small forward gap means frames were lost while locked. A
                // repeated or backward number is a frame the decoder zeroed
                // for excess errors, noise, or a new stream: no count.
                StreamFrame sf = decoder.getStreamFrame();
                uint16_t fn = sf.getFrameNumber() & 0x7FFF;
                if (haveStreamFn) {
                    uint16_t gap = (fn - lastStreamFn - 1) & 0x7FFF;
                    if (gap < 0x4000)
                        count["stream_missed"] += gap;
                }
                haveStreamFn = true;
                lastStreamFn = fn;
                if (sf.isLastFrame()) {
                    count["stream_ends"]++;
                    haveStreamFn = false;
                }
                break;
            }
            case FrameType::PACKET: {
                if (!decoder.getLsf().valid())
                    break;
                count["packet_frames"]++;
                const PacketFrame &pf = decoder.getPacketFrame();
                uint8_t counter = pf.getCounter();
                if (!pf.isEof()) {
                    if (counter != pkt.next
                        || pkt.len + PacketFrame::DATA_SIZE > pkt.buf.size()) {
                        count["packets_aborted"]++;
                        if (opt.verbose)
                            printPacket(t, pkt, "aborted");
                        pkt.reset();
                        break;
                    }
                    std::memcpy(&pkt.buf[pkt.len], pf.data(),
                                PacketFrame::DATA_SIZE);
                    pkt.len += PacketFrame::DATA_SIZE;
                    pkt.next++;
                    break;
                }
                // Last frame: the counter holds the number of valid bytes.
                if (counter == 0 || counter > PacketFrame::DATA_SIZE
                    || pkt.len + counter > pkt.buf.size()) {
                    count["packets_aborted"]++;
                    if (opt.verbose)
                        printPacket(t, pkt, "aborted");
                    pkt.reset();
                    break;
                }
                std::memcpy(&pkt.buf[pkt.len], pf.data(), counter);
                pkt.len += counter;
                bool crcOk = false;
                if (pkt.len >= 3) {
                    uint16_t computed = crc_m17(pkt.buf.data(), pkt.len - 2);
                    uint16_t stored = (pkt.buf[pkt.len - 2] << 8)
                                    | pkt.buf[pkt.len - 1];
                    crcOk = (computed == stored);
                }
                count[crcOk ? "packets_ok" : "packets_crc"]++;
                if (opt.verbose)
                    printPacket(t, pkt, crcOk ? "ok" : "crc");
                pkt.reset();
                break;
            }
            default:
                break;
        }
    }
    std::fclose(fp);
    return true;
}

} // namespace replay

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#include "core/crc.h"
#include "protocols/M17/LinkSetupFrame.hpp"
#include "protocols/M17/StreamFrame.hpp"
#include "M17Replay.hpp"

using namespace M17;

M17Replay::M17Replay(const bool invertPhase, const bool verbose)
    : invertPhase(invertPhase)
    , verbose(verbose)
    , locked(false)
    , haveStreamFn(false)
    , lastStreamFn(0)
    , packetLen(0)
    , packetNext(0)
    , count{}
{
    demod.init();
}

M17Replay::~M17Replay()
{
}

bool M17Replay::replay(const char *path, const uint32_t sampleRate,
                       const double start)
{
    FILE *file = std::fopen(path, "rb");
    if (file == nullptr)
        return false;

    long offset = std::lround(start * sampleRate) * sizeof(int16_t);
    if (std::fseek(file, offset, SEEK_SET) != 0) {
        std::fclose(file);
        return false;
    }

    std::vector<int16_t> samples;
    int16_t chunk[BLOCK_SAMPLES];
    size_t len;

    while ((len = std::fread(chunk, sizeof(int16_t), BLOCK_SAMPLES, file)) > 0)
        samples.insert(samples.end(), chunk, chunk + len);

    std::fclose(file);
    return replay(samples.data(), samples.size(), sampleRate);
}

bool M17Replay::replay(const int16_t *samples, const size_t numSamples,
                       const uint32_t sampleRate)
{
    if ((sampleRate != SAMPLE_RATE) && (sampleRate != 2 * SAMPLE_RATE))
        return false;

    count = Counts{};
    locked = false;
    haveStreamFn = false;
    packetLen = 0;
    packetNext = 0;
    decoder.reset();
    lastStream.clear();

    const size_t step = sampleRate / SAMPLE_RATE;
    int16_t block[BLOCK_SAMPLES];

    for (size_t n = 0; (n + 1) * BLOCK_SAMPLES * step <= numSamples; n++) {
        for (size_t i = 0; i < BLOCK_SAMPLES; i++)
            block[i] = samples[((n * BLOCK_SAMPLES) + i) * step];

        processBlock(block,
                     static_cast<double>(n * BLOCK_SAMPLES) / SAMPLE_RATE);
    }

    return true;
}

const M17Replay::Counts &M17Replay::counts() const
{
    return count;
}

void M17Replay::processBlock(const int16_t *block, const double time)
{
    bool frameReady = false;

    for (size_t i = 0; i < BLOCK_SAMPLES; i++) {
        if (demod.sample(block[i], invertPhase)) {
            frame = demod.getFrame();
            frameReady = true;
        }
    }

    // Lock handling as in OpMode_M17::rxState(): the frame decoder is reset
    // when a lock is acquired, and frames are consumed only while locked.
    bool lock = demod.isLocked();
    if (lock && !locked) {
        decoder.reset();
        lastStream.clear();
        packetLen = 0;
        packetNext = 0;
        haveStreamFn = false;
        count.locks++;
        if (verbose)
            std::printf("%8.3f  lock\n", time);
    }

    if (!lock && locked && verbose)
        std::printf("%8.3f  unlock\n", time);

    locked = lock;
    if (!frameReady || !locked)
        return;

    switch (decoder.decodeFrame(frame)) {
        case FrameType::LINK_SETUP:
            handleLinkSetup(time);
            break;
        case FrameType::STREAM:
            handleStream(time);
            break;
        case FrameType::PACKET:
            handlePacket(time);
            break;
        default:
            break;
    }
}

void M17Replay::handleLinkSetup(const double time)
{
    LinkSetupFrame lsf = decoder.getLsf();

    if (!lsf.valid()) {
        count.lsfInvalid++;
        return;
    }

    count.lsfValid++;
    if (verbose) {
        Callsign src = lsf.getSource();
        Callsign dst = lsf.getDestination();
        std::printf("%8.3f  lsf %s -> %s\n", time,
                    static_cast<const char *>(src),
                    static_cast<const char *>(dst));
    }
}

void M17Replay::handleStream(const double time)
{
    (void)time;
    count.streamFrames++;

    // The radio plays a stream frame only once the LSF is known, either from
    // the link setup frame or rebuilt from the LICH of the stream frames.
    if (decoder.getLsf().valid())
        count.streamWithLsf++;

    // A rejected frame leaves the decoder's frame unchanged, or cleared after
    // a lock: an all-zero frame 0 also reads as rejected.
    StreamFrame sf = decoder.getStreamFrame();
    if ((sf.getFrameNumber() == lastStream.getFrameNumber())
        && (std::memcmp(sf.data(), lastStream.data(), sizeof(payload_t))
            == 0)) {
        count.streamRejected++;
        return;
    }

    lastStream = sf;

    // Frame numbers count up modulo 0x8000 within one lock; a small forward
    // gap means frames were lost while locked. A repeated or backward number
    // is noise or a new stream, and is not counted.
    uint16_t fn = sf.getFrameNumber() & 0x7FFF;

    if (haveStreamFn) {
        uint16_t gap = (fn - lastStreamFn - 1) & 0x7FFF;
        if (gap < 0x4000)
            count.streamMissed += gap;
    }

    haveStreamFn = true;
    lastStreamFn = fn;

    if (sf.isLastFrame()) {
        count.streamEnds++;
        haveStreamFn = false;
    }
}

void M17Replay::handlePacket(const double time)
{
    if (!decoder.getLsf().valid())
        return;

    count.packetFrames++;
    const PacketFrame &pf = decoder.getPacketFrame();
    uint8_t counter = pf.getCounter();

    if (!pf.isEof()) {
        if ((counter != packetNext)
            || (packetLen + PacketFrame::DATA_SIZE > packet.size())) {
            count.packetsAborted++;
            if (verbose)
                printPacket(time, "aborted");
            packetLen = 0;
            packetNext = 0;
            return;
        }

        std::memcpy(&packet[packetLen], pf.data(), PacketFrame::DATA_SIZE);
        packetLen += PacketFrame::DATA_SIZE;
        packetNext++;
        return;
    }

    // Last frame: the counter holds the number of valid bytes.
    if ((counter == 0) || (counter > PacketFrame::DATA_SIZE)
        || (packetLen + counter > packet.size())) {
        count.packetsAborted++;
        if (verbose)
            printPacket(time, "aborted");
        packetLen = 0;
        packetNext = 0;
        return;
    }

    std::memcpy(&packet[packetLen], pf.data(), counter);
    packetLen += counter;

    bool crcOk = false;
    if (packetLen >= 3) {
        uint16_t computed = crc_m17(packet.data(), packetLen - 2);
        uint16_t stored = (packet[packetLen - 2] << 8) | packet[packetLen - 1];
        crcOk = (computed == stored);
    }

    if (crcOk)
        count.packetsOk++;
    else
        count.packetsCrc++;

    if (verbose)
        printPacket(time, crcOk ? "ok" : "crc");

    packetLen = 0;
    packetNext = 0;
}

void M17Replay::printPacket(const double time, const char *outcome)
{
    // Payload byte 0 is the protocol identifier and the last two bytes are
    // the CRC; SMS (0x05) carries NUL-terminated text between them, printed
    // even on a bad CRC since it usually identifies the message.
    std::string text;
    if ((packetLen >= 3) && (packet[0] == 0x05)) {
        text.assign(reinterpret_cast<const char *>(&packet[1]), packetLen - 3);
        while (!text.empty() && text.back() == '\0')
            text.pop_back();
        for (char &c : text)
            if (static_cast<unsigned char>(c) < 0x20)
                c = '?';
    }

    std::printf("%8.3f  packet %-7s len=%3zu type=0x%02x %s%s%s\n", time,
                outcome, packetLen, packetLen ? packet[0] : 0,
                text.empty() ? "" : "\"", text.c_str(),
                text.empty() ? "" : "\"");
}

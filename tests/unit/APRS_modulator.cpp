/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * Software loopback for the AFSK1200 transmit chain: modulate a frame, feed
 * the baseband straight back into the demodulator, and check that what comes
 * out the other end parses to the frame that went in.
 *
 * Running both halves in one process is what makes this worth having as a
 * unit test rather than only as an emulator script. Bit stuffing, NRZI
 * polarity, byte bit order, the frame check sequence, and continuous-phase
 * tone generation are each individually easy to get backwards and
 * individually invisible until something fails to decode; a round trip pins
 * all of them at once, with no audio device, no timing, and no radio.
 *
 * The demodulator hands back the frame with its check sequence still on it,
 * exactly as it reaches the packet layer on the air, so the comparison goes
 * through aprsPktFromFrame() — which is how a received frame is read for
 * real — rather than a raw byte compare.
 */

#include <catch2/catch_test_macros.hpp>

#include "protocols/APRS/Demodulator.hpp"
#include "protocols/APRS/Modulator.hpp"
#include "protocols/APRS/packet.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace
{

/** A decoded frame: the raw bytes the demodulator produced, and their count. */
struct rawFrame {
    uint8_t data[APRS_PACLEN];
    uint8_t len;
};

/** Modulator that keeps its baseband instead of playing it. */
class CapturingModulator : public APRS::Modulator
{
public:
    std::vector<int16_t> samples;

protected:
    void emitBlock(const stream_sample_t *block, size_t len) override
    {
        samples.insert(samples.end(), block, block + len);
    }
};

constexpr size_t DECIMATION = APRS::Modulator::TX_SAMPLE_RATE
                            / APRS_SAMPLE_RATE;

static_assert(APRS::Modulator::TX_SAMPLE_RATE % APRS_SAMPLE_RATE == 0,
              "transmit rate must be a whole multiple of the receive rate");

/*
 * The transmit chain generates at Modulator::TX_AMPLITUDE, sized to drive a
 * radio's modulator input, not a demodulator directly. The demodulator's
 * correlators run in int16 arithmetic and saturate above roughly 500 peak, so
 * a loopback at transmit level would fail for reasons unrelated to the
 * modulator. That headroom limit is a real receive-side robustness gap — a
 * strong signal on a real radio hits it too — and is tracked separately;
 * hence this explicit, documented rescale.
 */
constexpr int32_t RX_PEAK = 300;

std::vector<int16_t> toReceiveBaseband(const std::vector<int16_t> &tx)
{
    int32_t peak = 1;
    for (int16_t s : tx) {
        const int32_t mag = (s < 0) ? -(int32_t)s : (int32_t)s;
        if (mag > peak)
            peak = mag;
    }

    std::vector<int16_t> rx;
    rx.reserve(tx.size() / DECIMATION);
    for (size_t i = 0; i < tx.size(); i += DECIMATION)
        rx.push_back((int16_t)(((int32_t)tx[i] * RX_PEAK) / peak));

    return rx;
}

std::vector<rawFrame> demodulate(std::vector<int16_t> &rx)
{
    APRS::Demodulator demod;
    demod.init();

    std::vector<rawFrame> frames;
    for (size_t i = 0; i < rx.size(); i += APRS_BUF_SIZE) {
        dataBlock_t block;
        block.data = &rx[i];
        block.len = ((rx.size() - i) < APRS_BUF_SIZE) ? (rx.size() - i) :
                                                        APRS_BUF_SIZE;

        if (demod.update(block)) {
            rawFrame f;
            ssize_t n = demod.getFrame(f.data, sizeof(f.data));
            if (n > 0) {
                f.len = (uint8_t)n;
                frames.push_back(f);
            }
        }
    }

    return frames;
}

/** Modulate one built frame with a realistic preamble and tail, demodulate. */
std::vector<rawFrame> loopback(const uint8_t *frame, size_t len)
{
    CapturingModulator mod;

    mod.init();
    REQUIRE(mod.start() == true);
    /* A short key-up: the demodulator needs a few symbol times of tone to
     * settle before the frame starts. */
    mod.sendFlags(100);
    mod.sendFrame(frame, len);
    mod.sendFlags(20);
    mod.stop();
    mod.terminate();

    REQUIRE(mod.samples.empty() == false);

    std::vector<int16_t> rx = toReceiveBaseband(mod.samples);
    return demodulate(rx);
}

} // namespace

TEST_CASE("APRS modulator: a modulated frame decodes back", "[aprs][mod]")
{
    uint8_t frame[APRS_PACLEN];
    const char info[] = ":W1AW     :hello there";
    size_t len = aprsFrameBuild(frame, sizeof(frame), APRS_TOCALL, "N0CALL-7",
                                APRS_DEFAULT_PATH, info, strlen(info));
    REQUIRE(len > 0);

    std::vector<rawFrame> frames = loopback(frame, len);
    REQUIRE(frames.size() == 1);

    struct aprsPacket *pkt = aprsPktFromFrame(frames[0].data, frames[0].len);
    REQUIRE(pkt != NULL);
    REQUIRE(pkt->addressesLen == 4);
    REQUIRE(std::string(pkt->addresses[0].addr) == std::string(APRS_TOCALL));
    REQUIRE(std::string(pkt->addresses[1].addr) == std::string("N0CALL"));
    REQUIRE(pkt->addresses[1].ssid == 7);
    REQUIRE(std::string(pkt->info) == std::string(info));
    free(pkt);
}

TEST_CASE("APRS modulator: bit stuffing survives flag-like data", "[aprs][mod]")
{
    /* 0x7e is the flag byte and a run of 0xff is six-plus ones: without bit
     * stuffing either inside the info field would end the frame early or be
     * rejected. '~' is 0x7e in ASCII, so this is text a user could type. */
    uint8_t frame[APRS_PACLEN];
    const char info[] = ":W1AW     :~~~~~~~~ and back";
    size_t len = aprsFrameBuild(frame, sizeof(frame), APRS_TOCALL, "N0CALL-7",
                                NULL, info, strlen(info));
    REQUIRE(len > 0);

    std::vector<rawFrame> frames = loopback(frame, len);
    REQUIRE(frames.size() == 1);

    struct aprsPacket *pkt = aprsPktFromFrame(frames[0].data, frames[0].len);
    REQUIRE(pkt != NULL);
    REQUIRE(std::string(pkt->info) == std::string(info));
    free(pkt);
}

TEST_CASE("APRS modulator: back-to-back frames both decode", "[aprs][mod]")
{
    uint8_t a[APRS_PACLEN];
    uint8_t b[APRS_PACLEN];
    const char infoA[] = ":W1AW     :first";
    const char infoB[] = ":W1AW     :second";
    size_t la = aprsFrameBuild(a, sizeof(a), APRS_TOCALL, "N0CALL-7",
                               APRS_DEFAULT_PATH, infoA, strlen(infoA));
    size_t lb = aprsFrameBuild(b, sizeof(b), APRS_TOCALL, "N0CALL-7",
                               APRS_DEFAULT_PATH, infoB, strlen(infoB));
    REQUIRE(la > 0);
    REQUIRE(lb > 0);

    CapturingModulator mod;
    mod.init();
    REQUIRE(mod.start() == true);
    mod.sendFlags(100);
    mod.sendFrame(a, la);
    mod.sendFlags(20);
    mod.sendFrame(b, lb);
    mod.sendFlags(20);
    mod.stop();
    mod.terminate();

    std::vector<int16_t> rx = toReceiveBaseband(mod.samples);
    std::vector<rawFrame> frames = demodulate(rx);
    REQUIRE(frames.size() == 2);

    struct aprsPacket *p0 = aprsPktFromFrame(frames[0].data, frames[0].len);
    struct aprsPacket *p1 = aprsPktFromFrame(frames[1].data, frames[1].len);
    REQUIRE(p0 != NULL);
    REQUIRE(p1 != NULL);
    REQUIRE(std::string(p0->info) == std::string(infoA));
    REQUIRE(std::string(p1->info) == std::string(infoB));
    free(p0);
    free(p1);
}

TEST_CASE("APRS modulator: transmit level stays inside the sample range",
          "[aprs][mod]")
{
    uint8_t frame[APRS_PACLEN];
    const char info[] = ":W1AW     :level check";
    size_t len = aprsFrameBuild(frame, sizeof(frame), APRS_TOCALL, "N0CALL-7",
                                APRS_DEFAULT_PATH, info, strlen(info));
    REQUIRE(len > 0);

    CapturingModulator mod;
    mod.init();
    REQUIRE(mod.start() == true);
    mod.sendFlags(50);
    mod.sendFrame(frame, len);
    mod.stop();
    mod.terminate();

    int32_t peak = 0;
    for (int16_t s : mod.samples) {
        const int32_t mag = (s < 0) ? -(int32_t)s : (int32_t)s;
        if (mag > peak)
            peak = mag;
    }

    /* The tone must reach the configured level — a modulator that silently
     * under-drives produces a signal no receiver can decode — and must not
     * exceed it, because clipping here becomes splatter on the air. */
    REQUIRE(peak <= APRS::Modulator::TX_AMPLITUDE);
    REQUIRE(peak > ((APRS::Modulator::TX_AMPLITUDE * 9) / 10));
}

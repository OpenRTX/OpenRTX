/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Layered Horse modem loopback:
 *   a. bytes <-> symbols (no filtering)
 *   b. RRC TX/RX sampled at the known group delay
 *   c. demodulator timing recovery
 *   d. full transmission (preamble, LSF, signatures, voice, EOT)
 */

#include "protocols/horse/HorseFrameEncoder.hpp"
#include "protocols/horse/HorseFrameDecoder.hpp"
#include "protocols/horse/HorseModulator.hpp"
#include "protocols/horse/HorseDemodulator.hpp"
#include "protocols/horse/HorseConstants.hpp"
#include "protocols/horse/HorseUtils.hpp"
#include "protocols/M17/DSP.hpp"
#include "core/fir.hpp"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace horse;

static constexpr size_t SPS_48 = 48000 / SYMBOL_RATE;
static constexpr size_t SPS_24 = 24000 / SYMBOL_RATE;
static constexpr size_t TX_DELAY_48 = 40; /* (81-1)/2 taps */
static constexpr size_t RX_DELAY_24 = 20; /* (41-1)/2 taps */

struct impair_t {
    float noise;
    float gain;
    bool invert;
    float dc;
    float rate_ppm;
    size_t drop_start;
};

struct decoded_t {
    HorseFrameType type;
    uint16_t fn;
    uint8_t sync0;
    uint8_t sync1;
    uint8_t payload[12];
    uint8_t tag[4];
    uint8_t flags;
};

static int test_layer_a_bytes_symbols()
{
    for (unsigned v = 0; v < 256; v++) {
        uint8_t b = static_cast<uint8_t>(v);
        auto sym = byteToSymbols(b);
        std::array<uint8_t, 1> packed{ { 0 } };
        for (size_t i = 0; i < 4; i++)
            setSymbol(packed, i, sym[i]);
        if (packed[0] != b) {
            std::printf("layer a: byte 0x%02x round-trip 0x%02x\n", b,
                        packed[0]);
            return -1;
        }
    }

    const int8_t lsf_expect[8] = { +3, +3, -1, -1, -1, -1, +3, -3 };
    const int8_t voice_expect[8] = { +3, -3, -3, -1, -1, +3, -1, -3 };
    const int8_t eot_expect[8] = { +1, -3, -3, +1, -3, +3, -1, +1 };
    auto lsf = syncwordSymbols(LSF_SYNC_WORD);
    auto voice = syncwordSymbols(VOICE_SYNC_WORD);
    auto eot = syncwordSymbols(EOT_SYNC_WORD);
    for (size_t i = 0; i < 8; i++) {
        if (lsf[i] != lsf_expect[i] || voice[i] != voice_expect[i]
            || eot[i] != eot_expect[i]) {
            std::printf("layer a: syncword table mismatch at %zu\n", i);
            return -1;
        }
    }

    HorseFrameEncoder enc;
    frame_t frame{};
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    enc.encodeLsf(src, dst, nullptr, 0, frame);
    frame_t rebuilt{};
    size_t pos = 0;
    for (uint8_t byte : frame) {
        auto s = byteToSymbols(byte);
        for (int8_t sy : s)
            setSymbol(rebuilt, pos++, sy);
    }
    if (rebuilt != frame) {
        std::printf("layer a: LSF frame symbol inverse failed\n");
        return -1;
    }
    std::printf("layer a: bytes<->symbols OK\n");
    return 0;
}

static void impair_48k(const int16_t *in, size_t n, const impair_t &p,
                       std::vector<int16_t> &out)
{
    out.clear();
    size_t start = p.drop_start;
    if (start > n)
        start = n;
    double pos = static_cast<double>(start);
    double step = 1.0 + (p.rate_ppm / 1.0e6);
    unsigned rng = 1u;
    while (pos < static_cast<double>(n)) {
        size_t i = static_cast<size_t>(pos);
        float s = static_cast<float>(in[i]);
        s *= p.gain;
        if (p.invert)
            s = -s;
        s += p.dc;
        rng = rng * 1103515245u + 12345u;
        float nse = (static_cast<float>(rng & 0xFFFFu) / 32768.0f - 1.0f)
                  * p.noise;
        s += nse;
        if (s > 32767.0f)
            s = 32767.0f;
        if (s < -32768.0f)
            s = -32768.0f;
        out.push_back(static_cast<int16_t>(s));
        pos += step;
    }
}

static int render_frames(const std::vector<frame_t> &frames,
                         std::vector<int16_t> &bb48, bool preamble)
{
    HorseModulator mod;
    const size_t per = HorseModulator::captureSamplesPerFrame();
    size_t extra = preamble ? 2 : 0;
    /* One extra all-zero frame flushes the 81-tap TX RRC (group delay 40). */
    bb48.assign((frames.size() + extra + 2) * per, 0);
    mod.init();
    if (!mod.start())
        return -1;
    mod.beginCapture(bb48.data(), bb48.size());
    if (preamble)
        mod.sendPreamble();
    for (const auto &f : frames)
        mod.sendFrame(f);
    frame_t flush{};
    mod.sendFrame(flush);
    bb48.resize(mod.captureLength());
    mod.endCapture();
    mod.stop();
    mod.terminate();
    return 0;
}

static int8_t slice_symbol(int16_t sample, int16_t outer)
{
    if (outer < 1)
        outer = 1;
    return quantizeLevel(sample, outer, static_cast<int16_t>(-outer));
}

static int test_layer_b_rrc_known_phase()
{
    HorseFrameEncoder enc;
    frame_t frame{};
    uint8_t melpe[12];
    uint8_t tag[4] = { 9, 8, 7, 6 };
    memset(melpe, 0xA5, sizeof melpe);
    enc.encodeVoiceFrame(melpe, tag, frame, false);

    std::vector<int16_t> bb48;
    if (render_frames({ frame }, bb48, false) != 0)
        return -1;

    std::vector<int16_t> rx24;
    Fir<41> rxRrc(M17::rrc_taps_24k);
    for (size_t i = 0; i + 1 < bb48.size(); i += 2)
        rx24.push_back(
            static_cast<int16_t>(rxRrc(static_cast<float>(bb48[i]))));

    const size_t first = RX_DELAY_24 + TX_DELAY_48 / 2;
    if (first + FRAME_SYMBOLS * SPS_24 > rx24.size()) {
        std::printf("layer b: not enough samples (%zu)\n", rx24.size());
        return -1;
    }

    int16_t outer = 1;
    for (size_t s = 0; s < FRAME_SYMBOLS; s++) {
        int16_t v = rx24[first + s * SPS_24];
        if (std::abs(v) > outer)
            outer = static_cast<int16_t>(std::abs(v));
    }

    frame_t rebuilt{};
    for (size_t s = 0; s < FRAME_SYMBOLS; s++) {
        int8_t sy = slice_symbol(rx24[first + s * SPS_24], outer);
        setSymbol(rebuilt, s, sy);
    }
    size_t errs = 0;
    for (size_t i = 0; i < rebuilt.size(); i++)
        if (rebuilt[i] != frame[i])
            errs++;
    if (errs != 0) {
        std::printf("layer b: %zu byte errors, outer=%d first=%zu\n", errs,
                    outer, first);
        return -1;
    }
    std::printf("layer b: RRC known-phase SER=0 outer=%d\n", outer);
    return 0;
}

static int demod_stream(const std::vector<int16_t> &rx24, bool invert,
                        std::vector<decoded_t> &out, bool skip_dc = true)
{
    HorseDemodulator demod;
    HorseFrameDecoder decoder;
    demod.init();
    demod.resetImmediate();
    /*
     * Default skip_dc=true: dsp_dcBlockFilter left-shifts a negative
     * int16_t (dsp.cpp:19), which is UB. The firmware still runs that
     * filter. test_layer_dc_block() sets skip_dc=false. That path is
     * registered in the unsanitized meson suite; under UBSan it aborts
     * on the upstream shift (see UPSTREAM_ISSUE_dsp.md). Do not patch
     * dsp.cpp in this fork.
     */
    demod.setSkipDcBlock(skip_dc);
    out.clear();
    for (int16_t s : rx24) {
        demod.feedSample(s, invert);
        frame_t frame;
        if (!demod.takeFrame(frame))
            continue;
        decoded_t d{};
        d.sync0 = frame[0];
        d.sync1 = frame[1];
        d.type = decoder.decodeFrame(frame);
        if (d.type == HorseFrameType::VOICE)
            decoder.getVoicePayload(frame, d.payload, d.tag, &d.fn);
        out.push_back(d);
    }
    demod.terminate();
    return 0;
}

static void to_24k(const std::vector<int16_t> &bb48, std::vector<int16_t> &rx24)
{
    rx24.clear();
    for (size_t i = 0; i + 1 < bb48.size(); i += 2)
        rx24.push_back(bb48[i]);
}

static int test_layer_c_demod_timing()
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames(3);
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    uint8_t melpe[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(melpe, 0x11, sizeof melpe);
    enc.encodeLsf(src, dst, nullptr, 0, frames[0]);
    enc.encodeVoiceFrame(melpe, tag, frames[1], false);
    enc.encodeEotFrame(frames[2]);

    std::vector<int16_t> bb48, rx24;
    if (render_frames(frames, bb48, true) != 0)
        return -1;
    to_24k(bb48, rx24);
    std::vector<decoded_t> got;
    demod_stream(rx24, false, got);

    bool have_lsf = false, have_voice = false, have_eot = false;
    for (const auto &d : got) {
        if (d.type == HorseFrameType::LINK_SETUP)
            have_lsf = true;
        if (d.type == HorseFrameType::VOICE && d.fn < SIG_FRAME_BASE)
            have_voice = true;
        if (d.type == HorseFrameType::EOT)
            have_eot = true;
    }
    if (!have_lsf || !have_voice || !have_eot) {
        std::printf("layer c: decoded %zu frames lsf=%d voice=%d eot=%d\n",
                    got.size(), have_lsf, have_voice, have_eot);
        for (size_t i = 0; i < got.size(); i++)
            std::printf("  [%zu] type %u fn %u sync %02x %02x\n", i,
                        static_cast<unsigned>(got[i].type), got[i].fn,
                        got[i].sync0, got[i].sync1);
        return -1;
    }
    std::printf("layer c: demod timing recovered LSF+voice+EOT (%zu frames)\n",
                got.size());
    return 0;
}

static int test_layer_dc_block()
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames(3);
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    uint8_t melpe[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(melpe, 0x11, sizeof melpe);
    enc.encodeLsf(src, dst, nullptr, 0, frames[0]);
    enc.encodeVoiceFrame(melpe, tag, frames[1], false);
    enc.encodeEotFrame(frames[2]);

    std::vector<int16_t> bb48, rx24;
    if (render_frames(frames, bb48, true) != 0)
        return -1;
    to_24k(bb48, rx24);
    std::vector<decoded_t> got;
    demod_stream(rx24, false, got, false);

    bool have_lsf = false, have_voice = false, have_eot = false;
    for (const auto &d : got) {
        if (d.type == HorseFrameType::LINK_SETUP)
            have_lsf = true;
        if (d.type == HorseFrameType::VOICE && d.fn < SIG_FRAME_BASE)
            have_voice = true;
        if (d.type == HorseFrameType::EOT)
            have_eot = true;
    }
    if (!have_lsf || !have_voice || !have_eot) {
        std::printf("dc-block: decoded %zu frames lsf=%d voice=%d eot=%d\n",
                    got.size(), have_lsf, have_voice, have_eot);
        return -1;
    }
    std::printf("dc-block: firmware DC path recovered LSF+voice+EOT\n");
    return 0;
}

static int test_layer_d_full_tx()
{
    HorseFrameEncoder enc;
    call_t src = { { 9, 8, 7, 6, 5, 4 } };
    call_t dst = { { 1, 1, 1, 1, 1, 1 } };
    std::vector<frame_t> frames;
    frames.resize(1 + SIG_FRAME_COUNT + 300 + 1);

    uint8_t eph[32];
    memset(eph, 0x22, sizeof eph);
    enc.encodeLsf(src, dst, eph, LSF_FLAG_ENCRYPTED | LSF_FLAG_SIGNED,
                  frames[0]);

    uint8_t sig[64];
    uint8_t zero[4] = { 0 };
    for (size_t i = 0; i < sizeof sig; i++)
        sig[i] = static_cast<uint8_t>(i);
    for (uint16_t i = 0; i < SIG_FRAME_COUNT; i++)
        enc.encodeVoiceFrameWithFn(sig + i * SIG_CHUNK_BYTES, zero,
                                   SIG_FRAME_BASE + i, frames[1 + i], false,
                                   sig_chunk_bytes(i));

    uint8_t melpe[12];
    uint8_t tag[4] = { 4, 3, 2, 1 };
    memset(melpe, 0x33, sizeof melpe);
    for (int i = 0; i < 300; i++)
        enc.encodeVoiceFrame(melpe, tag, frames[1 + SIG_FRAME_COUNT + i],
                             i == 299);
    enc.encodeEotFrame(frames.back());

    std::vector<int16_t> bb48, rx24;
    if (render_frames(frames, bb48, true) != 0)
        return -1;
    to_24k(bb48, rx24);
    std::vector<decoded_t> got;
    demod_stream(rx24, false, got);

    size_t nlsf = 0, nsig = 0, nvoice = 0, neot = 0;
    uint8_t rebuilt[64];
    memset(rebuilt, 0, sizeof rebuilt);
    for (const auto &d : got) {
        if (d.type == HorseFrameType::LINK_SETUP)
            nlsf++;
        else if (d.type == HorseFrameType::EOT)
            neot++;
        else if (d.type == HorseFrameType::VOICE) {
            if (d.fn >= SIG_FRAME_BASE
                && d.fn < SIG_FRAME_BASE + SIG_FRAME_COUNT) {
                uint16_t c = static_cast<uint16_t>(d.fn - SIG_FRAME_BASE);
                memcpy(rebuilt + c * SIG_CHUNK_BYTES, d.payload,
                       sig_chunk_bytes(c));
                nsig++;
            } else
                nvoice++;
        }
    }
    if (nlsf < 1 || nsig < SIG_FRAME_COUNT || nvoice < 300 || neot < 1) {
        std::printf(
            "layer d: lsf=%zu sig=%zu voice=%zu eot=%zu (decoded %zu)\n", nlsf,
            nsig, nvoice, neot, got.size());
        return -1;
    }
    if (memcmp(rebuilt, sig, sizeof sig) != 0) {
        std::printf("layer d: signature bytes mismatch\n");
        return -1;
    }
    std::printf("layer d: full TX OK (lsf=%zu sig=%zu voice=%zu eot=%zu)\n",
                nlsf, nsig, nvoice, neot);
    return 0;
}

static int count_good_frames(const impair_t &p, bool demod_invert)
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames(3);
    call_t src = { { 2, 2, 2, 2, 2, 2 } };
    call_t dst = { { 3, 3, 3, 3, 3, 3 } };
    uint8_t melpe[12];
    uint8_t tag[4] = { 0 };
    memset(melpe, 0x44, sizeof melpe);
    enc.encodeLsf(src, dst, nullptr, 0, frames[0]);
    enc.encodeVoiceFrame(melpe, tag, frames[1], false);
    enc.encodeEotFrame(frames[2]);
    std::vector<int16_t> bb48, imp48, rx24;
    if (render_frames(frames, bb48, true) != 0)
        return -1;
    impair_48k(bb48.data(), bb48.size(), p, imp48);
    to_24k(imp48, rx24);
    std::vector<decoded_t> got;
    demod_stream(rx24, demod_invert, got);
    bool lsf = false, voice = false, eot = false;
    for (const auto &d : got) {
        if (d.type == HorseFrameType::LINK_SETUP &&
            d.sync0 == LSF_SYNC_WORD[0] && d.sync1 == LSF_SYNC_WORD[1])
            lsf = true;
        if (d.type == HorseFrameType::VOICE &&
            d.sync0 == VOICE_SYNC_WORD[0] && d.sync1 == VOICE_SYNC_WORD[1] &&
            memcmp(d.payload, melpe, 12) == 0)
            voice = true;
        if (d.type == HorseFrameType::EOT && d.sync0 == EOT_SYNC_WORD[0] &&
            d.sync1 == EOT_SYNC_WORD[1])
            eot = true;
    }
    return (lsf ? 1 : 0) + (voice ? 1 : 0) + (eot ? 1 : 0);
}

static int test_impairments()
{
    impair_t base{};
    base.gain = 1.0f;
    if (count_good_frames(base, false) < 3) {
        std::printf("impair: clean channel failed\n");
        return -1;
    }

    float noise_fail = -1.0f;
    for (float n = 500.0f; n <= 20000.0f; n += 500.0f) {
        impair_t p = base;
        p.noise = n;
        if (count_good_frames(p, false) < 3) {
            noise_fail = n;
            break;
        }
    }

    float ppm_fail = -1.0f;
    for (float ppm = 50.0f; ppm <= 800.0f; ppm += 50.0f) {
        impair_t p = base;
        p.rate_ppm = ppm;
        if (count_good_frames(p, false) < 3) {
            ppm_fail = ppm;
            break;
        }
    }

    impair_t inv = base;
    inv.invert = true;
    int inv_auto = count_good_frames(inv, false);
    int inv_flag = count_good_frames(inv, true);

    impair_t dc = base;
    dc.dc = 500.0f;
    int dc_good = count_good_frames(dc, false);

    impair_t gain = base;
    gain.gain = 0.25f;
    int gain_good = count_good_frames(gain, false);

    impair_t trunc = base;
    trunc.drop_start = HorseModulator::captureSamplesPerFrame();
    int trunc_good = count_good_frames(trunc, false);

    std::printf("impair: first_noise_fail=%.0f first_ppm_fail=%.0f "
                "invert_no_flag=%d/3 invert_with_flag=%d/3 dc500=%d/3 "
                "gain0.25=%d/3 drop1frame=%d/3\n",
                noise_fail, ppm_fail, inv_auto, inv_flag, dc_good, gain_good,
                trunc_good);
    return 0;
}

int main()
{
    if (test_layer_a_bytes_symbols() != 0)
        return -1;
    if (test_layer_b_rrc_known_phase() != 0)
        return -1;
    if (test_layer_c_demod_timing() != 0)
        return -1;
    if (test_layer_dc_block() != 0)
        return -1;
    if (test_layer_d_full_tx() != 0)
        return -1;
    if (test_impairments() != 0)
        return -1;
    std::printf("horse_loopback: all tests passed\n");
    return 0;
}

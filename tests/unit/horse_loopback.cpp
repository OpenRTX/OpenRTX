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
#include "protocols/horse/horse_crypto.h"
#ifdef HAVE_LIBSODIUM
#include <sodium.h>
#endif
#include "protocols/M17/DSP.hpp"
#include "core/fir.hpp"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>

using namespace horse;

static void push_lsf(HorseFrameEncoder &enc, const call_t &src, const call_t &dst,
                     const uint8_t *eph, uint8_t flags, std::vector<frame_t> &frames)
{
    frame_t lsf[LSF_OPENING_FRAMES];
    enc.encodeLsf(src, dst, eph, flags, lsf);
    for (size_t i = 0; i < LSF_OPENING_FRAMES; i++)
        frames.push_back(lsf[i]);
}


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
    unsigned seed;
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
    frame_t lsf3[LSF_OPENING_FRAMES];
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    enc.encodeLsf(src, dst, nullptr, 0, lsf3);
    frame_t frame = lsf3[0];
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
    unsigned rng = p.seed != 0u ? p.seed : 1u;
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
     * on the upstream shift (see docs/horse/UPSTREAM_ISSUE_dsp.md). Do not patch
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
        if (d.type == HorseFrameType::LINK_SETUP && decoder.lsfReady())
            demod.noteValidTag();
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

int horse_render_frames(const std::vector<frame_t> &frames,
                        std::vector<int16_t> &bb48, bool preamble)
{
    return render_frames(frames, bb48, preamble);
}

void horse_to_24k(const std::vector<int16_t> &bb48, std::vector<int16_t> &rx24)
{
    to_24k(bb48, rx24);
}

static int test_layer_c_demod_timing()
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    uint8_t melpe[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(melpe, 0x11, sizeof melpe);
    push_lsf(enc, src, dst, nullptr, 0, frames);
    { frame_t _v; enc.encodeVoiceFrame(melpe, tag, _v, false); frames.push_back(_v); }
    { frame_t _e; enc.encodeEotFrame(_e); frames.push_back(_e); }

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

static int test_no_late_entry()
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames;
    uint8_t melpe[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(melpe, 0x11, sizeof melpe);
    { frame_t _v; enc.encodeVoiceFrame(melpe, tag, _v, false); frames.push_back(_v); }
    { frame_t _e; enc.encodeEotFrame(_e); frames.push_back(_e); }
    std::vector<int16_t> bb48, rx24;
    if (render_frames(frames, bb48, true) != 0)
        return -1;
    to_24k(bb48, rx24);
    std::vector<decoded_t> got;
    demod_stream(rx24, false, got);
    for (const auto &d : got) {
        if (d.type == HorseFrameType::VOICE || d.type == HorseFrameType::EOT) {
            std::printf("late-entry: locked without LSF type %u\n",
                        static_cast<unsigned>(d.type));
            return -1;
        }
    }
    std::printf("late-entry: no lock without LSF\n");
    return 0;
}

/* Open-discriminator FM: Gaussian, sigma 10000, clipped to int16. */
static int16_t fm_open_noise(unsigned &rng)
{
    rng = rng * 1103515245u + 12345u;
    float u1 = static_cast<float>((rng >> 8) & 0xFFFFu) / 65536.0f + 1.0e-6f;
    rng = rng * 1103515245u + 12345u;
    float u2 = static_cast<float>((rng >> 8) & 0xFFFFu) / 65536.0f;
    float g = std::sqrt(-2.0f * std::log(u1))
            * std::cos(2.0f * 3.14159265f * u2);
    float s = g * 10000.0f;
    if (s > 32767.0f)
        s = 32767.0f;
    if (s < -32768.0f)
        s = -32768.0f;
    return static_cast<int16_t>(s);
}

struct false_lock_row_t {
    uint8_t hd;
    int32_t peak;
    size_t demod_locks;
    size_t lsf_locks;
    size_t samples;
};

static false_lock_row_t
count_false_locks(uint8_t hd, int32_t peak, size_t samples, unsigned seed,
                  std::vector<int32_t> *lock_peaks = nullptr, float gain = 1.0f)
{
    false_lock_row_t r{};
    r.hd = hd;
    r.peak = peak;
    r.samples = samples;
    HorseDemodulator demod;
    HorseFrameDecoder decoder;
    demod.init();
    demod.resetImmediate();
    demod.setSkipDcBlock(true);
    demod.setAcquireHamming(hd);
    demod.setCorrPeakMin(peak);
    unsigned rng = seed;
    for (size_t i = 0; i < samples; i++) {
        bool was_locked = demod.isLocked();
        float v = static_cast<float>(fm_open_noise(rng)) * gain;
        if (v > 32767.0f)
            v = 32767.0f;
        if (v < -32768.0f)
            v = -32768.0f;
        demod.feedSample(static_cast<int16_t>(v), false);
        if (!was_locked && demod.isLocked()) {
            r.demod_locks++;
            if (lock_peaks != nullptr)
                lock_peaks->push_back(demod.lastLockCorrAbs());
        }
        frame_t frame;
        if (!demod.takeFrame(frame))
            continue;
        if (decoder.decodeFrame(frame) == HorseFrameType::LINK_SETUP)
            r.lsf_locks++;
    }
    demod.terminate();
    return r;
}

static void print_pctiles(const char *label, std::vector<int32_t> v)
{
    if (v.empty()) {
        std::printf("%s: n=0\n", label);
        return;
    }
    std::sort(v.begin(), v.end());
    size_t p5 = (v.size() * 5u) / 100u;
    std::printf("%s: n=%zu min=%d p5=%d p50=%d max=%d\n", label, v.size(),
                v.front(), v[p5], v[v.size() / 2], v.back());
}

static void collect_lock_peaks(const std::vector<int16_t> &rx24,
                               std::vector<int32_t> &peaks)
{
    HorseDemodulator demod;
    demod.init();
    demod.resetImmediate();
    demod.setSkipDcBlock(true);
    demod.setCorrPeakMin(0);
    for (int16_t s : rx24) {
        bool was = demod.isLocked();
        demod.feedSample(s, false);
        if (!was && demod.isLocked())
            peaks.push_back(demod.lastLockCorrAbs());
    }
    demod.terminate();
}

static int test_corr_peak_distributions()
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    uint8_t melpe[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(melpe, 0x11, sizeof melpe);
    push_lsf(enc, src, dst, nullptr, 0, frames);
    { frame_t _v; enc.encodeVoiceFrame(melpe, tag, _v, false); frames.push_back(_v); }
    { frame_t _e; enc.encodeEotFrame(_e); frames.push_back(_e); }

    std::vector<int16_t> bb48;
    if (render_frames(frames, bb48, true) != 0)
        return -1;

    const float gains[] = { 0.25f, 0.5f, 1.0f, 2.0f };
    int32_t min_real = 0x7fffffff;
    int32_t max_false = 0;
    bool any_false = false;

    const bool long_noise = std::getenv("HORSE_FALSE_LOCK_LONG") != nullptr;
    const size_t noise_s = long_noise ? (24000u * 600u) : (24000u * 10u);

    for (float g : gains) {
        impair_t p{};
        p.gain = g;
        std::vector<int16_t> imp48;
        std::vector<int16_t> rx24;
        std::vector<int32_t> real_peaks;
        impair_48k(bb48.data(), bb48.size(), p, imp48);
        to_24k(imp48, rx24);
        for (int i = 0; i < 8; i++)
            collect_lock_peaks(rx24, real_peaks);
        char lab[80];
        std::snprintf(lab, sizeof lab, "real LSF gain=%.2f (ncc Q12)", g);
        print_pctiles(lab, real_peaks);
        for (int32_t x : real_peaks)
            if (x < min_real)
                min_real = x;

        std::vector<int32_t> noise_peaks;
        count_false_locks(0, 0, noise_s, 7u, &noise_peaks, g);
        std::snprintf(lab, sizeof lab,
                      "false lock gain=%.2f Hamming0 (ncc Q12)", g);
        print_pctiles(lab, noise_peaks);
        for (int32_t x : noise_peaks) {
            any_false = true;
            if (x > max_false)
                max_false = x;
        }
    }

    if (!any_false || max_false >= min_real) {
        std::printf("ncc Q12: overlap or no false locks "
                    "(false_max=%d real_min=%d); CORR_PEAK_MIN=%d\n",
                    max_false, min_real, CORR_PEAK_MIN);
    } else {
        std::printf("ncc Q12: separate false_max=%d real_min=%d "
                    "CORR_PEAK_MIN=%d\n",
                    max_false, min_real, CORR_PEAK_MIN);
    }
    return 0;
}

static int test_lsf_false_lock_noise()
{
    const bool minute = std::getenv("HORSE_FALSE_LOCK_MINUTE") != nullptr;
    const size_t samples = minute ? (24000u * 60u) : (24000u * 2u);
    const unsigned seed = 1u;
    false_lock_row_t rows[3];
    rows[0] = count_false_locks(0, 0, samples, seed);
    rows[1] = count_false_locks(1, 0, samples, seed);
    rows[2] = count_false_locks(0, CORR_PEAK_MIN, samples, seed);

    std::printf("false-lock table (%zu s, FM-open Gaussian sigma=10000):\n",
                samples / 24000u);
    std::printf("  Hamming  peakMin  demod/min  LSF/min\n");
    size_t best_i = 0;
    double best_rate = 1e9;
    for (size_t i = 0; i < 3; i++) {
        double minutes = static_cast<double>(rows[i].samples) / 24000.0 / 60.0;
        double dpm = rows[i].demod_locks / minutes;
        double lpm = rows[i].lsf_locks / minutes;
        std::printf("  %7u  %7d  %9.2f  %7.2f\n", rows[i].hd, rows[i].peak, dpm,
                    lpm);
        if (dpm < best_rate) {
            best_rate = dpm;
            best_i = i;
        }
    }
    std::printf("false-lock: compile HAMMING_ACQUIRE_MAX=%u CORR_PEAK_MIN=%d "
                "(table best Hamming %u peak %d)\n",
                HAMMING_ACQUIRE_MAX, CORR_PEAK_MIN, rows[best_i].hd,
                rows[best_i].peak);
    if (rows[0].lsf_locks > (minute ? 30u : 4u))
        return -1;
    return 0;
}

static int test_tx_during_false_lock()
{
    HorseDemodulator demod;
    HorseFrameDecoder decoder;
    demod.init();
    demod.resetImmediate();
    demod.setSkipDcBlock(true);
    demod.setDropWithoutTag(true);
    demod.setAcquireHamming(1);
    demod.setCorrPeakMin(0);

    unsigned rng = 99u;
    size_t i = 0;
    const size_t cap = 24000u * 3u;
    for (; i < cap; i++) {
        demod.feedSample(fm_open_noise(rng), false);
        frame_t dump;
        (void)demod.takeFrame(dump);
        if (demod.isLocked())
            break;
    }
    if (!demod.isLocked()) {
        std::printf("tx-during-false-lock: no Hamming-1 false lock in 3s\n");
        return -1;
    }
    demod.setAcquireHamming(HAMMING_ACQUIRE_MAX);
    demod.setCorrPeakMin(CORR_PEAK_MIN);

    HorseFrameEncoder enc;
    std::vector<frame_t> frames;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    uint8_t melpe[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(melpe, 0x22, sizeof melpe);
    push_lsf(enc, src, dst, nullptr, 0, frames);
    { frame_t _v; enc.encodeVoiceFrame(melpe, tag, _v, false); frames.push_back(_v); }
    { frame_t _e; enc.encodeEotFrame(_e); frames.push_back(_e); }
    std::vector<int16_t> bb48, rx24;
    if (render_frames(frames, bb48, true) != 0)
        return -1;
    to_24k(bb48, rx24);
    /* Real TX starts during the false lock; drop window eats the first
     * frames, so pad with enough samples that LSF remains after unlock. */
    const size_t pad = static_cast<size_t>(LOCK_NO_TAG_FRAMES + 4)
                     * FRAME_SYMBOLS * 5u;
    std::vector<int16_t> stream(pad, 0);
    stream.insert(stream.end(), rx24.begin(), rx24.end());

    bool have_lsf = false;
    bool have_voice = false;
    for (int16_t s : stream) {
        demod.feedSample(s, false);
        frame_t frame;
        if (!demod.takeFrame(frame))
            continue;
        HorseFrameType t = decoder.decodeFrame(frame);
        if (t == HorseFrameType::LINK_SETUP)
            have_lsf = true;
        if (t == HorseFrameType::VOICE) {
            uint8_t payload[12];
            uint8_t vtag[4];
            uint16_t fn = 0;
            decoder.getVoicePayload(frame, payload, vtag, &fn);
            if (fn < SIG_FRAME_BASE) {
                have_voice = true;
                demod.noteValidTag();
            }
        }
    }
    demod.terminate();
    if (!have_lsf || !have_voice) {
        std::printf("tx-during-false-lock: lsf=%d voice=%d after drop\n",
                    have_lsf, have_voice);
        return -1;
    }
    std::printf("tx-during-false-lock: real LSF+voice after false lock\n");
    return 0;
}

static int run_lsf_into_false_lock(unsigned frames_in)
{
    HorseDemodulator demod;
    HorseFrameDecoder decoder;
    demod.init();
    demod.resetImmediate();
    demod.setSkipDcBlock(true);
    demod.setDropWithoutTag(true);
    demod.setAcquireHamming(1);
    demod.setCorrPeakMin(0);

    unsigned rng = 99u + frames_in;
    const size_t cap = 24000u * 3u;
    for (size_t i = 0; i < cap; i++) {
        demod.feedSample(fm_open_noise(rng), false);
        frame_t dump;
        (void)demod.takeFrame(dump);
        if (demod.isLocked())
            break;
    }
    if (!demod.isLocked()) {
        std::printf("lsf-replace: no false lock for %u frames-in\n", frames_in);
        return -1;
    }
    demod.setAcquireHamming(HAMMING_ACQUIRE_MAX);
    demod.setCorrPeakMin(CORR_PEAK_MIN);

    const size_t wait_n = static_cast<size_t>(frames_in) * FRAME_SYMBOLS * 5u;
    for (size_t n = 0; n < wait_n; n++) {
        demod.feedSample(fm_open_noise(rng), false);
        frame_t dump;
        (void)demod.takeFrame(dump);
    }

    HorseFrameEncoder enc;
    std::vector<frame_t> frames;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    uint8_t melpe[12];
    uint8_t tag[4] = { 9, 8, 7, 6 };
    memset(melpe, 0x55, sizeof melpe);
    push_lsf(enc, src, dst, nullptr, 0, frames);
    { frame_t _v; enc.encodeVoiceFrame(melpe, tag, _v, false); frames.push_back(_v); }
    { frame_t _v; enc.encodeVoiceFrame(melpe, tag, _v, true); frames.push_back(_v); }
    { frame_t _e; enc.encodeEotFrame(_e); frames.push_back(_e); }
    std::vector<int16_t> bb48;
    std::vector<int16_t> rx24;
    if (render_frames(frames, bb48, true) != 0) {
        demod.terminate();
        return -1;
    }
    to_24k(bb48, rx24);

    bool have_lsf = false;
    bool have_voice = false;
    bool have_eot = false;
    unsigned nvoice = 0;
    for (int16_t s : rx24) {
        demod.feedSample(s, false);
        frame_t frame;
        if (!demod.takeFrame(frame))
            continue;
        HorseFrameType t = decoder.decodeFrame(frame);
        if (t == HorseFrameType::LINK_SETUP)
            have_lsf = true;
        if (t == HorseFrameType::VOICE) {
            uint8_t payload[12];
            uint8_t vtag[4];
            uint16_t fn = 0;
            decoder.getVoicePayload(frame, payload, vtag, &fn);
            if (fn < SIG_FRAME_BASE && memcmp(payload, melpe, 12) == 0) {
                have_voice = true;
                nvoice++;
                demod.noteValidTag();
            }
        }
        if (t == HorseFrameType::EOT)
            have_eot = true;
    }
    demod.terminate();
    if (!have_lsf || nvoice < 2 || !have_eot) {
        std::printf("lsf-replace @%u: lsf=%d voice=%u eot=%d\n", frames_in,
                    have_lsf, nvoice, have_eot);
        return -1;
    }
    std::printf("lsf-replace: full RX with LSF %u frames into false lock\n",
                frames_in);
    (void)have_voice;
    return 0;
}

static int test_lsf_replaces_unauth_lock()
{
    const unsigned offs[] = { 1, 4, 7 };
    for (unsigned f : offs) {
        if (run_lsf_into_false_lock(f) != 0)
            return -1;
    }
    return 0;
}

static int test_layer_dc_block()
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    uint8_t melpe[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(melpe, 0x11, sizeof melpe);
    push_lsf(enc, src, dst, nullptr, 0, frames);
    { frame_t _v; enc.encodeVoiceFrame(melpe, tag, _v, false); frames.push_back(_v); }
    { frame_t _e; enc.encodeEotFrame(_e); frames.push_back(_e); }

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

    uint8_t eph[32];
    memset(eph, 0x22, sizeof eph);
    push_lsf(enc, src, dst, eph, LSF_FLAG_ENCRYPTED | LSF_FLAG_SIGNED, frames);

    uint8_t sig[64];
    uint8_t zero[4] = { 0 };
    for (size_t i = 0; i < sizeof sig; i++)
        sig[i] = static_cast<uint8_t>(i);
    for (uint16_t i = 0; i < SIG_FRAME_COUNT; i++) {
        frame_t vf{};
        enc.encodeVoiceFrameWithFn(sig + i * SIG_CHUNK_BYTES, zero,
                                   SIG_FRAME_BASE + i, vf, false,
                                   sig_chunk_bytes(i));
        frames.push_back(vf);
    }

    uint8_t melpe[12];
    uint8_t tag[4] = { 4, 3, 2, 1 };
    memset(melpe, 0x33, sizeof melpe);
    for (int i = 0; i < 300; i++) {
        frame_t vf{};
        enc.encodeVoiceFrame(melpe, tag, vf, i == 299);
        frames.push_back(vf);
    }
    { frame_t ef; enc.encodeEotFrame(ef); frames.push_back(ef); }

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
    std::vector<frame_t> frames;
    call_t src = { { 2, 2, 2, 2, 2, 2 } };
    call_t dst = { { 3, 3, 3, 3, 3, 3 } };
    uint8_t melpe[12];
    uint8_t tag[4] = { 0 };
    memset(melpe, 0x44, sizeof melpe);
    push_lsf(enc, src, dst, nullptr, 0, frames);
    { frame_t _v; enc.encodeVoiceFrame(melpe, tag, _v, false); frames.push_back(_v); }
    { frame_t _e; enc.encodeEotFrame(_e); frames.push_back(_e); }
    std::vector<int16_t> bb48, imp48, rx24;
    if (render_frames(frames, bb48, true) != 0)
        return -1;
    impair_48k(bb48.data(), bb48.size(), p, imp48);
    to_24k(imp48, rx24);
    std::vector<decoded_t> got;
    demod_stream(rx24, demod_invert, got);
    bool lsf = false, voice = false, eot = false;
    for (const auto &d : got) {
        if (d.type == HorseFrameType::LINK_SETUP && d.sync0 == LSF_SYNC_WORD[0]
            && d.sync1 == LSF_SYNC_WORD[1])
            lsf = true;
        if (d.type == HorseFrameType::VOICE && d.sync0 == VOICE_SYNC_WORD[0]
            && d.sync1 == VOICE_SYNC_WORD[1]
            && memcmp(d.payload, melpe, 12) == 0)
            voice = true;
        if (d.type == HorseFrameType::EOT && d.sync0 == EOT_SYNC_WORD[0]
            && d.sync1 == EOT_SYNC_WORD[1])
            eot = true;
    }
    return (lsf ? 1 : 0) + (voice ? 1 : 0) + (eot ? 1 : 0);
}

static int collect_real_lsf_trial(const std::vector<int16_t> &rx24,
                                  const uint8_t *melpe, int32_t *ncc, int *good)
{
    HorseDemodulator demod;
    HorseFrameDecoder decoder;
    demod.init();
    demod.resetImmediate();
    demod.setSkipDcBlock(true);
    demod.setCorrPeakMin(0);
    int32_t got = -1;
    bool have_lsf = false;
    bool have_voice = false;
    bool have_eot = false;
    for (int16_t s : rx24) {
        bool was = demod.isLocked();
        demod.feedSample(s, false);
        if (!was && demod.isLocked())
            got = demod.lastLockCorrAbs();
        frame_t frame;
        if (!demod.takeFrame(frame))
            continue;
        HorseFrameType t = decoder.decodeFrame(frame);
        if (t == HorseFrameType::LINK_SETUP && frame[0] == LSF_SYNC_WORD[0]
            && frame[1] == LSF_SYNC_WORD[1]) {
            have_lsf = true;
            if (decoder.lsfReady())
                demod.noteValidTag();
        }
        if (t == HorseFrameType::VOICE) {
            uint8_t payload[12];
            uint8_t vtag[4];
            uint16_t fn = 0;
            decoder.getVoicePayload(frame, payload, vtag, &fn);
            if (fn < SIG_FRAME_BASE && memcmp(payload, melpe, 12) == 0)
                have_voice = true;
        }
        if (t == HorseFrameType::EOT && frame[0] == EOT_SYNC_WORD[0]
            && frame[1] == EOT_SYNC_WORD[1])
            have_eot = true;
    }
    demod.terminate();
    *good = (have_lsf ? 1 : 0) + (have_voice ? 1 : 0) + (have_eot ? 1 : 0);
    if (!have_lsf || got < 0)
        return -1;
    *ncc = got;
    return 0;
}

static int test_lsf_intact_under_noise()
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames;
    call_t src = { { 2, 2, 2, 2, 2, 2 } };
    call_t dst = { { 3, 3, 3, 3, 3, 3 } };
    uint8_t melpe[12];
    uint8_t tag[4] = { 0 };
    memset(melpe, 0x44, sizeof melpe);
    push_lsf(enc, src, dst, nullptr, 0, frames);
    { frame_t _v; enc.encodeVoiceFrame(melpe, tag, _v, false); frames.push_back(_v); }
    { frame_t _e; enc.encodeEotFrame(_e); frames.push_back(_e); }
    std::vector<int16_t> bb48;
    if (render_frames(frames, bb48, true) != 0)
        return -1;

    const float sigmas[] = { 2000.0f, 5000.0f, 10000.0f, 12500.0f, 15000.0f };
    const unsigned n_try = 200u;
    std::printf("uncoded LSF ncc and intact LSF+voice+EOT (gain=1):\n");
    std::printf("  sigma   n  min   p5  p50  max  intact/200\n");

    for (float sig : sigmas) {
        std::vector<int32_t> nccs;
        unsigned intact = 0;
        for (unsigned t = 0; t < n_try; t++) {
            impair_t p{};
            p.gain = 1.0f;
            p.noise = sig;
            p.seed = 1000u + t * 17u + static_cast<unsigned>(sig);
            std::vector<int16_t> imp48;
            std::vector<int16_t> rx24;
            impair_48k(bb48.data(), bb48.size(), p, imp48);
            to_24k(imp48, rx24);
            int32_t ncc = 0;
            int good = 0;
            if (collect_real_lsf_trial(rx24, melpe, &ncc, &good) == 0)
                nccs.push_back(ncc);
            if (good >= 3)
                intact++;
        }
        if (nccs.empty()) {
            std::printf("  %5.0f    0     -    -    -    -  %3u/200\n", sig,
                        intact);
            continue;
        }
        std::sort(nccs.begin(), nccs.end());
        size_t p5n = (nccs.size() * 5u) / 100u;
        std::printf("  %5.0f  %3zu  %4d %4d %4d %4d  %3u/200\n", sig,
                    nccs.size(), nccs.front(), nccs[p5n], nccs[nccs.size() / 2],
                    nccs.back(), intact);
    }

    impair_t base{};
    base.gain = 1.0f;
    base.noise = 12500.0f;
    int at_12500 = count_good_frames(base, false);
    base.noise = 13000.0f;
    int at_13000 = count_good_frames(base, false);
    std::printf("noise baseline at CORR_PEAK_MIN=%d: 12500=%d/3 13000=%d/3 "
                "(pre-threshold: 12500 pass, 13000 fail)\n",
                CORR_PEAK_MIN, at_12500, at_13000);

    const bool long_noise = std::getenv("HORSE_FALSE_LOCK_LONG") != nullptr;
    const size_t noise_s = long_noise ? (24000u * 600u) : (24000u * 10u);
    std::vector<int32_t> false_ncc;
    false_lock_row_t fl = count_false_locks(0, 0, noise_s, 7u, &false_ncc,
                                            1.0f);
    print_pctiles(long_noise ? "false lock Hamming0 gain=1 ncc Q12 (10 min)" :
                               "false lock Hamming0 gain=1 ncc Q12 (10 s)",
                  false_ncc);
    double minutes = static_cast<double>(fl.samples) / 24000.0 / 60.0;
    std::printf("false decoded LSF/min over %.1f min: %.2f (n=%zu); "
                "demod/min %.2f\n",
                minutes, fl.lsf_locks / minutes, fl.lsf_locks,
                fl.demod_locks / minutes);
    return 0;
}

static int test_c20_multiseed()
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames;
    call_t src = { { 2, 2, 2, 2, 2, 2 } };
    call_t dst = { { 3, 3, 3, 3, 3, 3 } };
    uint8_t melpe[12];
    uint8_t tag[4] = { 0 };
    memset(melpe, 0x44, sizeof melpe);
    push_lsf(enc, src, dst, nullptr, 0, frames);
    { frame_t _v; enc.encodeVoiceFrame(melpe, tag, _v, false); frames.push_back(_v); }
    { frame_t _e; enc.encodeEotFrame(_e); frames.push_back(_e); }
    std::vector<int16_t> bb48;
    if (render_frames(frames, bb48, true) != 0)
        return -1;
    const float sig = 10000.0f;
    const unsigned n_try = 200u;
    const unsigned bases[] = { 1000u, 2000u, 3000u, 4000u, 5000u };
    std::printf("C20 multi-seed intact at sigma=10000 (200 trials each):\n");
    unsigned sum = 0;
    for (unsigned base : bases) {
        unsigned intact = 0;
        for (unsigned t = 0; t < n_try; t++) {
            impair_t p{};
            p.gain = 1.0f;
            p.noise = sig;
            p.seed = base + t * 17u + static_cast<unsigned>(sig);
            std::vector<int16_t> imp48, rx24;
            impair_48k(bb48.data(), bb48.size(), p, imp48);
            to_24k(imp48, rx24);
            int32_t ncc = 0;
            int good = 0;
            if (collect_real_lsf_trial(rx24, melpe, &ncc, &good) == 0
                && good >= 3)
                intact++;
        }
        sum += intact;
        std::printf("  seed_base=%u intact=%u/200\n", base, intact);
    }
    double mean = sum / 5.0;
    std::printf("C20 multi-seed mean=%.1f/200 (range covers prior 118 vs "
                "121)\n",
                mean);
    return 0;
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
    gain.gain = 0.5f;
    int gain05 = count_good_frames(gain, false);
    gain.gain = 2.0f;
    int gain20 = count_good_frames(gain, false);

    impair_t trunc = base;
    trunc.drop_start = HorseModulator::captureSamplesPerFrame();
    int trunc_good = count_good_frames(trunc, false);

    std::printf("impair: first_noise_fail=%.0f first_ppm_fail=%.0f "
                "invert_no_flag=%d/3 invert_with_flag=%d/3 dc500=%d/3 "
                "gain0.25=%d/3 gain0.5=%d/3 gain2.0=%d/3 drop1frame=%d/3 "
                "CORR_PEAK_MIN=%d\n",
                noise_fail, ppm_fail, inv_auto, inv_flag, dc_good, gain_good,
                gain05, gain20, trunc_good, CORR_PEAK_MIN);
    if (gain20 < 3)
        std::printf("impair: gain 2.0 fails because the samples clip\n");
    /*
     * Opening LSF is three coded frames (was one uncoded). The short
     * LSF+voice+EOT probe is two frames longer, so exact-sync EOT under
     * the same impair PRNG fails earlier. Measured floor after the
     * change is 9500 (was 13000). Do not lower further without cause.
     */
    if (noise_fail >= 0.0f && noise_fail < 9500.0f) {
        std::printf(
            "impair: noise floor regressed (fail at %.0f, want >=9500)\n",
            noise_fail);
        return -1;
    }
    if (ppm_fail >= 0.0f && ppm_fail < 300.0f) {
        std::printf("impair: ppm floor regressed (fail at %.0f, want >=300)\n",
                    ppm_fail);
        return -1;
    }
    return 0;
}

/*
 * Before the acquire snapshot/restore, a Hamming-fail LSF search at
 * noise 12000 left frameIndex=8 and stalled takeFrame (got=162/302,
 * locked=1). After: the full LSF+300 voice+EOT stream is delivered.
 */
static int test_frame_index_stall()
{
    HorseFrameEncoder enc;
    HorseFrameDecoder decoder;
    const int nvoice = 300;
    std::vector<frame_t> frames;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    push_lsf(enc, src, dst, nullptr, 0, frames);
    uint8_t melpe[12];
    uint8_t tag[4] = { 9, 8, 7, 6 };
    memset(melpe, 0xA5, sizeof melpe);
    for (int i = 0; i < nvoice; i++) {
        frame_t vf{};
        enc.encodeVoiceFrameWithFn(melpe, tag, (uint16_t)i, vf, false, 12);
        frames.push_back(vf);
    }
    { frame_t ef; enc.encodeEotFrame(ef); frames.push_back(ef); }
    std::vector<int16_t> bb48, imp48, rx24;
    if (render_frames(frames, bb48, true) != 0)
        return -1;
    impair_t p{};
    p.noise = 12000.f;
    p.gain = 1.0f;
    p.seed = 1u;
    impair_48k(bb48.data(), bb48.size(), p, imp48);
    to_24k(imp48, rx24);
    HorseDemodulator demod;
    demod.init();
    demod.resetImmediate();
    demod.setSkipDcBlock(true);
    int got = 0;
    for (int16_t s : rx24) {
        demod.feedSample(s, false);
        frame_t f;
        if (demod.takeFrame(f))
            got++;
    }
    demod.terminate();
    std::printf("frame-index stall: noise12000 unauth got=%d/%d "
                "(pre-fix was 162/302)\n",
                got, (int)LSF_OPENING_FRAMES + nvoice + 1);
    if (got < 290) {
        std::printf("frame-index stall: still stuck after failed LSF "
                    "acquire\n");
        return -1;
    }
    return 0;
}

static int count_stream_frames(float noise, int nvoice, unsigned seed,
                               uint8_t miss, bool auth)
{
    HorseFrameEncoder enc;
    HorseFrameDecoder decoder;
    std::vector<frame_t> frames;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    push_lsf(enc, src, dst, nullptr, 0, frames);
    uint8_t melpe[12];
    uint8_t tag[4] = { 9, 8, 7, 6 };
    memset(melpe, 0xA5, sizeof melpe);
    for (int i = 0; i < nvoice; i++) {
        frame_t vf{};
        enc.encodeVoiceFrameWithFn(melpe, tag, (uint16_t)i, vf, false, 12);
        frames.push_back(vf);
    }
    { frame_t ef; enc.encodeEotFrame(ef); frames.push_back(ef); }
    std::vector<int16_t> bb48, imp48, rx24;
    if (render_frames(frames, bb48, true) != 0)
        return -1;
    impair_t p{};
    p.noise = noise;
    p.gain = 1.0f;
    p.seed = seed;
    impair_48k(bb48.data(), bb48.size(), p, imp48);
    to_24k(imp48, rx24);
    HorseDemodulator demod;
    demod.init();
    demod.resetImmediate();
    demod.setSkipDcBlock(true);
    demod.setMissUnlock(miss);
    int got = 0;
    for (int16_t s : rx24) {
        demod.feedSample(s, false);
        frame_t f;
        if (!demod.takeFrame(f))
            continue;
        if (auth && decoder.decodeFrame(f) == HorseFrameType::LINK_SETUP
            && decoder.lsfReady())
            demod.noteValidTag();
        got++;
    }
    demod.terminate();
    return got;
}

static int count_idle_after_voice(uint8_t miss)
{
    HorseFrameEncoder enc;
    HorseFrameDecoder decoder;
    const int nvoice = 20;
    std::vector<frame_t> frames;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    push_lsf(enc, src, dst, nullptr, 0, frames);
    uint8_t melpe[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(melpe, 0x11, sizeof melpe);
    for (int i = 0; i < nvoice; i++) {
        frame_t vf{};
        enc.encodeVoiceFrameWithFn(melpe, tag, (uint16_t)i, vf, false, 12);
        frames.push_back(vf);
    }
    std::vector<int16_t> bb48, rx24;
    if (render_frames(frames, bb48, true) != 0)
        return -1;
    to_24k(bb48, rx24);
    const size_t extra = 30u * FRAME_SYMBOLS * 5u;
    rx24.insert(rx24.end(), extra, 0);
    HorseDemodulator demod;
    demod.init();
    demod.resetImmediate();
    demod.setSkipDcBlock(true);
    demod.setMissUnlock(miss);
    int after_voice = 0;
    bool seen_voice = false;
    int voice_n = 0;
    for (int16_t s : rx24) {
        demod.feedSample(s, false);
        frame_t f;
        if (!demod.takeFrame(f))
            continue;
        HorseFrameType t = decoder.decodeFrame(f);
        if (t == HorseFrameType::LINK_SETUP && decoder.lsfReady())
            demod.noteValidTag();
        if (t == HorseFrameType::VOICE) {
            seen_voice = true;
            voice_n++;
            after_voice = 0;
        } else if (seen_voice && voice_n >= nvoice)
            after_voice++;
        if (!demod.isLocked() && seen_voice && voice_n >= nvoice)
            break;
    }
    demod.terminate();
    return after_voice;
}

static int test_coast_and_sp_protect()
{
    int auth = count_stream_frames(12000.f, 300, 1u, COAST_MISS_UNLOCK, true);
    int auth125 = count_stream_frames(12500.f, 300, 1u, COAST_MISS_UNLOCK,
                                      true);
    std::printf("coast/sp: noise12000 auth=%d/%d noise12500 auth=%d/%d\n",
                auth, (int)LSF_OPENING_FRAMES + 300 + 1, auth125,
                (int)LSF_OPENING_FRAMES + 300 + 1);
    if (auth < 290 || auth125 < 290) {
        std::printf("coast/sp: authenticated long lock failed\n");
        return -1;
    }

    HorseFrameEncoder enc;
    HorseFrameDecoder decoder;
    std::vector<frame_t> tx;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    uint8_t melpe[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(melpe, 0x33, sizeof melpe);
    push_lsf(enc, src, dst, nullptr, 0, tx);
    { frame_t _v; enc.encodeVoiceFrame(melpe, tag, _v, false); tx.push_back(_v); }
    { frame_t _v; enc.encodeVoiceFrame(melpe, tag, _v, false); tx.push_back(_v); }
    push_lsf(enc, src, dst, nullptr, 0, tx);
    std::vector<int16_t> bb48, rx24;
    if (render_frames(tx, bb48, true) != 0)
        return -1;
    to_24k(bb48, rx24);
    HorseDemodulator demod;
    demod.init();
    demod.resetImmediate();
    demod.setSkipDcBlock(true);
    uint32_t sp_after_tag = 0;
    bool tagged = false;
    int lsf_n = 0;
    for (int16_t s : rx24) {
        demod.feedSample(s, false);
        frame_t f;
        if (!demod.takeFrame(f))
            continue;
        HorseFrameType t = decoder.decodeFrame(f);
        if (t == HorseFrameType::LINK_SETUP) {
            lsf_n++;
            if (decoder.lsfReady())
                demod.noteValidTag();
            if (tagged && demod.debugSamplingPoint() != sp_after_tag) {
                std::printf("coast/sp: authenticated lock moved SP on LSF "
                            "%u -> %u\n",
                            sp_after_tag, demod.debugSamplingPoint());
                demod.terminate();
                return -1;
            }
        }
        if (t == HorseFrameType::VOICE && !tagged) {
            demod.noteValidTag();
            tagged = true;
            sp_after_tag = demod.debugSamplingPoint();
        }
    }
    demod.terminate();
    if (!tagged || lsf_n < 1) {
        std::printf("coast/sp: auth LSF-ignore setup failed tag=%d lsf=%d\n",
                    tagged, lsf_n);
        return -1;
    }
    std::printf("coast/sp: authenticated lock kept SP=%u through later LSF\n",
                sp_after_tag);

    {
        HorseFrameEncoder enc2;
        HorseFrameDecoder decoder2;
        std::vector<frame_t> tx2;
        push_lsf(enc2, src, dst, nullptr, 0, tx2);
        { frame_t _v; enc2.encodeVoiceFrame(melpe, tag, _v, false); tx2.push_back(_v); }
        std::vector<int16_t> bb2, rx2;
        if (render_frames(tx2, bb2, true) != 0)
            return -1;
        to_24k(bb2, rx2);
        HorseDemodulator d2;
        d2.init();
        d2.resetImmediate();
        d2.setSkipDcBlock(true);
        uint32_t sp_lock = 0;
        bool have_lock = false;
        for (size_t i = 0; i < rx2.size(); i++) {
            d2.feedSample(rx2[i], false);
            frame_t f;
            if (!d2.takeFrame(f))
                continue;
            if (decoder2.decodeFrame(f) == HorseFrameType::VOICE) {
                have_lock = true;
                sp_lock = d2.debugSamplingPoint();
                break;
            }
        }
        if (!have_lock) {
            std::printf("coast/sp: unauth reject setup failed\n");
            d2.terminate();
            return -1;
        }
        const uint16_t fi_lock = d2.debugFrameIndex();
        for (int z = 0; z < 200; z++)
            d2.feedSample(0, false);
        if (d2.debugSamplingPoint() != sp_lock
            || d2.debugFrameIndex() < fi_lock) {
            std::printf("coast/sp: rejected LSF search moved lock sp %u->%u "
                        "fi %u->%u\n",
                        sp_lock, d2.debugSamplingPoint(), (unsigned)fi_lock,
                        (unsigned)d2.debugFrameIndex());
            d2.terminate();
            return -1;
        }
        d2.terminate();
        std::printf("coast/sp: unauth rejected candidate kept SP=%u\n",
                    sp_lock);
    }

    std::printf("coast idle frames after last voice (no EOT):\n");
    int extra4 = 0;
    for (uint8_t n : { (uint8_t)2, (uint8_t)4, (uint8_t)8 }) {
        int extra = count_idle_after_voice(n);
        std::printf("  N=%u extra=%d (%.0f ms)\n", (unsigned)n, extra,
                    extra * 40.0f);
        if (n == 4)
            extra4 = extra;
    }
    if (extra4 < 0 || extra4 > 12) {
        std::printf("coast/sp: coast N=4 extra out of range\n");
        return -1;
    }
    return 0;
}

/*
 * Long transmissions at clock offset. Expect >= 99 % delivery up to
 * 20 ppm with bounded tracking. Frozen mode is reported for the
 * before/after table.
 */
static int run_long_clock_case(const std::vector<int16_t> &bb48, int nvoice,
                               float ppm, float noise, bool tracking,
                               int *got_out, int *ok_out)
{
    HorseFrameDecoder decoder;
    uint8_t melpe[12];
    uint8_t tag[4] = { 5, 6, 7, 8 };
    memset(melpe, 0x5A, sizeof melpe);
    std::vector<int16_t> imp48, rx24;
    impair_t p{};
    p.noise = noise;
    p.gain = 1.0f;
    p.seed = 1u;
    p.rate_ppm = ppm;
    impair_48k(bb48.data(), bb48.size(), p, imp48);
    to_24k(imp48, rx24);
    HorseDemodulator demod;
    demod.init();
    demod.resetImmediate();
    demod.setSkipDcBlock(true);
    demod.setClockTracking(tracking);
    int got = 0;
    int ok = 0;
    for (int16_t s : rx24) {
        demod.feedSample(s, false);
        frame_t f;
        if (!demod.takeFrame(f))
            continue;
        HorseFrameType t = decoder.decodeFrame(f);
        if (t == HorseFrameType::LINK_SETUP && decoder.lsfReady())
            demod.noteValidTag();
        got++;
        if (t == HorseFrameType::VOICE) {
            uint16_t fn = 0;
            uint8_t payload[12], tagb[4];
            decoder.getVoicePayload(f, payload, tagb, &fn);
            if (fn < (uint16_t)nvoice && memcmp(payload, melpe, 12) == 0
                && memcmp(tagb, tag, 4) == 0)
                ok++;
        }
    }
    demod.terminate();
    *got_out = got;
    *ok_out = ok;
    return 0;
}

static int render_long_voice(int nvoice, std::vector<int16_t> &bb48)
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    push_lsf(enc, src, dst, nullptr, 0, frames);
    uint8_t melpe[12];
    uint8_t tag[4] = { 5, 6, 7, 8 };
    memset(melpe, 0x5A, sizeof melpe);
    for (int i = 0; i < nvoice; i++) {
            frame_t vf{};
            enc.encodeVoiceFrameWithFn(melpe, tag, (uint16_t)i, vf, false, 12);
            frames.push_back(vf);
        }
    { frame_t ef; enc.encodeEotFrame(ef); frames.push_back(ef); }
    return render_frames(frames, bb48, true);
}

static int test_long_clock_smoke()
{
    std::vector<int16_t> bb48;
    if (render_long_voice(3000, bb48) != 0)
        return -1;
    for (float ppm : { 0.f, 20.f }) {
        int got = 0, ok = 0;
        if (run_long_clock_case(bb48, 3000, ppm, 0.f, true, &got, &ok) != 0)
            return -1;
        const int expect = (int)LSF_OPENING_FRAMES + 3000 + 1;
        double deliv = (double)got / expect;
        std::printf("long-clock smoke ppm=%.0f got=%d/%d ok=%d deliv=%.4f\n",
                    ppm, got, expect, ok, deliv);
        if (deliv < 0.99) {
            std::printf("long-clock smoke: delivery below 99 %%\n");
            return -1;
        }
    }
    return 0;
}

static int test_long_clock_tracking()
{
    const int nvoices[] = { 3000, 28000 };
    const float ppms[] = { 0.f, 2.f, 5.f, 10.f, 20.f, 50.f };
    const float noises[] = { 0.f, 8000.f };
    int fails = 0;
    for (int nvoice : nvoices) {
        std::vector<int16_t> bb48;
        std::printf("long-clock render nvoice=%d ...\n", nvoice);
        std::fflush(stdout);
        if (render_long_voice(nvoice, bb48) != 0)
            return -1;
        for (bool tracking : { false, true }) {
            std::printf("long-clock tracking=%d nvoice=%d:\n",
                        tracking ? 1 : 0, nvoice);
            std::fflush(stdout);
            for (float noise : noises) {
                for (float ppm : ppms) {
                    int got = 0, ok = 0;
                    if (run_long_clock_case(bb48, nvoice, ppm, noise, tracking,
                                            &got, &ok)
                        != 0)
                        return -1;
                    const int expect = nvoice + (int)LSF_OPENING_FRAMES + 1;
                    double fer = nvoice ? 1.0 - (double)ok / nvoice : 1.0;
                    double deliv = expect ? (double)got / expect : 0.0;
                    std::printf("  noise=%.0f ppm=%.0f got=%d/%d ok=%d "
                                "fer=%.6f deliv=%.4f\n",
                                noise, ppm, got, expect, ok, fer, deliv);
                    std::fflush(stdout);
                    if (tracking && ppm <= 20.f && deliv < 0.99) {
                        std::printf("long-clock: delivery below 99 %% at "
                                    "ppm=%.0f n=%d\n",
                                    ppm, nvoice);
                        fails++;
                    }
                }
            }
        }
    }
    return fails == 0 ? 0 : -1;
}

int test_three_mode_loopback(void);

int main(int argc, char **argv)
{
    if (argc > 1 && std::string(argv[1]) == "long")
        return test_long_clock_tracking() == 0 ? 0 : 1;
    if (argc > 1 && std::string(argv[1]) == "c20seeds")
        return test_c20_multiseed() == 0 ? 0 : 1;

    if (test_layer_a_bytes_symbols() != 0)
        return -1;
    if (test_layer_b_rrc_known_phase() != 0)
        return -1;
    if (test_layer_c_demod_timing() != 0)
        return -1;
    if (test_no_late_entry() != 0)
        return -1;
    if (test_lsf_false_lock_noise() != 0)
        return -1;
    if (test_corr_peak_distributions() != 0)
        return -1;
    if (test_tx_during_false_lock() != 0)
        return -1;
    if (test_lsf_replaces_unauth_lock() != 0)
        return -1;
    if (test_layer_dc_block() != 0)
        return -1;
    if (test_layer_d_full_tx() != 0)
        return -1;
    if (test_impairments() != 0)
        return -1;
    if (test_lsf_intact_under_noise() != 0)
        return -1;
    if (test_frame_index_stall() != 0)
        return -1;
    if (test_coast_and_sp_protect() != 0)
        return -1;
    if (test_long_clock_smoke() != 0)
        return -1;
    if (test_three_mode_loopback() != 0)
        return -1;
    std::printf("horse_loopback: all tests passed\n");
    return 0;
}

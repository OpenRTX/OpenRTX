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
        if (d.type == HorseFrameType::LINK_SETUP)
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

static int test_no_late_entry()
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames(2);
    uint8_t melpe[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(melpe, 0x11, sizeof melpe);
    enc.encodeVoiceFrame(melpe, tag, frames[0], false);
    enc.encodeEotFrame(frames[1]);
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
                  std::vector<int32_t> *lock_peaks = nullptr, float gain = 1.0f,
                  std::vector<int32_t> *diss = nullptr)
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
        uint8_t lsf_hd = static_cast<uint8_t>(
            __builtin_popcount(frame[0] ^ LSF_SYNC_WORD[0])
            + __builtin_popcount(frame[1] ^ LSF_SYNC_WORD[1]));
        if (lsf_hd <= HAMMING_SYNC_MAX && diss != nullptr)
            diss->push_back(
                static_cast<int32_t>(decoder.lsfRepeatDisagreements(frame)));
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
    std::vector<frame_t> frames(3);
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    uint8_t melpe[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(melpe, 0x11, sizeof melpe);
    enc.encodeLsf(src, dst, nullptr, 0, frames[0]);
    enc.encodeVoiceFrame(melpe, tag, frames[1], false);
    enc.encodeEotFrame(frames[2]);

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
    std::vector<frame_t> frames(3);
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    uint8_t melpe[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(melpe, 0x22, sizeof melpe);
    enc.encodeLsf(src, dst, nullptr, 0, frames[0]);
    enc.encodeVoiceFrame(melpe, tag, frames[1], false);
    enc.encodeEotFrame(frames[2]);
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
    std::vector<frame_t> frames(4);
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    uint8_t melpe[12];
    uint8_t tag[4] = { 9, 8, 7, 6 };
    memset(melpe, 0x55, sizeof melpe);
    enc.encodeLsf(src, dst, nullptr, 0, frames[0]);
    enc.encodeVoiceFrame(melpe, tag, frames[1], false);
    enc.encodeVoiceFrame(melpe, tag, frames[2], true);
    enc.encodeEotFrame(frames[3]);
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
                                  const uint8_t *melpe, int32_t *ncc, int *good,
                                  int32_t *diss)
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
            demod.noteValidTag();
            if (diss != nullptr)
                *diss =
                    static_cast<int32_t>(decoder.lsfRepeatDisagreements(frame));
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

static int test_ncc_under_noise()
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
    std::vector<int16_t> bb48;
    if (render_frames(frames, bb48, true) != 0)
        return -1;

    {
        HorseFrameEncoder kenc;
        frame_t keyed{};
        uint8_t eph[32];
        memset(eph, 0x22, sizeof eph);
        kenc.encodeLsf(src, dst, eph, LSF_FLAG_ENCRYPTED | LSF_FLAG_SIGNED,
                       keyed);
        HorseFrameDecoder kd;
        std::printf("keyed LSF (eph 0x22) clean repeat-pair diss=%u "
                    "(LSF is uncoded; limit=%u)\n",
                    kd.lsfRepeatDisagreements(keyed), LSF_REPEAT_DISAGREE_MAX);
    }

    const float sigmas[] = { 2000.0f, 5000.0f, 10000.0f, 12500.0f, 15000.0f };
    const unsigned n_try = 200u;
    std::printf("ncc/diss under noise (gain=1, collect floor 0):\n");
    std::printf("  kind  sigma   n  min   p5  p50  max  good3/200\n");

    for (float sig : sigmas) {
        std::vector<int32_t> nccs;
        std::vector<int32_t> disss;
        std::vector<int32_t> diss_ok;
        unsigned good3 = 0;
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
            int32_t diss = 0;
            if (collect_real_lsf_trial(rx24, melpe, &ncc, &good, &diss) == 0) {
                nccs.push_back(ncc);
                disss.push_back(diss);
            }
            if (good >= 3) {
                good3++;
                diss_ok.push_back(diss);
            }
        }
        if (nccs.empty()) {
            std::printf("ncc   %5.0f    0     -    -    -    -  %3u/200\n", sig,
                        good3);
            std::printf("diss  %5.0f    0     -    -    -    -\n", sig);
            continue;
        }
        std::sort(nccs.begin(), nccs.end());
        std::sort(disss.begin(), disss.end());
        size_t p5n = (nccs.size() * 5u) / 100u;
        size_t p5d = (disss.size() * 5u) / 100u;
        std::printf("ncc   %5.0f  %3zu  %4d %4d %4d %4d  %3u/200\n", sig,
                    nccs.size(), nccs.front(), nccs[p5n], nccs[nccs.size() / 2],
                    nccs.back(), good3);
        std::printf("diss  %5.0f  %3zu  %4d %4d %4d %4d\n", sig, disss.size(),
                    disss.front(), disss[p5d], disss[disss.size() / 2],
                    disss.back());
        if (!diss_ok.empty()) {
            std::sort(diss_ok.begin(), diss_ok.end());
            std::printf("diss3 %5.0f  %3zu  %4d %4d %4d %4d (payload OK)\n",
                        sig, diss_ok.size(), diss_ok.front(),
                        diss_ok[(diss_ok.size() * 5u) / 100u],
                        diss_ok[diss_ok.size() / 2], diss_ok.back());
        }
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
    std::vector<int32_t> false_diss;
    false_lock_row_t fl = count_false_locks(0, 0, noise_s, 7u, &false_ncc, 1.0f,
                                            &false_diss);
    print_pctiles(long_noise ? "false lock Hamming0 gain=1 ncc Q12 (10 min)" :
                               "false lock Hamming0 gain=1 ncc Q12 (10 s)",
                  false_ncc);
    print_pctiles(long_noise ?
                      "false lock Hamming0 gain=1 repeat-2 diss (10 min)" :
                      "false lock Hamming0 gain=1 repeat-2 diss (10 s)",
                  false_diss);
    double minutes = static_cast<double>(fl.samples) / 24000.0 / 60.0;
    size_t kept = 0;
    for (int32_t d : false_diss)
        if (d <= static_cast<int32_t>(LSF_REPEAT_DISAGREE_MAX))
            kept++;
    std::printf("false LSF/min over %.1f min: before diss-check %.2f "
                "(n=%zu) after (diss<=%u) %.2f (n=%zu); demod/min %.2f\n",
                minutes, false_diss.size() / minutes, false_diss.size(),
                LSF_REPEAT_DISAGREE_MAX, kept / minutes, kept,
                fl.demod_locks / minutes);
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
    if (noise_fail >= 0.0f && noise_fail < 13000.0f) {
        std::printf(
            "impair: noise floor regressed (fail at %.0f, want >=13000)\n",
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

int test_three_mode_loopback(void);

int main()
{
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
    if (test_ncc_under_noise() != 0)
        return -1;
    if (test_three_mode_loopback() != 0)
        return -1;
    std::printf("horse_loopback: all tests passed\n");
    return 0;
}

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Host simulation of Horse v2 FEC candidates through HorseModulator and
 * HorseDemodulator. Firmware on-air format is not changed.
 */

#include "horse_fec_v2_codes.hpp"
#include "protocols/horse/HorseFrameEncoder.hpp"
#include "protocols/horse/HorseFrameDecoder.hpp"
#include "protocols/horse/HorseModulator.hpp"
#include "protocols/horse/HorseDemodulator.hpp"
#include "protocols/horse/HorseConstants.hpp"
#include "protocols/horse/HorseUtils.hpp"
#include "protocols/M17/DSP.hpp"
#include "core/fir.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>

using namespace horse;

static constexpr size_t SPS_48 = 48000 / SYMBOL_RATE;
static constexpr size_t SPS_24 = 24000 / SYMBOL_RATE;
static constexpr size_t TX_DELAY_48 = 40;
static constexpr size_t RX_DELAY_24 = 20;
static constexpr size_t PREAMBLE_FRAMES = 2;

struct impair_t {
    float noise;
    float gain;
    unsigned seed;
};

static void impair_48k(const int16_t *in, size_t n, const impair_t &p,
                       std::vector<int16_t> &out)
{
    out.clear();
    unsigned rng = p.seed != 0u ? p.seed : 1u;
    out.reserve(n);
    for (size_t i = 0; i < n; i++) {
        float s = static_cast<float>(in[i]) * p.gain;
        rng = rng * 1103515245u + 12345u;
        float nse = (static_cast<float>(rng & 0xFFFFu) / 32768.0f - 1.0f)
                  * p.noise;
        s += nse;
        if (s > 32767.0f)
            s = 32767.0f;
        if (s < -32768.0f)
            s = -32768.0f;
        out.push_back(static_cast<int16_t>(s));
    }
}

static int render_frames(const std::vector<frame_t> &frames,
                         std::vector<int16_t> &bb48, bool preamble)
{
    HorseModulator mod;
    const size_t per = HorseModulator::captureSamplesPerFrame();
    size_t extra = preamble ? 2 : 0;
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

static void to_24k(const std::vector<int16_t> &bb48, std::vector<int16_t> &rx24)
{
    rx24.clear();
    for (size_t i = 0; i + 1 < bb48.size(); i += 2)
        rx24.push_back(bb48[i]);
}

static int demod_frames(const std::vector<int16_t> &rx24,
                        std::vector<frame_t> &out)
{
    HorseDemodulator demod;
    demod.init();
    demod.resetImmediate();
    demod.setSkipDcBlock(true);
    out.clear();
    for (int16_t s : rx24) {
        demod.feedSample(s, false);
        frame_t f{};
        if (demod.takeFrame(f))
            out.push_back(f);
    }
    demod.terminate();
    return 0;
}

static void pack_voice(const uint8_t coded46[FEC_CODED_BYTES], frame_t &out)
{
    std::copy(VOICE_SYNC_WORD.begin(), VOICE_SYNC_WORD.end(), out.begin());
    memcpy(out.data() + 2, coded46, FEC_CODED_BYTES);
}

static void burst_corrupt(frame_t &f, size_t nsym, bool interleave)
{
    std::array<uint8_t, FEC_CODED_BYTES> p{};
    memcpy(p.data(), f.data() + 2, FEC_CODED_BYTES);
    if (interleave)
        M17::interleave(p);
    for (size_t s = 8; s < 8 + nsym && s < FRAME_SYMBOLS; s++) {
        int8_t sym = +3;
        horse::setSymbol(p, s - 8, static_cast<int8_t>(-sym));
    }
    if (interleave)
        M17::deinterleave(p);
    memcpy(f.data() + 2, p.data(), FEC_CODED_BYTES);
}

static void llr_from_frame_hard(const frame_t &f, float *llr)
{
    for (size_t i = 0; i < FEC_CODED_BITS; i++) {
        size_t bi = 16 + i;
        bool b = (f[bi / 8] >> (7 - (bi % 8))) & 1u;
        llr[i] = b ? -8.f : 8.f;
    }
}

enum CodecId {
    CID_R2 = 0,
    CID_POLAR4,
    CID_POLAR8,
    CID_POLAR16,
    CID_M17H,
    CID_M17S,
    CID_CCSDS10,
    CID_CCSDS20,
    CID_CCSDS50,
    CID_COUNT
};

static const char *codec_name(int id)
{
    static const char *n[] = { "repeat2",   "polar_L4",  "polar_L8",
                               "polar_L16", "m17_hard",  "m17_soft",
                               "ccsds_I10", "ccsds_I20", "ccsds_I50" };
    return n[id];
}

struct Codecs {
    Repeat2Codec r2;
    PolarCodec polar;
    M17VoiceCodec m17;
    Ccsds512 ccsds;
};

static void encode_cid(Codecs &c, int id, const uint8_t *info, uint8_t *cw)
{
    if (id == CID_R2)
        c.r2.encode(info, cw);
    else if (id >= CID_POLAR4 && id <= CID_POLAR16)
        c.polar.encode(info, cw);
    else if (id == CID_M17H || id == CID_M17S)
        c.m17.encode(info, cw);
    else
        c.ccsds.encode(info, cw);
}

static bool decode_cid(Codecs &c, int id, const float *llr,
                       const uint8_t *hard46, uint8_t *info)
{
    if (id == CID_R2)
        return c.r2.decode_hard(hard46, info);
    if (id == CID_POLAR4)
        return c.polar.decode(llr, 4, info);
    if (id == CID_POLAR8)
        return c.polar.decode(llr, 8, info);
    if (id == CID_POLAR16)
        return c.polar.decode(llr, 16, info);
    if (id == CID_M17H)
        return c.m17.decode_hard(hard46, info);
    if (id == CID_M17S)
        return c.m17.decode_soft(llr, info);
    int it = 10;
    if (id == CID_CCSDS20)
        it = 20;
    if (id == CID_CCSDS50)
        it = 50;
    return c.ccsds.decode_minsum(llr, it, info);
}

static int run_voice_fer(Codecs &c, int id, float noise, unsigned seed,
                         int nframes, int *ok, int *got, int *undet,
                         double *us_per)
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames;
    frames.resize(static_cast<size_t>(nframes) + 2);
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    enc.encodeLsf(src, dst, nullptr, 0, frames[0]);
    std::vector<std::array<uint8_t, FEC_INFO_BYTES>> refs(
        static_cast<size_t>(nframes));
    unsigned rng = seed;
    for (int i = 0; i < nframes; i++) {
        for (size_t b = 0; b < FEC_INFO_BYTES; b++) {
            rng = rng * 1103515245u + 12345u;
            refs[static_cast<size_t>(i)][b] = (uint8_t)(rng >> 16);
        }
        uint8_t cw[FEC_CODED_BYTES];
        encode_cid(c, id, refs[static_cast<size_t>(i)].data(), cw);
        pack_voice(cw, frames[static_cast<size_t>(i) + 1]);
    }
    enc.encodeEotFrame(frames.back());

    std::vector<int16_t> bb48, imp, rx24;
    if (render_frames(frames, bb48, true) != 0)
        return -1;
    impair_t p{};
    p.noise = noise;
    p.gain = 1.0f;
    p.seed = seed;
    impair_48k(bb48.data(), bb48.size(), p, imp);
    to_24k(imp, rx24);
    std::vector<frame_t> gotf;
    demod_frames(rx24, gotf);

    *ok = 0;
    *got = 0;
    *undet = 0;
    int decoded_n = 0;
    double tsum = 0;
    HorseFrameDecoder dec;
    for (const auto &f : gotf) {
        if (dec.decodeFrame(f) != HorseFrameType::VOICE)
            continue;
        if (*got >= nframes)
            break;
        float llr[FEC_CODED_BITS];
        llr_from_frame_hard(f, llr);
        uint8_t hard46[FEC_CODED_BYTES];
        memcpy(hard46, f.data() + 2, FEC_CODED_BYTES);
        uint8_t out[FEC_INFO_BYTES];
        auto t0 = std::chrono::steady_clock::now();
        bool pass = decode_cid(c, id, llr, hard46, out);
        auto t1 = std::chrono::steady_clock::now();
        tsum += std::chrono::duration<double, std::micro>(t1 - t0).count();
        decoded_n++;
        const uint8_t *ref = refs[static_cast<size_t>(*got)].data();
        if (pass && memcmp(out, ref, FEC_INFO_BYTES) == 0)
            (*ok)++;
        else if (pass)
            (*undet)++;
        (*got)++;
    }
    *us_per = decoded_n ? tsum / decoded_n : 0;
    return 0;
}

static int run_burst(Codecs &c, int id, size_t nsym, bool intl, int nframes,
                     int *ok)
{
    *ok = 0;
    unsigned rng = 7;
    for (int i = 0; i < nframes; i++) {
        uint8_t info[FEC_INFO_BYTES];
        for (size_t b = 0; b < FEC_INFO_BYTES; b++) {
            rng = rng * 1103515245u + 12345u;
            info[b] = (uint8_t)(rng >> 16);
        }
        uint8_t cw[FEC_CODED_BYTES];
        encode_cid(c, id, info, cw);
        frame_t f{};
        pack_voice(cw, f);
        burst_corrupt(f, nsym, intl);
        float llr[FEC_CODED_BITS];
        llr_from_frame_hard(f, llr);
        uint8_t out[FEC_INFO_BYTES];
        if (decode_cid(c, id, llr, f.data() + 2, out)
            && memcmp(out, info, FEC_INFO_BYTES) == 0)
            (*ok)++;
    }
    return 0;
}

static int hd_sync(const frame_t &f, const syncw_t &w)
{
    return __builtin_popcount((unsigned)f[0] ^ w[0])
         + __builtin_popcount((unsigned)f[1] ^ w[1]);
}

static bool match_sync(const frame_t &f, const syncw_t &w)
{
    return hd_sync(f, w) <= (int)HAMMING_SYNC_MAX;
}

static void slice_ideal(const std::vector<int16_t> &rx24, bool preamble,
                        std::vector<frame_t> &out)
{
    Fir<41> rxRrc(M17::rrc_taps_24k);
    std::vector<int16_t> filt;
    filt.reserve(rx24.size());
    for (size_t i = 0; i < rx24.size(); i++)
        filt.push_back(
            static_cast<int16_t>(rxRrc(static_cast<float>(rx24[i]))));
    size_t first = RX_DELAY_24 + TX_DELAY_48 / 2;
    if (preamble)
        first += PREAMBLE_FRAMES * FRAME_SYMBOLS * SPS_24;
    const size_t span = FRAME_SYMBOLS * SPS_24;
    std::vector<int16_t> mag;
    size_t p0 = first;
    while (p0 + span <= filt.size()) {
        for (size_t s = 0; s < FRAME_SYMBOLS; s++) {
            int16_t v = filt[p0 + s * SPS_24];
            if (v < 0)
                v = static_cast<int16_t>(-v);
            mag.push_back(v);
        }
        p0 += span;
    }
    int16_t outer = 1;
    if (mag.size() > 0) {
        size_t q = mag.size() * 9 / 10;
        if (q >= mag.size())
            q = mag.size() - 1;
        std::nth_element(mag.begin(), mag.begin() + (ptrdiff_t)q, mag.end());
        outer = mag[q];
        if (outer < 1)
            outer = 1;
    }
    out.clear();
    while (first + span <= filt.size()) {
        frame_t f{};
        for (size_t s = 0; s < FRAME_SYMBOLS; s++) {
            int8_t sy = quantizeLevel(filt[first + s * SPS_24], outer,
                                      static_cast<int16_t>(-outer));
            setSymbol(f, s, sy);
        }
        out.push_back(f);
        first += span;
    }
}

enum DropKind { DROP_NONE = 0, DROP_EOT, DROP_MISS, DROP_NEVER };

struct TrackStat {
    int delivered;
    int lock_len;
    int n_locks;
    int drop;
    int extra_after_eot;
    int n_eot_hd;
    int n_miss;
};

/*
 * miss_limit: unlock when consecutive non-matches exceed this.
 * Firmware uses missedSyncs > 4, so miss_limit = 4.
 */
static TrackStat track_frames(const std::vector<frame_t> &fr, int miss_limit,
                              int eot_index)
{
    TrackStat t{};
    t.drop = DROP_NEVER;
    bool locked = false;
    int missed = 0;
    int lock_start = 0;
    for (size_t i = 0; i < fr.size(); i++) {
        const frame_t &f = fr[i];
        bool valid = match_sync(f, LSF_SYNC_WORD)
                  || match_sync(f, VOICE_SYNC_WORD);
        bool eot = match_sync(f, EOT_SYNC_WORD);
        if (!locked) {
            if (hd_sync(f, LSF_SYNC_WORD) == 0) {
                locked = true;
                missed = 0;
                lock_start = (int)i;
                t.n_locks++;
                t.delivered++;
            }
            continue;
        }
        t.delivered++;
        if (valid)
            missed = 0;
        else {
            missed++;
            t.n_miss++;
        }
        if (eot)
            t.n_eot_hd++;
        if (eot || missed > miss_limit) {
            t.drop = eot ? DROP_EOT : DROP_MISS;
            t.lock_len = (int)i - lock_start + 1;
            locked = false;
            missed = 0;
            if (eot_index >= 0)
                t.extra_after_eot = (int)i - eot_index;
        }
    }
    if (locked) {
        t.drop = DROP_NONE;
        t.lock_len = (int)fr.size() - lock_start;
    }
    return t;
}

static int decode_voice_ok(Codecs &c, int id, const frame_t &f,
                           const uint8_t *ref)
{
    float llr[FEC_CODED_BITS];
    llr_from_frame_hard(f, llr);
    uint8_t out[FEC_INFO_BYTES];
    if (!decode_cid(c, id, llr, f.data() + 2, out))
        return 0;
    return memcmp(out, ref, FEC_INFO_BYTES) == 0 ? 1 : 0;
}

struct VoiceRow {
    int got;
    int ok;
    int undet;
    double us;
    TrackStat tr;
};

static void coast_from_ok(const std::vector<frame_t> &sl,
                          const std::vector<uint8_t> &okv, int nframes,
                          int miss_limit, VoiceRow *coast)
{
    *coast = VoiceRow{};
    coast->tr = track_frames(sl, miss_limit, nframes + 1);
    bool locked = false;
    int missed = 0;
    for (size_t i = 0; i < sl.size(); i++) {
        const frame_t &f = sl[i];
        bool valid = match_sync(f, LSF_SYNC_WORD)
                  || match_sync(f, VOICE_SYNC_WORD);
        bool eot = match_sync(f, EOT_SYNC_WORD);
        if (!locked) {
            if (hd_sync(f, LSF_SYNC_WORD) == 0) {
                locked = true;
                missed = 0;
            }
            continue;
        }
        if (i >= 1 && (int)i <= nframes) {
            coast->got++;
            if (okv[i - 1])
                coast->ok++;
        }
        if (valid)
            missed = 0;
        else
            missed++;
        if (eot || missed > miss_limit)
            locked = false;
    }
}

static int run_voice_study(Codecs &c, int id, float noise, unsigned seed,
                           int nframes, VoiceRow *demod, VoiceRow *ideal,
                           VoiceRow *c2, VoiceRow *c4, VoiceRow *c8)
{
    std::vector<frame_t> frames;
    std::vector<std::array<uint8_t, FEC_INFO_BYTES>> refs;
    frames.resize((size_t)nframes + 2);
    refs.resize((size_t)nframes);
    HorseFrameEncoder enc;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    enc.encodeLsf(src, dst, nullptr, 0, frames[0]);
    unsigned rng = seed;
    for (int i = 0; i < nframes; i++) {
        uint8_t info[FEC_INFO_BYTES];
        for (size_t b = 0; b < FEC_INFO_BYTES; b++) {
            rng = rng * 1103515245u + 12345u;
            info[b] = (uint8_t)(rng >> 16);
        }
        memcpy(refs[static_cast<size_t>(i)].data(), info, FEC_INFO_BYTES);
        uint8_t cw[FEC_CODED_BYTES];
        encode_cid(c, id, info, cw);
        pack_voice(cw, frames[static_cast<size_t>(i + 1)]);
    }
    enc.encodeEotFrame(frames.back());
    std::vector<int16_t> bb48, imp, rx24;
    if (render_frames(frames, bb48, true) != 0)
        return -1;
    impair_t p{};
    p.noise = noise;
    p.gain = 1.0f;
    p.seed = seed;
    impair_48k(bb48.data(), bb48.size(), p, imp);
    to_24k(imp, rx24);

    std::vector<frame_t> got;
    demod_frames(rx24, got);
    *demod = VoiceRow{};
    demod->tr = track_frames(got, 4, -1);
    double tsum = 0;
    int decoded_n = 0;
    HorseFrameDecoder dec;
    size_t vi = 0;
    for (const auto &f : got) {
        if (dec.decodeFrame(f) != HorseFrameType::VOICE)
            continue;
        if (vi >= (size_t)nframes)
            break;
        auto t0 = std::chrono::steady_clock::now();
        int good = decode_voice_ok(c, id, f, refs[vi].data());
        auto t1 = std::chrono::steady_clock::now();
        tsum += std::chrono::duration<double, std::micro>(t1 - t0).count();
        decoded_n++;
        demod->got++;
        if (good)
            demod->ok++;
        vi++;
    }
    demod->us = decoded_n ? tsum / decoded_n : 0;
    if (got.size() > 0) {
        const frame_t &last = got.back();
        std::printf("study last_demod %s hd_v=%d hd_l=%d hd_e=%d n=%zu\n",
                    codec_name(id), hd_sync(last, VOICE_SYNC_WORD),
                    hd_sync(last, LSF_SYNC_WORD), hd_sync(last, EOT_SYNC_WORD),
                    got.size());
    }

    std::vector<frame_t> sl;
    slice_ideal(rx24, true, sl);
    *ideal = VoiceRow{};
    ideal->tr = track_frames(sl, 4, -1);
    std::vector<uint8_t> okv((size_t)nframes, 0);
    tsum = 0;
    decoded_n = 0;
    for (int i = 0; i < nframes; i++) {
        size_t idx = (size_t)i + 1;
        if (idx >= sl.size())
            break;
        auto t0 = std::chrono::steady_clock::now();
        int good = decode_voice_ok(c, id, sl[idx],
                                   refs[static_cast<size_t>(i)].data());
        auto t1 = std::chrono::steady_clock::now();
        tsum += std::chrono::duration<double, std::micro>(t1 - t0).count();
        decoded_n++;
        ideal->got++;
        if (good) {
            ideal->ok++;
            okv[static_cast<size_t>(i)] = 1;
        }
    }
    ideal->us = decoded_n ? tsum / decoded_n : 0;

    coast_from_ok(sl, okv, nframes, 2, c2);
    coast_from_ok(sl, okv, nframes, 4, c4);
    coast_from_ok(sl, okv, nframes, 8, c8);
    return 0;
}

static void print_voice_row(const char *tag, int id, float noise, int n,
                            const VoiceRow &r)
{
    double fer = n ? 1.0 - (double)r.ok / n : 1.0;
    const char *ds[] = { "none", "eot", "miss", "never" };
    std::printf("%s %s noise=%.0f n=%d got=%d ok=%d fer=%.6f us/frame=%.1f "
                "lock_len=%d n_locks=%d drop=%s eot_hd=%d miss_ev=%d\n",
                tag, codec_name(id), noise, n, r.got, r.ok, fer, r.us,
                r.tr.lock_len, r.tr.n_locks, ds[r.tr.drop], r.tr.n_eot_hd,
                r.tr.n_miss);
}

static void hist_payload(int id, Codecs &c, unsigned seed, int n, int *h)
{
    h[0] = h[1] = h[2] = h[3] = 0;
    unsigned rng = seed;
    for (int i = 0; i < n; i++) {
        uint8_t info[FEC_INFO_BYTES];
        uint8_t cw[FEC_CODED_BYTES];
        for (size_t b = 0; b < FEC_INFO_BYTES; b++) {
            rng = rng * 1103515245u + 12345u;
            info[b] = (uint8_t)(rng >> 16);
        }
        encode_cid(c, id, info, cw);
        for (size_t b = 0; b < FEC_CODED_BYTES; b++) {
            auto sy = byteToSymbols(cw[b]);
            for (int k = 0; k < 4; k++) {
                if (sy[static_cast<size_t>(k)] == 3)
                    h[0]++;
                else if (sy[static_cast<size_t>(k)] == 1)
                    h[1]++;
                else if (sy[static_cast<size_t>(k)] == -1)
                    h[2]++;
                else
                    h[3]++;
            }
        }
    }
}

static int run_lockstat(float noise, unsigned seed, int nframes)
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames((size_t)nframes + 2);
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    enc.encodeLsf(src, dst, nullptr, 0, frames[0]);
    uint8_t payload[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(payload, 0x5A, sizeof payload);
    for (int i = 0; i < nframes; i++)
        enc.encodeVoiceFrameWithFn(payload, tag, (uint16_t)i,
                                   frames[static_cast<size_t>(i + 1)], false,
                                   12);
    enc.encodeEotFrame(frames.back());
    std::vector<int16_t> bb48, imp, rx24;
    if (render_frames(frames, bb48, true) != 0)
        return -1;
    impair_t p{};
    p.noise = noise;
    p.gain = 1.0f;
    p.seed = seed;
    impair_48k(bb48.data(), bb48.size(), p, imp);
    to_24k(imp, rx24);
    std::vector<frame_t> got, sl;
    demod_frames(rx24, got);
    slice_ideal(rx24, true, sl);
    int eot_i = nframes + 1;
    TrackStat td = track_frames(got, 4, eot_i);
    TrackStat ti = track_frames(sl, 4, eot_i);
    const char *ds[] = { "none", "eot", "miss", "never" };
    std::printf("lockstat demod noise=%.0f n=%d got=%d lock_len=%d n_locks=%d "
                "drop=%s extra_eot=%d eot_hd=%d miss_ev=%d\n",
                noise, nframes, td.delivered, td.lock_len, td.n_locks,
                ds[td.drop], td.extra_after_eot, td.n_eot_hd, td.n_miss);
    std::printf("lockstat ideal noise=%.0f n=%d got=%d lock_len=%d n_locks=%d "
                "drop=%s extra_eot=%d eot_hd=%d miss_ev=%d sl=%zu\n",
                noise, nframes, ti.delivered, ti.lock_len, ti.n_locks,
                ds[ti.drop], ti.extra_after_eot, ti.n_eot_hd, ti.n_miss,
                sl.size());
    if (got.size() > 0) {
        const frame_t &last = got.back();
        std::printf("lockstat last_demod hd_v=%d hd_l=%d hd_e=%d\n",
                    hd_sync(last, VOICE_SYNC_WORD),
                    hd_sync(last, LSF_SYNC_WORD), hd_sync(last, EOT_SYNC_WORD));
    }
    return 0;
}

static int run_eot_notice(float noise, unsigned seed, int nvoice,
                          int miss_limit)
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames((size_t)nvoice + 22);
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    enc.encodeLsf(src, dst, nullptr, 0, frames[0]);
    uint8_t payload[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(payload, 0x11, sizeof payload);
    for (int i = 0; i < nvoice; i++)
        enc.encodeVoiceFrameWithFn(payload, tag, (uint16_t)i,
                                   frames[static_cast<size_t>(i + 1)], false,
                                   12);
    enc.encodeEotFrame(frames[static_cast<size_t>(nvoice + 1)]);
    for (int i = nvoice + 2; i < (int)frames.size(); i++)
        memset(frames[static_cast<size_t>(i)].data(), 0,
               frames[static_cast<size_t>(i)].size());
    std::vector<int16_t> bb48, imp, rx24;
    if (render_frames(frames, bb48, true) != 0)
        return -1;
    impair_t p{};
    p.noise = noise;
    p.gain = 1.0f;
    p.seed = seed;
    impair_48k(bb48.data(), bb48.size(), p, imp);
    to_24k(imp, rx24);
    std::vector<frame_t> sl;
    slice_ideal(rx24, true, sl);
    int eot_i = nvoice + 1;
    TrackStat t = track_frames(sl, miss_limit, eot_i);
    const char *ds[] = { "none", "eot", "miss", "never" };
    std::printf("eot_notice miss_limit=%d noise=%.0f delivered=%d drop=%s "
                "extra_after_eot=%d lock_len=%d\n",
                miss_limit, noise, t.delivered, ds[t.drop], t.extra_after_eot,
                t.lock_len);
    return 0;
}

static void m17_encode_chunk(Codecs &c, const uint8_t *info18, frame_t &out,
                             bool lsf_sync)
{
    uint8_t cw[FEC_CODED_BYTES];
    c.m17.encode(info18, cw);
    if (lsf_sync)
        std::copy(LSF_SYNC_WORD.begin(), LSF_SYNC_WORD.end(), out.begin());
    else
        std::copy(VOICE_SYNC_WORD.begin(), VOICE_SYNC_WORD.end(), out.begin());
    memcpy(out.data() + 2, cw, FEC_CODED_BYTES);
}

static int run_complete_v2(Codecs &c, float noise, unsigned seed, int ntx,
                           int miss_limit, int mode, int *ok, int *got_voice)
{
    *ok = 0;
    *got_voice = 0;
    HorseFrameEncoder enc;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    uint8_t lsf46[46];
    memset(lsf46, 0, sizeof lsf46);
    memcpy(lsf46, src.data(), 6);
    memcpy(lsf46 + 6, dst.data(), 6);
    lsf46[LSF_FLAGS_OFFSET] = 0;
    if (mode == 1 || mode == 3)
        lsf46[LSF_FLAGS_OFFSET] |= LSF_FLAG_ENCRYPTED;
    if (mode == 2 || mode == 3)
        lsf46[LSF_FLAGS_OFFSET] |= LSF_FLAG_SIGNED;
    lsf46[LSF_VERSION_OFFSET] = 2;
    uint16_t crc = crc_m17(lsf46, 46);
    uint8_t lsfpad[54];
    memset(lsfpad, 0, sizeof lsfpad);
    memcpy(lsfpad, lsf46, 46);
    lsfpad[46] = (uint8_t)(crc >> 8);
    lsfpad[47] = (uint8_t)(crc & 0xFF);
    const int nlsf = 3;
    const int nsig = 6;
    const int nvoice = 300;
    const int ntot = nlsf + nsig + nvoice + 1;
    std::vector<frame_t> tx((size_t)ntot);
    for (int i = 0; i < nlsf; i++)
        m17_encode_chunk(c, lsfpad + i * 18, tx[static_cast<size_t>(i)], true);
    uint8_t siginfo[FEC_INFO_BYTES];
    memset(siginfo, 0x22, sizeof siginfo);
    for (int i = 0; i < nsig; i++) {
        siginfo[0] = (uint8_t)((SIG_FRAME_BASE + i) >> 8);
        siginfo[1] = (uint8_t)(SIG_FRAME_BASE + i);
        m17_encode_chunk(c, siginfo, tx[static_cast<size_t>(nlsf + i)], false);
    }
    std::vector<int16_t> bb48;
    {
        unsigned rng = seed;
        for (int i = 0; i < nvoice; i++) {
            uint8_t info[FEC_INFO_BYTES];
            if (mode == 2) {
                memset(info, 0x11, sizeof info);
                info[0] = (uint8_t)(i >> 8);
                info[1] = (uint8_t)i;
            } else {
                for (size_t b = 0; b < FEC_INFO_BYTES; b++) {
                    rng = rng * 1103515245u + 12345u;
                    info[b] = (uint8_t)(rng >> 16);
                }
            }
            m17_encode_chunk(c, info, tx[static_cast<size_t>(nlsf + nsig + i)],
                             false);
        }
        enc.encodeEotFrame(tx.back());
        if (render_frames(tx, bb48, true) != 0)
            return -1;
    }
    for (int t = 0; t < ntx; t++) {
        std::vector<int16_t> imp, rx24;
        impair_t p{};
        p.noise = noise;
        p.gain = 1.0f;
        p.seed = seed + (unsigned)t * 17u;
        impair_48k(bb48.data(), bb48.size(), p, imp);
        to_24k(imp, rx24);
        std::vector<frame_t> sl;
        slice_ideal(rx24, true, sl);
        TrackStat tr = track_frames(sl, miss_limit, nlsf + nsig + nvoice);
        if (tr.drop == DROP_NEVER && tr.n_locks == 0)
            continue;
        if ((int)sl.size() < ntot)
            continue;
        uint8_t rec[54];
        memset(rec, 0, sizeof rec);
        bool lsf_ok = true;
        for (int i = 0; i < nlsf; i++) {
            uint8_t out[FEC_INFO_BYTES];
            if (!c.m17.decode_hard(sl[static_cast<size_t>(i)].data() + 2,
                                   out)) {
                lsf_ok = false;
                break;
            }
            memcpy(rec + i * 18, out, 18);
        }
        if (!lsf_ok)
            continue;
        uint16_t want = ((uint16_t)rec[46] << 8) | rec[47];
        if (crc_m17(rec, 46) != want)
            continue;
        int vok = 0;
        bool locked = true;
        int missed = 0;
        for (int i = 0; i < nlsf + nsig && locked; i++) {
            const frame_t &f = sl[static_cast<size_t>(i)];
            bool valid = match_sync(f, LSF_SYNC_WORD)
                      || match_sync(f, VOICE_SYNC_WORD);
            bool eot = match_sync(f, EOT_SYNC_WORD);
            if (valid)
                missed = 0;
            else
                missed++;
            if (eot || missed > miss_limit)
                locked = false;
        }
        unsigned rng = seed;
        for (int i = 0; i < nvoice && locked; i++) {
            size_t idx = (size_t)(nlsf + nsig + i);
            if (idx >= sl.size())
                break;
            const frame_t &f = sl[idx];
            bool valid = match_sync(f, LSF_SYNC_WORD)
                      || match_sync(f, VOICE_SYNC_WORD);
            bool eot = match_sync(f, EOT_SYNC_WORD);
            uint8_t info[FEC_INFO_BYTES];
            uint8_t exp[FEC_INFO_BYTES];
            if (mode == 2) {
                memset(exp, 0x11, sizeof exp);
                exp[0] = (uint8_t)(i >> 8);
                exp[1] = (uint8_t)i;
            } else {
                for (size_t b = 0; b < FEC_INFO_BYTES; b++) {
                    rng = rng * 1103515245u + 12345u;
                    exp[b] = (uint8_t)(rng >> 16);
                }
            }
            if (c.m17.decode_hard(f.data() + 2, info)
                && memcmp(info, exp, FEC_INFO_BYTES) == 0)
                vok++;
            if (valid)
                missed = 0;
            else
                missed++;
            if (eot || missed > miss_limit)
                locked = false;
        }
        *got_voice += vok;
        if (vok >= nvoice)
            (*ok)++;
    }
    return 0;
}

static int run_complete_v1(float noise, unsigned seed, int ntx, int *ok)
{
    HorseFrameEncoder enc;
    *ok = 0;
    for (int t = 0; t < ntx; t++) {
        std::vector<frame_t> frames(308);
        call_t src = { { 1, 2, 3, 4, 5, 6 } };
        call_t dst = { { 6, 5, 4, 3, 2, 1 } };
        enc.encodeLsf(src, dst, nullptr, 0, frames[0]);
        uint8_t payload[12];
        uint8_t tag[4] = { 1, 2, 3, 4 };
        memset(payload, 0x11, sizeof payload);
        for (int i = 0; i < 6; i++)
            enc.encodeVoiceFrameWithFn(payload, tag,
                                       (uint16_t)(SIG_FRAME_BASE + i),
                                       frames[static_cast<size_t>(1 + i)],
                                       false, 12);
        for (int i = 0; i < 300; i++)
            enc.encodeVoiceFrameWithFn(payload, tag, (uint16_t)i,
                                       frames[static_cast<size_t>(7 + i)],
                                       false, 12);
        enc.encodeEotFrame(frames.back());
        std::vector<int16_t> bb48, imp, rx24;
        if (render_frames(frames, bb48, true) != 0)
            return -1;
        impair_t p{};
        p.noise = noise;
        p.gain = 1.0f;
        p.seed = seed + (unsigned)t * 17u;
        impair_48k(bb48.data(), bb48.size(), p, imp);
        to_24k(imp, rx24);
        std::vector<frame_t> got;
        demod_frames(rx24, got);
        HorseFrameDecoder dec;
        bool lsf = false, eot = false;
        int voice = 0;
        for (const auto &f : got) {
            auto ty = dec.decodeFrame(f);
            if (ty == HorseFrameType::LINK_SETUP)
                lsf = true;
            if (ty == HorseFrameType::EOT)
                eot = true;
            if (ty == HorseFrameType::VOICE)
                voice++;
        }
        if (lsf && eot && voice >= 300)
            (*ok)++;
    }
    return 0;
}

int main(int argc, char **argv)
{
    Codecs c;
    std::string mode = argc > 1 ? argv[1] : "selftest";
    if (mode == "selftest") {
        uint8_t info[FEC_INFO_BYTES];
        uint8_t cw[FEC_CODED_BYTES];
        uint8_t out[FEC_INFO_BYTES];
        for (int i = 0; i < (int)FEC_INFO_BYTES; i++)
            info[i] = (uint8_t)(0xA5 ^ i);
        float llr[FEC_CODED_BITS];
        int ids[] = { CID_R2, CID_POLAR4, CID_POLAR8, CID_M17H, CID_CCSDS10 };
        for (int id : ids) {
            encode_cid(c, id, info, cw);
            frame_t f{};
            pack_voice(cw, f);
            llr_from_frame_hard(f, llr);
            memset(out, 0, sizeof out);
            if (!decode_cid(c, id, llr, cw, out)
                || memcmp(out, info, FEC_INFO_BYTES) != 0) {
                std::printf("selftest fail %s\n", codec_name(id));
                return 1;
            }
            std::printf("selftest ok %s\n", codec_name(id));
        }
        {
            HorseFrameEncoder enc;
            frame_t lsf{}, voice{}, eot{};
            call_t src = { { 1, 2, 3, 4, 5, 6 } };
            call_t dst = { { 6, 5, 4, 3, 2, 1 } };
            enc.encodeLsf(src, dst, nullptr, 0, lsf);
            uint8_t cw[FEC_CODED_BYTES];
            encode_cid(c, CID_M17H, info, cw);
            pack_voice(cw, voice);
            enc.encodeEotFrame(eot);
            std::vector<int16_t> bb48, rx24;
            if (render_frames({ lsf, voice, eot }, bb48, true) != 0)
                return 1;
            to_24k(bb48, rx24);
            std::vector<frame_t> sl;
            slice_ideal(rx24, true, sl);
            if (sl.size() < 3 || sl[0] != lsf || sl[1] != voice
                || sl[2] != eot) {
                std::printf("selftest fail ideal slice n=%zu\n", sl.size());
                return 1;
            }
            std::printf("selftest ok ideal_slice\n");
        }
        return 0;
    }
    if (mode == "voice") {
        int id = argc > 2 ? atoi(argv[2]) : 0;
        float noise = argc > 3 ? strtof(argv[3], nullptr) : 0.f;
        int n = argc > 4 ? atoi(argv[4]) : 10000;
        unsigned seed = argc > 5 ? (unsigned)atoi(argv[5]) : 1u;
        int ok = 0, got = 0, undet = 0;
        double us = 0;
        if (run_voice_fer(c, id, noise, seed, n, &ok, &got, &undet, &us) != 0)
            return 1;
        std::printf("voice %s noise=%.0f n=%d got=%d ok=%d fer=%.6f undet=%d "
                    "us/frame=%.1f\n",
                    codec_name(id), noise, n, got, ok,
                    n ? 1.0 - (double)ok / n : 1.0, undet, us);
        return 0;
    }
    if (mode == "burst") {
        int nsym = argc > 2 ? atoi(argv[2]) : 8;
        int intl = argc > 3 ? atoi(argv[3]) : 0;
        int n = argc > 4 ? atoi(argv[4]) : 1000;
        for (int id = 0; id < CID_COUNT; id++) {
            if (id == CID_POLAR16 || id >= CID_CCSDS10)
                continue;
            int ok = 0;
            run_burst(c, id, (size_t)nsym, intl != 0, n, &ok);
            std::printf("burst %s nsym=%d intl=%d ok=%d/%d\n", codec_name(id),
                        nsym, intl, ok, n);
        }
        return 0;
    }
    if (mode == "complete_v1") {
        float noise = argc > 2 ? strtof(argv[2], nullptr) : 10000.f;
        int ntx = argc > 3 ? atoi(argv[3]) : 200;
        unsigned seed = argc > 4 ? (unsigned)atoi(argv[4]) : 1u;
        int ok = 0;
        if (run_complete_v1(noise, seed, ntx, &ok) != 0)
            return 1;
        std::printf("complete_v1 noise=%.0f ok=%d/%d\n", noise, ok, ntx);
        return 0;
    }
    if (mode == "study") {
        int id = argc > 2 ? atoi(argv[2]) : 0;
        float noise = argc > 3 ? strtof(argv[3], nullptr) : 0.f;
        int n = argc > 4 ? atoi(argv[4]) : 10000;
        unsigned seed = argc > 5 ? (unsigned)atoi(argv[5]) : 1u;
        VoiceRow d{}, idl{}, c2{}, c4{}, c8{};
        if (run_voice_study(c, id, noise, seed, n, &d, &idl, &c2, &c4, &c8)
            != 0)
            return 1;
        print_voice_row("demod", id, noise, n, d);
        print_voice_row("ideal", id, noise, n, idl);
        print_voice_row("coast2", id, noise, n, c2);
        print_voice_row("coast4", id, noise, n, c4);
        print_voice_row("coast8", id, noise, n, c8);
        if (d.got > 0) {
            std::printf("study last_demod_note drop=%d lock_len=%d\n",
                        d.tr.drop, d.tr.lock_len);
        }
        return 0;
    }
    if (mode == "hist") {
        int n = argc > 2 ? atoi(argv[2]) : 10000;
        unsigned seed = argc > 3 ? (unsigned)atoi(argv[3]) : 1u;
        int ids[] = { CID_R2, CID_POLAR4, CID_M17H };
        for (int id : ids) {
            int h[4];
            hist_payload(id, c, seed, n, h);
            int tot = h[0] + h[1] + h[2] + h[3];
            std::printf("hist %s n=%d +3=%d +1=%d -1=%d -3=%d tot=%d\n",
                        codec_name(id), n, h[0], h[1], h[2], h[3], tot);
        }
        return 0;
    }
    if (mode == "lockstat") {
        float noise = argc > 2 ? strtof(argv[2], nullptr) : 11000.f;
        int n = argc > 3 ? atoi(argv[3]) : 10000;
        unsigned seed = argc > 4 ? (unsigned)atoi(argv[4]) : 1u;
        return run_lockstat(noise, seed, n);
    }
    if (mode == "eot_notice") {
        float noise = argc > 2 ? strtof(argv[2], nullptr) : 0.f;
        int miss = argc > 3 ? atoi(argv[3]) : 4;
        int nvoice = argc > 4 ? atoi(argv[4]) : 20;
        unsigned seed = argc > 5 ? (unsigned)atoi(argv[5]) : 1u;
        return run_eot_notice(noise, seed, nvoice, miss);
    }
    if (mode == "polar_prof") {
        int n = argc > 2 ? atoi(argv[2]) : 50;
        uint8_t info[FEC_INFO_BYTES];
        uint8_t cw[FEC_CODED_BYTES];
        uint8_t out[FEC_INFO_BYTES];
        for (int i = 0; i < (int)FEC_INFO_BYTES; i++)
            info[i] = (uint8_t)(0xA5 ^ i);
        encode_cid(c, CID_POLAR4, info, cw);
        float llr[FEC_CODED_BITS];
        frame_t f{};
        pack_voice(cw, f);
        llr_from_frame_hard(f, llr);
        c.polar.n_path_copy = c.polar.n_encode_n = c.polar.n_fcomb = 0;
        c.polar.n_leaf = 0;
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < n; i++) {
            memset(out, 0, sizeof out);
            if (!c.polar.decode(llr, 4, out))
                return 1;
        }
        auto t1 = std::chrono::steady_clock::now();
        double us = std::chrono::duration<double, std::micro>(t1 - t0).count()
                  / n;
        std::printf("polar_prof n=%d us/frame=%.1f path_copy/frame=%.0f "
                    "encode_n/frame=%.0f fcomb/frame=%.0f leaf/frame=%.0f\n",
                    n, us, (double)c.polar.n_path_copy / n,
                    (double)c.polar.n_encode_n / n, (double)c.polar.n_fcomb / n,
                    (double)c.polar.n_leaf / n);
        return 0;
    }
    if (mode == "complete_v2") {
        float noise = argc > 2 ? strtof(argv[2], nullptr) : 10000.f;
        int ntx = argc > 3 ? atoi(argv[3]) : 200;
        unsigned seed = argc > 4 ? (unsigned)atoi(argv[4]) : 1u;
        int miss = argc > 5 ? atoi(argv[5]) : 4;
        int md = argc > 6 ? atoi(argv[6]) : 1;
        int ok = 0, gv = 0;
        if (run_complete_v2(c, noise, seed, ntx, miss, md, &ok, &gv) != 0)
            return 1;
        const char *mn[] = { "?", "encrypt", "sign", "both" };
        std::printf("complete_v2 mode=%s miss_limit=%d noise=%.0f ok=%d/%d "
                    "voice_ok=%d\n",
                    mn[md], miss, noise, ok, ntx, gv);
        return 0;
    }
    std::printf("usage: horse_fec_v2_sim selftest|voice|burst|complete_v1|"
                "study|hist|lockstat|eot_notice|polar_prof|complete_v2\n");
    return 1;
}

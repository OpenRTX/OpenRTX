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

static void push_lsf_sim(HorseFrameEncoder &enc, const call_t &src,
                         const call_t &dst, std::vector<frame_t> &frames)
{
    frame_t lsf[LSF_OPENING_FRAMES];
    enc.encodeLsf(src, dst, nullptr, 0, lsf);
    for (size_t i = 0; i < LSF_OPENING_FRAMES; i++)
        frames.push_back(lsf[i]);
}


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
    HorseFrameDecoder decoder;
    demod.init();
    demod.resetImmediate();
    demod.setSkipDcBlock(true);
    out.clear();
    for (int16_t s : rx24) {
        demod.feedSample(s, false);
        frame_t f{};
        if (!demod.takeFrame(f))
            continue;
        if (decoder.decodeFrame(f) == HorseFrameType::LINK_SETUP)
            demod.noteValidTag();
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

static void apply_code_randomiser(int id, uint8_t cw[FEC_CODED_BYTES])
{
    if (id == CID_M17H || id == CID_M17S)
        return;
    std::array<uint8_t, FEC_CODED_BYTES> a{};
    memcpy(a.data(), cw, FEC_CODED_BYTES);
    M17::decorrelate(a);
    memcpy(cw, a.data(), FEC_CODED_BYTES);
}

struct Codecs {
    Repeat2Codec r2;
    PolarCodec polar;
    PolarLsfCodec polar_lsf;
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
    apply_code_randomiser(id, cw);
}

static bool decode_cid(Codecs &c, int id, const float *llr,
                       const uint8_t *hard46, uint8_t *info)
{
    uint8_t der[FEC_CODED_BYTES];
    memcpy(der, hard46, FEC_CODED_BYTES);
    apply_code_randomiser(id, der);
    float llr2[FEC_CODED_BITS];
    memcpy(llr2, llr, sizeof llr2);
    if (id != CID_M17H && id != CID_M17S) {
        for (size_t i = 0; i < FEC_CODED_BITS; i++) {
            bool b = (der[i / 8] >> (7 - (i % 8))) & 1u;
            bool oldb = (hard46[i / 8] >> (7 - (i % 8))) & 1u;
            if (b != oldb)
                llr2[i] = -llr2[i];
        }
    }
    if (id == CID_R2)
        return c.r2.decode_hard(der, info);
    if (id == CID_POLAR4)
        return c.polar.decode(llr2, 4, info);
    if (id == CID_POLAR8)
        return c.polar.decode(llr2, 8, info);
    if (id == CID_POLAR16)
        return c.polar.decode(llr2, 16, info);
    if (id == CID_M17H)
        return c.m17.decode_hard(der, info);
    if (id == CID_M17S)
        return c.m17.decode_soft(llr2, info);
    int it = 10;
    if (id == CID_CCSDS20)
        it = 20;
    if (id == CID_CCSDS50)
        it = 50;
    return c.ccsds.decode_minsum(llr2, it, info);
}

static int run_voice_fer(Codecs &c, int id, float noise, unsigned seed,
                         int nframes, int *ok, int *got, int *undet,
                         double *us_per)
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    push_lsf_sim(enc, src, dst, frames);
    std::vector<std::array<uint8_t, FEC_INFO_BYTES>> refs(
        static_cast<size_t>(nframes));
    unsigned rng = seed;
    for (int i = 0; i < nframes; i++) {
        for (size_t b = 0; b < FEC_INFO_BYTES; b++) {
            rng = rng * 1103515245u + 12345u;
            refs[static_cast<size_t>(i)][b] = (uint8_t)(rng >> 16);
        }
        refs[static_cast<size_t>(i)][0] = (uint8_t)(i >> 8);
        refs[static_cast<size_t>(i)][1] = (uint8_t)i;
        uint8_t cw[FEC_CODED_BYTES];
        encode_cid(c, id, refs[static_cast<size_t>(i)].data(), cw);
        frame_t vf; pack_voice(cw, vf); frames.push_back(vf);
    }
    { frame_t ef; enc.encodeEotFrame(ef); frames.push_back(ef); }

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
        unsigned fn = ((unsigned)out[0] << 8) | out[1];
        if (pass && fn < (unsigned)nframes
            && memcmp(out, refs[fn].data(), FEC_INFO_BYTES) == 0)
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

static int decode_voice_fn(Codecs &c, int id, const frame_t &f, int nframes,
                           const std::vector<std::array<uint8_t, FEC_INFO_BYTES>>
                               &refs)
{
    float llr[FEC_CODED_BITS];
    llr_from_frame_hard(f, llr);
    uint8_t out[FEC_INFO_BYTES];
    if (!decode_cid(c, id, llr, f.data() + 2, out))
        return 0;
    unsigned fn = ((unsigned)out[0] << 8) | out[1];
    if (fn >= (unsigned)nframes)
        return 0;
    return memcmp(out, refs[fn].data(), FEC_INFO_BYTES) == 0 ? 1 : 0;
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
    refs.resize((size_t)nframes);
    HorseFrameEncoder enc;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    push_lsf_sim(enc, src, dst, frames);
    unsigned rng = seed;
    for (int i = 0; i < nframes; i++) {
        uint8_t info[FEC_INFO_BYTES];
        for (size_t b = 0; b < FEC_INFO_BYTES; b++) {
            rng = rng * 1103515245u + 12345u;
            info[b] = (uint8_t)(rng >> 16);
        }
        info[0] = (uint8_t)(i >> 8);
        info[1] = (uint8_t)i;
        memcpy(refs[static_cast<size_t>(i)].data(), info, FEC_INFO_BYTES);
        uint8_t cw[FEC_CODED_BYTES];
        encode_cid(c, id, info, cw);
        frame_t vf; pack_voice(cw, vf); frames.push_back(vf);
    }
    { frame_t ef; enc.encodeEotFrame(ef); frames.push_back(ef); }
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
        int good = decode_voice_fn(c, id, f, nframes, refs);
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
    std::vector<frame_t> frames;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    push_lsf_sim(enc, src, dst, frames);
    uint8_t payload[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(payload, 0x5A, sizeof payload);
    for (int i = 0; i < nframes; i++) {
        frame_t vf{};
        enc.encodeVoiceFrameWithFn(payload, tag, (uint16_t)i, vf, false, 12);
        frames.push_back(vf);
    }
    { frame_t ef; enc.encodeEotFrame(ef); frames.push_back(ef); }
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
    int eot_i = (int)LSF_OPENING_FRAMES + nframes;
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

static int run_clktrace(float noise, unsigned seed, int nframes, int auth)
{
    HorseFrameEncoder enc;
    HorseFrameDecoder dec;
    std::vector<frame_t> frames;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    push_lsf_sim(enc, src, dst, frames);
    uint8_t payload[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(payload, 0x5A, sizeof payload);
    for (int i = 0; i < nframes; i++) {
        frame_t vf{};
        enc.encodeVoiceFrameWithFn(payload, tag, (uint16_t)i, vf, false, 12);
        frames.push_back(vf);
    }
    { frame_t ef; enc.encodeEotFrame(ef); frames.push_back(ef); }
    std::vector<int16_t> bb48, imp, rx24;
    if (render_frames(frames, bb48, true) != 0)
        return -1;
    impair_t p{};
    p.noise = noise;
    p.gain = 1.0f;
    p.seed = seed;
    impair_48k(bb48.data(), bb48.size(), p, imp);
    to_24k(imp, rx24);

    HorseDemodulator demod;
    demod.init();
    demod.resetImmediate();
    demod.setSkipDcBlock(true);
    int got = 0;
    int jumps = 0;
    int max_abs_d = 0;
    uint32_t last_sp = 0;
    bool have_sp = false;
    std::printf("clktrace n=%d noise=%.0f auth=%d seed=%u\n", nframes, noise,
                auth, seed);
    std::printf("# i sp d hd_v hd_e auth locked fi\n");
    for (size_t si = 0; si < rx24.size(); si++) {
        demod.feedSample(rx24[si], false);
        frame_t f{};
        if (!demod.takeFrame(f))
            continue;
        if (auth && dec.decodeFrame(f) == HorseFrameType::LINK_SETUP
            && dec.lsfReady())
            demod.noteValidTag();
        uint32_t sp = demod.debugSamplingPoint();
        int8_t dlt = demod.debugLastClockDelta();
        int ad = dlt < 0 ? -dlt : dlt;
        if (ad > max_abs_d)
            max_abs_d = ad;
        if (have_sp && sp != last_sp)
            jumps++;
        last_sp = sp;
        have_sp = true;
        if (got < 25 || got + 25 >= nframes || ad >= 2 || !demod.isLocked()) {
            std::printf("%d %u %d %d %d %d %d %d\n", got, sp, (int)dlt,
                        hd_sync(f, VOICE_SYNC_WORD),
                        hd_sync(f, EOT_SYNC_WORD),
                        demod.lockAuthenticated() ? 1 : 0,
                        demod.isLocked() ? 1 : 0,
                        (int)demod.debugFrameIndex());
        }
        got++;
    }
    std::printf("clktrace got=%d jumps=%d max|d|=%d locked=%d fi=%u\n", got,
                jumps, max_abs_d, demod.isLocked() ? 1 : 0,
                (unsigned)demod.debugFrameIndex());
    demod.terminate();
    return 0;
}

static int run_eot_notice(float noise, unsigned seed, int nvoice,
                          int miss_limit)
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    push_lsf_sim(enc, src, dst, frames);
    uint8_t payload[12];
    uint8_t tag[4] = { 1, 2, 3, 4 };
    memset(payload, 0x11, sizeof payload);
    for (int i = 0; i < nvoice; i++) {
        frame_t vf{};
        enc.encodeVoiceFrameWithFn(payload, tag, (uint16_t)i, vf, false, 12);
        frames.push_back(vf);
    }
    { frame_t ef; enc.encodeEotFrame(ef); frames.push_back(ef); }
    for (int i = 0; i < 20; i++) {
        frame_t z{};
        memset(z.data(), 0, z.size());
        frames.push_back(z);
    }
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
        demod_frames(rx24, sl);
        if ((int)sl.size() < nlsf)
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

static void pack_lsf_payload(const uint8_t *cw46, frame_t &out)
{
    std::copy(LSF_SYNC_WORD.begin(), LSF_SYNC_WORD.end(), out.begin());
    memcpy(out.data() + 2, cw46, FEC_CODED_BYTES);
}

static int run_lsf_cmp(Codecs &c, int kind, int nfr, float noise,
                       unsigned seed, int ntx, int *ok, double *us,
                       size_t *ram)
{
    *ok = 0;
    *us = 0;
    *ram = 0;
    HorseFrameEncoder enc;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    uint8_t lsf46[46];
    memset(lsf46, 0, sizeof lsf46);
    memcpy(lsf46, src.data(), 6);
    memcpy(lsf46 + 6, dst.data(), 6);
    lsf46[LSF_FLAGS_OFFSET] = LSF_FLAG_SIGNED;
    lsf46[LSF_VERSION_OFFSET] = 2;
    uint16_t crc = crc_m17(lsf46, 46);
    uint8_t lsf48[48];
    memcpy(lsf48, lsf46, 46);
    lsf48[46] = (uint8_t)(crc >> 8);
    lsf48[47] = (uint8_t)(crc & 0xFF);
    uint8_t lsfpad[54];
    memset(lsfpad, 0, sizeof lsfpad);
    memcpy(lsfpad, lsf48, 48);

    std::vector<frame_t> tx((size_t)nfr + 1);
    const size_t E = (size_t)nfr * FEC_CODED_BITS;
    std::vector<uint8_t> polar_cw((E + 7) / 8);
    if (kind == 0) {
        for (int i = 0; i < nfr; i++) {
            int src_i = (i < 3) ? i : 0;
            m17_encode_chunk(c, lsfpad + src_i * 18, tx[static_cast<size_t>(i)],
                             true);
        }
        *ram = 18 * 3 + 368 * 2;
    } else if (kind == 1) {
        uint8_t copy[FEC_CODED_BYTES];
        memcpy(copy, lsf46, 44);
        uint16_t c16 = crc_m17(copy, 44);
        copy[44] = (uint8_t)(c16 >> 8);
        copy[45] = (uint8_t)(c16 & 0xFF);
        for (int i = 0; i < nfr; i++)
            pack_lsf_payload(copy, tx[static_cast<size_t>(i)]);
        *ram = 46;
    } else if (kind == 2) {
        c.polar_lsf.encode(lsf48, E, polar_cw.data());
        for (int i = 0; i < nfr; i++) {
            uint8_t cw[FEC_CODED_BYTES];
            memcpy(cw, polar_cw.data() + (size_t)i * FEC_CODED_BYTES,
                   FEC_CODED_BYTES);
            std::array<uint8_t, FEC_CODED_BYTES> a{};
            memcpy(a.data(), cw, FEC_CODED_BYTES);
            M17::decorrelate(a);
            pack_lsf_payload(a.data(), tx[static_cast<size_t>(i)]);
        }
        *ram = sizeof(PolarLsfCodec::SclPath) * 4;
    } else {
        std::printf("lsf_cmp kind=ldpc skipped: ETSI TS 138 212 V16.2.0 "
                    "clause 5.3.2 Table 5.3.2-3 (BG2 V_i,j) is not a complete "
                    "verified matrix in this tree; CCSDS 231.0-B-4 (512,256) "
                    "does not rate-match to 368 bits. No H was constructed.\n");
        return 0;
    }
    enc.encodeEotFrame(tx.back());
    std::vector<int16_t> bb48;
    if (render_frames(tx, bb48, true) != 0)
        return -1;
    double tsum = 0;
    int nd = 0;
    for (int t = 0; t < ntx; t++) {
        std::vector<int16_t> imp, rx24;
        impair_t p{};
        p.noise = noise;
        p.gain = 1.0f;
        p.seed = seed + (unsigned)t * 17u;
        impair_48k(bb48.data(), bb48.size(), p, imp);
        to_24k(imp, rx24);
        std::vector<frame_t> sl;
        demod_frames(rx24, sl);
        std::vector<frame_t> lsfs;
        for (const auto &f : sl) {
            if (match_sync(f, LSF_SYNC_WORD))
                lsfs.push_back(f);
        }
        bool pass = false;
        auto t0 = std::chrono::steady_clock::now();
        if (kind == 0) {
            uint8_t rec[54];
            memset(rec, 0, sizeof rec);
            bool okc[3] = { false, false, false };
            for (size_t i = 0; i < lsfs.size(); i++) {
                uint8_t out[FEC_INFO_BYTES];
                if (!c.m17.decode_hard(lsfs[i].data() + 2, out))
                    continue;
                int slot = (i < 3) ? (int)i : 0;
                memcpy(rec + slot * 18, out, 18);
                okc[slot] = true;
            }
            uint16_t want = ((uint16_t)rec[46] << 8) | rec[47];
            pass = okc[0] && okc[1] && okc[2] && crc_m17(rec, 46) == want;
        } else if (kind == 1) {
            for (const auto &f : lsfs) {
                const uint8_t *p = f.data() + 2;
                uint16_t want = ((uint16_t)p[44] << 8) | p[45];
                if (crc_m17(p, 44) == want && memcmp(p, lsf46, 44) == 0) {
                    pass = true;
                    break;
                }
            }
        } else if (kind == 2) {
            if (lsfs.size() >= (size_t)nfr) {
                std::vector<float> llr(E);
                std::vector<uint8_t> hard((E + 7) / 8, 0);
                for (int i = 0; i < nfr; i++) {
                    uint8_t cw[FEC_CODED_BYTES];
                    memcpy(cw, lsfs[static_cast<size_t>(i)].data() + 2,
                           FEC_CODED_BYTES);
                    std::array<uint8_t, FEC_CODED_BYTES> a{};
                    memcpy(a.data(), cw, FEC_CODED_BYTES);
                    M17::decorrelate(a);
                    memcpy(hard.data() + (size_t)i * FEC_CODED_BYTES, a.data(),
                           FEC_CODED_BYTES);
                }
                for (size_t i = 0; i < E; i++) {
                    bool b = (hard[i / 8] >> (7 - (i % 8))) & 1u;
                    llr[i] = b ? -8.f : 8.f;
                }
                uint8_t out[48];
                pass = c.polar_lsf.decode(llr.data(), E, 4, out)
                    && memcmp(out, lsf48, 48) == 0;
            }
        }
        auto t1 = std::chrono::steady_clock::now();
        tsum += std::chrono::duration<double, std::micro>(t1 - t0).count();
        nd++;
        if (pass)
            (*ok)++;
    }
    *us = nd ? tsum / nd : 0;
    return 0;
}

/* Option C spare after DATA_PUNCTURE: 368 - 272 = 96 bits (12 bytes). */
static constexpr size_t OPT_C_PUNCT_BYTES = 34;
static constexpr size_t OPT_C_SPARE_BITS = FEC_CODED_BITS - OPT_C_PUNCT_BYTES * 8;
static constexpr size_t OPT_C_SPARE_BYTES = OPT_C_SPARE_BITS / 8;
static constexpr size_t LSF_FRAG_COUNT = 4;

static void m17_encode_voice_frag(const uint8_t info18[18],
                                  const uint8_t frag12[OPT_C_SPARE_BYTES],
                                  frame_t &out)
{
    M17::ConvolutionalEncoder enc;
    std::array<uint8_t, 37> encoded{};
    enc.reset();
    enc.encode(info18, encoded.data(), FEC_INFO_BYTES);
    encoded[36] = (uint8_t)enc.flush();
    std::array<uint8_t, OPT_C_PUNCT_BYTES> punct{};
    M17::puncture(encoded, punct, M17::DATA_PUNCTURE);
    std::array<uint8_t, FEC_CODED_BYTES> frame{};
    memcpy(frame.data(), punct.data(), OPT_C_PUNCT_BYTES);
    for (size_t i = 0; i < OPT_C_SPARE_BITS; i++) {
        bool b = (frag12[i / 8] >> (7 - (i % 8))) & 1u;
        M17::setBit(frame, OPT_C_PUNCT_BYTES * 8 + i, b);
    }
    M17::interleave(frame);
    M17::decorrelate(frame);
    std::copy(VOICE_SYNC_WORD.begin(), VOICE_SYNC_WORD.end(), out.begin());
    memcpy(out.data() + 2, frame.data(), FEC_CODED_BYTES);
}

static bool extract_voice_frag(const frame_t &f,
                               uint8_t frag12[OPT_C_SPARE_BYTES])
{
    std::array<uint8_t, FEC_CODED_BYTES> frame{};
    memcpy(frame.data(), f.data() + 2, FEC_CODED_BYTES);
    M17::decorrelate(frame);
    M17::deinterleave(frame);
    memset(frag12, 0, OPT_C_SPARE_BYTES);
    for (size_t i = 0; i < OPT_C_SPARE_BITS; i++) {
        if (M17::getBit(frame, OPT_C_PUNCT_BYTES * 8 + i))
            frag12[i / 8] |= (uint8_t)(0x80u >> (i % 8));
    }
    return true;
}

/*
 * Opening LSF (M17 3/4-chunk) plus option-C voice spare fragments.
 * erase_open=1 zeroes LSF payloads (keeps sync) so the demod locks but
 * opening decode fails; success then requires fragment rebuild.
 */
static int run_lsf_frag(Codecs &c, int nfr, int erase_open, float noise,
                        unsigned seed, int ntx, int nvoice, int *ok_open,
                        int *ok_frag, int *start_fr)
{
    *ok_open = 0;
    *ok_frag = 0;
    *start_fr = 0;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    uint8_t lsf46[46];
    memset(lsf46, 0, sizeof lsf46);
    memcpy(lsf46, src.data(), 6);
    memcpy(lsf46 + 6, dst.data(), 6);
    lsf46[LSF_FLAGS_OFFSET] = LSF_FLAG_SIGNED;
    lsf46[LSF_VERSION_OFFSET] = 2;
    uint8_t lsf48[48];
    memcpy(lsf48, lsf46, 46);
    uint16_t crc = crc_m17(lsf46, 46);
    lsf48[46] = (uint8_t)(crc >> 8);
    lsf48[47] = (uint8_t)(crc & 0xFF);
    uint8_t lsfpad[54];
    memset(lsfpad, 0, sizeof lsfpad);
    memcpy(lsfpad, lsf48, 48);

    HorseFrameEncoder enc;
    std::vector<frame_t> tx((size_t)nfr + (size_t)nvoice + 1);
    for (int i = 0; i < nfr; i++) {
        int src_i = (i < 3) ? i : 0;
        m17_encode_chunk(c, lsfpad + src_i * 18, tx[static_cast<size_t>(i)],
                         true);
        if (erase_open)
            memset(tx[static_cast<size_t>(i)].data() + 2, 0, FEC_CODED_BYTES);
    }
    uint8_t info[FEC_INFO_BYTES];
    memset(info, 0x55, sizeof info);
    for (int i = 0; i < nvoice; i++) {
        info[0] = (uint8_t)((i >> 8) & 0xFF);
        info[1] = (uint8_t)(i & 0xFF);
        uint8_t frag[OPT_C_SPARE_BYTES];
        memcpy(frag, lsf48 + (i % (int)LSF_FRAG_COUNT) * (int)OPT_C_SPARE_BYTES,
               OPT_C_SPARE_BYTES);
        m17_encode_voice_frag(info, frag, tx[static_cast<size_t>(nfr + i)]);
    }
    enc.encodeEotFrame(tx.back());
    std::vector<int16_t> bb48;
    if (render_frames(tx, bb48, true) != 0)
        return -1;

    int start_sum = 0;
    int start_n = 0;
    for (int t = 0; t < ntx; t++) {
        std::vector<int16_t> imp, rx24;
        impair_t p{};
        p.noise = noise;
        p.gain = 1.0f;
        p.seed = seed + (unsigned)t * 17u;
        impair_48k(bb48.data(), bb48.size(), p, imp);
        to_24k(imp, rx24);
        std::vector<frame_t> sl;
        demod_frames(rx24, sl);
        std::vector<frame_t> lsfs;
        std::vector<frame_t> voices;
        for (const auto &f : sl) {
            if (match_sync(f, LSF_SYNC_WORD))
                lsfs.push_back(f);
            else if (match_sync(f, VOICE_SYNC_WORD))
                voices.push_back(f);
        }
        bool open_ok = false;
        uint8_t rec[54];
        memset(rec, 0, sizeof rec);
        bool okc[3] = { false, false, false };
        for (size_t i = 0; i < lsfs.size(); i++) {
            uint8_t out[FEC_INFO_BYTES];
            if (!c.m17.decode_hard(lsfs[i].data() + 2, out))
                continue;
            int slot = (i < 3) ? (int)i : 0;
            memcpy(rec + slot * 18, out, 18);
            okc[slot] = true;
        }
        uint16_t want = ((uint16_t)rec[46] << 8) | rec[47];
        open_ok = okc[0] && okc[1] && okc[2] && crc_m17(rec, 46) == want
            && memcmp(rec, lsf48, 48) == 0;
        if (open_ok)
            (*ok_open)++;

        uint8_t have[LSF_FRAG_COUNT] = { 0 };
        uint8_t assem[48];
        memset(assem, 0, sizeof assem);
        int frag_at = -1;
        for (size_t vi = 0; vi < voices.size(); vi++) {
            uint8_t frag[OPT_C_SPARE_BYTES];
            extract_voice_frag(voices[vi], frag);
            uint8_t info_out[FEC_INFO_BYTES];
            c.m17.decode_hard(voices[vi].data() + 2, info_out);
            int fn = ((int)info_out[0] << 8) | info_out[1];
            int slot = fn % (int)LSF_FRAG_COUNT;
            memcpy(assem + slot * (int)OPT_C_SPARE_BYTES, frag,
                   OPT_C_SPARE_BYTES);
            have[slot] = 1;
            if (have[0] && have[1] && have[2] && have[3]) {
                uint16_t w = ((uint16_t)assem[46] << 8) | assem[47];
                if (crc_m17(assem, 46) == w && memcmp(assem, lsf48, 48) == 0) {
                    frag_at = (int)vi;
                    break;
                }
            }
        }
        bool frag_ok = open_ok || frag_at >= 0;
        if (frag_ok)
            (*ok_frag)++;
        if (frag_ok) {
            int st = open_ok ? 0 : (nfr + frag_at);
            start_sum += st;
            start_n++;
        }
    }
    *start_fr = start_n ? (start_sum + start_n / 2) / start_n : -1;
    return 0;
}

static int run_complete_v1(float noise, unsigned seed, int ntx, int *ok)
{
    HorseFrameEncoder enc;
    *ok = 0;
    for (int t = 0; t < ntx; t++) {
        std::vector<frame_t> frames;
        call_t src = { { 1, 2, 3, 4, 5, 6 } };
        call_t dst = { { 6, 5, 4, 3, 2, 1 } };
        push_lsf_sim(enc, src, dst, frames);
        uint8_t payload[12];
        uint8_t tag[4] = { 1, 2, 3, 4 };
        memset(payload, 0x11, sizeof payload);
        for (int i = 0; i < 6; i++) {
            frame_t vf{};
            enc.encodeVoiceFrameWithFn(payload, tag,
                                       (uint16_t)(SIG_FRAME_BASE + i),
                                       vf, false, 12);
            frames.push_back(vf);
        }
        for (int i = 0; i < 300; i++) {
            frame_t vf{};
            enc.encodeVoiceFrameWithFn(payload, tag, (uint16_t)i,
                                       vf, false, 12);
            frames.push_back(vf);
        }
        { frame_t ef; enc.encodeEotFrame(ef); frames.push_back(ef); }
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
            uint8_t lsf48[48];
            uint8_t out[48];
            memset(lsf48, 0x3C, sizeof lsf48);
            size_t E = 3 * FEC_CODED_BITS;
            std::vector<uint8_t> cw((E + 7) / 8);
            c.polar_lsf.encode(lsf48, E, cw.data());
            std::vector<float> llr(E);
            for (size_t i = 0; i < E; i++) {
                bool b = (cw[i / 8] >> (7 - (i % 8))) & 1u;
                llr[i] = b ? -8.f : 8.f;
            }
            if (!c.polar_lsf.decode(llr.data(), E, 4, out)
                || memcmp(out, lsf48, 48) != 0) {
                std::printf("selftest fail polar_lsf dec=%d\n",
                            c.polar_lsf.decode(llr.data(), E, 4, out) ? 1 : 0);
                return 1;
            }
            std::printf("selftest ok polar_lsf\n");
        }
        {
            HorseFrameEncoder enc;
            frame_t lsf3[LSF_OPENING_FRAMES], voice{}, eot{};
            call_t src = { { 1, 2, 3, 4, 5, 6 } };
            call_t dst = { { 6, 5, 4, 3, 2, 1 } };
            enc.encodeLsf(src, dst, nullptr, 0, lsf3);
            std::vector<frame_t> lsfv(lsf3, lsf3 + LSF_OPENING_FRAMES);
            uint8_t cw[FEC_CODED_BYTES];
            encode_cid(c, CID_M17H, info, cw);
            pack_voice(cw, voice);
            enc.encodeEotFrame(eot);
            std::vector<int16_t> bb48, rx24;
            lsfv.push_back(voice); lsfv.push_back(eot);
            if (render_frames(lsfv, bb48, true) != 0)
                return 1;
            to_24k(bb48, rx24);
            std::vector<frame_t> sl;
            slice_ideal(rx24, true, sl);
            if (sl.size() < LSF_OPENING_FRAMES + 2
                || sl[LSF_OPENING_FRAMES] != voice
                || sl[LSF_OPENING_FRAMES + 1] != eot) {
                std::printf("selftest fail ideal slice n=%zu\n", sl.size());
                return 1;
            }
            for (size_t i = 0; i < LSF_OPENING_FRAMES; i++) {
                if (sl[i] != lsf3[i]) {
                    std::printf("selftest fail ideal LSF %zu\n", i);
                    return 1;
                }
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
    if (mode == "clktrace") {
        float noise = argc > 2 ? strtof(argv[2], nullptr) : 12000.f;
        int n = argc > 3 ? atoi(argv[3]) : 400;
        unsigned seed = argc > 4 ? (unsigned)atoi(argv[4]) : 1u;
        int auth = argc > 5 ? atoi(argv[5]) : 0;
        return run_clktrace(noise, seed, n, auth);
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
        c.polar.encode(info, cw);
        float llr[FEC_CODED_BITS];
        frame_t f{};
        pack_voice(cw, f);
        llr_from_frame_hard(f, llr);
        for (size_t L : { (size_t)4, (size_t)8 }) {
            c.polar.n_path_copy = c.polar.n_encode_n = c.polar.n_fcomb = 0;
            c.polar.n_leaf = 0;
            auto t0 = std::chrono::steady_clock::now();
            for (int i = 0; i < n; i++) {
                memset(out, 0, sizeof out);
                if (!c.polar.decode(llr, L, out)) {
                    std::printf("polar_prof L=%zu decode fail\n", L);
                    return 1;
                }
            }
            auto t1 = std::chrono::steady_clock::now();
            double us = std::chrono::duration<double, std::micro>(t1 - t0)
                            .count()
                      / n;
            std::printf("polar_prof L=%zu n=%d us/frame=%.1f "
                        "path_copy/frame=%.0f encode_n/frame=%.0f "
                        "fcomb/frame=%.0f leaf/frame=%.0f sizeof_path=%zu\n",
                        L, n, us, (double)c.polar.n_path_copy / n,
                        (double)c.polar.n_encode_n / n,
                        (double)c.polar.n_fcomb / n,
                        (double)c.polar.n_leaf / n, sizeof(PolarCodec::SclPath));
        }
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
    if (mode == "lsf_cmp") {
        int kind = argc > 2 ? atoi(argv[2]) : 0;
        int nfr = argc > 3 ? atoi(argv[3]) : 3;
        float noise = argc > 4 ? strtof(argv[4], nullptr) : 10000.f;
        int ntx = argc > 5 ? atoi(argv[5]) : 200;
        unsigned seed = argc > 6 ? (unsigned)atoi(argv[6]) : 1u;
        int ok = 0;
        double us = 0;
        size_t ram = 0;
        const char *kn[] = { "m17_3chunk", "repeat_crc", "polar_block",
                             "ldpc" };
        if (run_lsf_cmp(c, kind, nfr, noise, seed, ntx, &ok, &us, &ram) != 0)
            return 1;
        if (kind == 3)
            return 0;
        std::printf("lsf_cmp %s frames=%d noise=%.0f ok=%d/%d loss=%.4f "
                    "us/decode=%.1f ram=%zu\n",
                    kn[kind], nfr, noise, ok, ntx,
                    ntx ? 1.0 - (double)ok / ntx : 1.0, us, ram);
        return 0;
    }
    if (mode == "lsf_frag") {
        int nfr = argc > 2 ? atoi(argv[2]) : 3;
        int erase = argc > 3 ? atoi(argv[3]) : 0;
        float noise = argc > 4 ? strtof(argv[4], nullptr) : 10000.f;
        int ntx = argc > 5 ? atoi(argv[5]) : 200;
        unsigned seed = argc > 6 ? (unsigned)atoi(argv[6]) : 1u;
        int nvoice = argc > 7 ? atoi(argv[7]) : 40;
        int ok_o = 0, ok_f = 0, st = 0;
        if (run_lsf_frag(c, nfr, erase, noise, seed, ntx, nvoice, &ok_o, &ok_f,
                         &st)
            != 0)
            return 1;
        std::printf("lsf_frag nfr=%d erase_open=%d noise=%.0f nvoice=%d "
                    "open=%d/%d frag_or_open=%d/%d mean_start_fr=%d "
                    "spare_bits=%zu\n",
                    nfr, erase, noise, nvoice, ok_o, ntx, ok_f, ntx, st,
                    OPT_C_SPARE_BITS);
        return 0;
    }
    std::printf("usage: horse_fec_v2_sim selftest|voice|burst|complete_v1|"
                "study|hist|lockstat|clktrace|eot_notice|polar_prof|"
                "complete_v2|lsf_cmp|lsf_frag\n");
    return 1;
}

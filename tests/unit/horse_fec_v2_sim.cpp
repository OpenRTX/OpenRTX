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
#include <string>
#include <vector>

using namespace horse;

static constexpr size_t SPS_48 = 48000 / SYMBOL_RATE;

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
    static const char *n[] = { "repeat2", "polar_L4", "polar_L8", "polar_L16",
                               "m17_hard", "m17_soft", "ccsds_I10", "ccsds_I20",
                               "ccsds_I50" };
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

static bool decode_cid(Codecs &c, int id, const float *llr, const uint8_t *hard46,
                       uint8_t *info)
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
    std::vector<std::array<uint8_t, FEC_INFO_BYTES> > refs(
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
    std::printf("usage: horse_fec_v2_sim selftest|voice|burst|complete_v1\n");
    return 1;
}

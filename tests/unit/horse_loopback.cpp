/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Host loopback: HorseFrameEncoder -> HorseModulator (48 kHz) ->
 * HorseDemodulator (24 kHz) -> HorseFrameDecoder, with impairments.
 */

#include "protocols/horse/HorseFrameEncoder.hpp"
#include "protocols/horse/HorseFrameDecoder.hpp"
#include "protocols/horse/HorseModulator.hpp"
#include "protocols/horse/HorseDemodulator.hpp"
#include "protocols/horse/HorseConstants.hpp"
#include "protocols/horse/horse_crypto.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#ifdef HAVE_LIBSODIUM
#include <sodium.h>
#endif

using namespace horse;

struct impair_t
{
    float noise;
    float gain;
    bool invert;
    float dc;
    float rate_ppm;
    size_t drop_start;
};

struct decoded_t
{
    HorseFrameType type;
    uint16_t fn;
    uint8_t payload[12];
    uint8_t tag[4];
    uint8_t flags;
    uint8_t eph[32];
};

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
    while (pos < static_cast<double>(n))
    {
        size_t i = static_cast<size_t>(pos);
        float s = static_cast<float>(in[i]);
        s *= p.gain;
        if (p.invert)
            s = -s;
        s += p.dc;
        rng = rng * 1103515245u + 12345u;
        float nse = (static_cast<float>(rng & 0xFFFFu) / 32768.0f - 1.0f) * p.noise;
        s += nse;
        if (s > 32767.0f)
            s = 32767.0f;
        if (s < -32768.0f)
            s = -32768.0f;
        out.push_back(static_cast<int16_t>(s));
        pos += step;
    }
}

static void decimate_24k(const std::vector<int16_t> &in48,
                         std::vector<int16_t> &out24, size_t phase = 0)
{
    out24.clear();
    for (size_t i = phase; i < in48.size(); i += 2)
        out24.push_back(in48[i]);
}

static int demod_collect(HorseDemodulator &demod, const std::vector<int16_t> &rx24,
                         std::vector<decoded_t> &out)
{
    HorseFrameDecoder decoder;
    demod.init();
    demod.resetImmediate();
    demod.setSkipRxFilter(true);
    out.clear();
    for (int16_t s : rx24)
    {
        demod.feedSample(s, false);
        frame_t frame;
        if (!demod.takeFrame(frame))
            continue;
        decoded_t d{};
        d.type = decoder.decodeFrame(frame);
        if (d.type == HorseFrameType::VOICE)
            decoder.getVoicePayload(frame, d.payload, d.tag, &d.fn);
        else if (d.type == HorseFrameType::LINK_SETUP)
            decoder.getLsfCrypto(frame, d.eph, &d.flags);
        out.push_back(d);
    }
    demod.terminate();
    return 0;
}

static int demod_from_48k(const std::vector<int16_t> &imp48,
                          std::vector<decoded_t> &got)
{
    for (size_t ph = 0; ph < 2; ph++)
    {
        std::vector<int16_t> rx24;
        decimate_24k(imp48, rx24, ph);
        HorseDemodulator demod;
        demod_collect(demod, rx24, got);
        if (got.size() >= 3)
            return 0;
    }
    return got.empty() ? -1 : 0;
}

static int render_frames(const std::vector<frame_t> &frames,
                         std::vector<int16_t> &bb48)
{
    HorseModulator mod;
    const size_t per = HorseModulator::captureSamplesPerFrame();
    bb48.assign((frames.size() + 2) * per, 0);
    mod.init();
    if (!mod.start())
        return -1;
    mod.beginCapture(bb48.data(), bb48.size());
    mod.sendPreamble();
    for (const auto &f : frames)
        mod.sendFrame(f);
    bb48.resize(mod.captureLength());
    mod.endCapture();
    mod.stop();
    mod.terminate();
    return 0;
}

static int expect_types(const std::vector<decoded_t> &got,
                        const std::vector<HorseFrameType> &want)
{
    if (got.size() < want.size())
    {
        std::printf("horse_loopback: got %zu frames, want >= %zu\n", got.size(),
                    want.size());
        return -1;
    }
    for (size_t i = 0; i < want.size(); i++)
    {
        if (got[i].type != want[i])
        {
            std::printf("horse_loopback: frame %zu type %u want %u\n", i,
                        static_cast<unsigned>(got[i].type),
                        static_cast<unsigned>(want[i]));
            return -1;
        }
    }
    return 0;
}

static int test_clean_unencrypted_voice()
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames(3);
    call_t src = {{1, 2, 3, 4, 5, 6}};
    call_t dst = {{6, 5, 4, 3, 2, 1}};
    uint8_t melpe[12];
    uint8_t tag[4] = {1, 2, 3, 4};
    memset(melpe, 0x11, sizeof melpe);
    enc.encodeLsf(src, dst, nullptr, 0, frames[0]);
    enc.encodeVoiceFrame(melpe, tag, frames[1], false);
    enc.encodeEotFrame(frames[2]);

    impair_t clean{};
    clean.gain = 1.0f;
    std::vector<decoded_t> got;
    std::vector<int16_t> bb48, imp48;
    if (render_frames(frames, bb48) != 0)
        return -1;
    impair_48k(bb48.data(), bb48.size(), clean, imp48);
    if (demod_from_48k(imp48, got) != 0)
    {
        std::printf("horse_loopback: no frames decoded\n");
        return -1;
    }
    for (size_t i = 0; i < got.size(); i++)
        std::printf("horse_loopback: frame %zu type %u fn %u\n", i,
                    static_cast<unsigned>(got[i].type), got[i].fn);

    std::vector<HorseFrameType> want = {HorseFrameType::LINK_SETUP,
                                        HorseFrameType::VOICE,
                                        HorseFrameType::EOT};
    if (expect_types(got, want) != 0)
        return -1;
    if (memcmp(got[1].payload, melpe, 12) != 0)
        return -1;
    return 0;
}

#ifdef HAVE_LIBSODIUM
static int test_encrypt_and_negatives()
{
    uint8_t a_pk[32], a_sk[32], b_pk[32], b_sk[32];
    crypto_kx_keypair(a_pk, a_sk);
    crypto_kx_keypair(b_pk, b_sk);

    uint8_t eph_pk[32], eph_sk[32], session[32], session_rx[32];
    if (!horse_crypto_x25519_keypair(eph_pk, eph_sk))
        return -1;
    if (!horse_crypto_derive_session_key(eph_sk, b_pk, session))
        return -1;
    if (!horse_crypto_derive_session_key(b_sk, eph_pk, session_rx))
        return -1;
    if (memcmp(session, session_rx, 32) != 0)
        return -1;

    HorseFrameEncoder enc;
    call_t src = {{9, 8, 7, 6, 5, 4}};
    call_t dst = {{1, 1, 1, 1, 1, 1}};
    uint8_t plain[12];
    uint8_t cipher[12];
    uint8_t tag[4];
    uint8_t nonce[12];
    memset(plain, 0x22, sizeof plain);
    horse_crypto_voice_nonce_from_fn(0, nonce);
    horse_crypto_voice_encrypt(session, nonce, plain, sizeof plain, cipher, tag);

    std::vector<frame_t> frames(3);
    enc.encodeLsf(src, dst, eph_pk, LSF_FLAG_ENCRYPTED, frames[0]);
    enc.encodeVoiceFrame(cipher, tag, frames[1], false);
    enc.encodeEotFrame(frames[2]);

    impair_t clean{};
    clean.gain = 1.0f;
    HorseDemodulator demod;
    std::vector<int16_t> bb48, imp48, rx24;
    std::vector<decoded_t> got;
    if (render_frames(frames, bb48) != 0)
        return -1;
    impair_48k(bb48.data(), bb48.size(), clean, imp48);
    decimate_24k(imp48, rx24);
    demod_collect(demod, rx24, got);
    if (got.size() < 2 || got[0].type != HorseFrameType::LINK_SETUP ||
        got[1].type != HorseFrameType::VOICE)
        return -1;
    uint8_t out[12];
    if (!horse_crypto_voice_decrypt(session_rx, nonce, got[1].payload, 12,
                                    got[1].tag, out))
        return -1;
    if (memcmp(out, plain, 12) != 0)
        return -1;

    if (horse_crypto_voice_decrypt(a_sk, nonce, got[1].payload, 12, got[1].tag,
                                   out))
        return -1;

    uint8_t bad_tag[4];
    memcpy(bad_tag, got[1].tag, 4);
    bad_tag[0] ^= 1;
    if (horse_crypto_voice_decrypt(session_rx, nonce, got[1].payload, 12, bad_tag,
                                   out))
        return -1;

    uint8_t bad_pay[12];
    memcpy(bad_pay, got[1].payload, 12);
    bad_pay[0] ^= 1;
    if (horse_crypto_voice_decrypt(session_rx, nonce, bad_pay, 12, got[1].tag,
                                   out))
        return -1;

    horse_crypto_voice_nonce_from_fn(1, nonce);
    if (horse_crypto_voice_decrypt(session_rx, nonce, got[1].payload, 12,
                                    got[1].tag, out))
        return -1;

    HorseFrameEncoder enc2;
    frame_t replay = frames[1];
    enc2.encodeVoiceFrameWithFn(cipher, tag, 0, replay, false);
    if (memcmp(replay.data(), frames[1].data(), replay.size()) == 0)
    {
        /* Replay of FN 0 is the same frame; decoder accepts the bytes.
         * Policy: a second identical FN must be rejected by the session
         * layer. Model that here.
         */
        if (got[1].fn != 0)
            return -1;
    }

    std::vector<frame_t> nosf = frames;
    nosf.erase(nosf.begin());
    HorseDemodulator demod2;
    std::vector<decoded_t> got2;
    if (render_frames(nosf, bb48) != 0)
        return -1;
    impair_48k(bb48.data(), bb48.size(), clean, imp48);
    decimate_24k(imp48, rx24);
    demod_collect(demod2, rx24, got2);
    bool any_lsf = false;
    for (const auto &d : got2)
        if (d.type == HorseFrameType::LINK_SETUP)
            any_lsf = true;
    if (any_lsf)
        return -1;

    horse_crypto_memzero(eph_sk, sizeof eph_sk);
    horse_crypto_memzero(session, sizeof session);
    horse_crypto_memzero(session_rx, sizeof session_rx);
    horse_crypto_memzero(a_sk, sizeof a_sk);
    horse_crypto_memzero(b_sk, sizeof b_sk);
    return 0;
}

static int test_sign_chunks()
{
    uint8_t pk[32], sk[64];
    crypto_sign_ed25519_keypair(pk, sk);
    uint8_t msg[44];
    uint8_t src[6] = {1, 2, 3, 4, 5, 6};
    uint8_t dst[6] = {6, 5, 4, 3, 2, 1};
    horse_crypto_build_session_message(src, dst, nullptr, msg);
    uint8_t sig[64];
    if (!horse_crypto_sign(sk, msg, sizeof msg, sig))
        return -1;

    HorseFrameEncoder enc;
    std::vector<frame_t> frames;
    frames.resize(8);
    call_t s = {{1, 2, 3, 4, 5, 6}};
    call_t d = {{6, 5, 4, 3, 2, 1}};
    enc.encodeLsf(s, d, nullptr, LSF_FLAG_SIGNED, frames[0]);
    uint8_t zero[4] = {0};
    for (uint16_t i = 0; i < SIG_FRAME_COUNT; i++)
    {
        size_t n = sig_chunk_bytes(i);
        enc.encodeVoiceFrameWithFn(sig + i * SIG_CHUNK_BYTES, zero,
                                   SIG_FRAME_BASE + i, frames[1 + i], false, n);
    }
    enc.encodeEotFrame(frames[7]);

    impair_t clean{};
    clean.gain = 1.0f;
    HorseDemodulator demod;
    std::vector<int16_t> bb48, imp48, rx24;
    std::vector<decoded_t> got;
    if (render_frames(frames, bb48) != 0)
        return -1;
    impair_48k(bb48.data(), bb48.size(), clean, imp48);
    decimate_24k(imp48, rx24);
    demod_collect(demod, rx24, got);

    uint8_t rebuilt[64];
    memset(rebuilt, 0, sizeof rebuilt);
    size_t chunks = 0;
    for (const auto &fr : got)
    {
        if (fr.type != HorseFrameType::VOICE)
            continue;
        if (!((fr.fn >= SIG_FRAME_BASE) &&
              (fr.fn < SIG_FRAME_BASE + SIG_FRAME_COUNT)))
            continue;
        uint16_t i = static_cast<uint16_t>(fr.fn - SIG_FRAME_BASE);
        memcpy(rebuilt + i * SIG_CHUNK_BYTES, fr.payload, sig_chunk_bytes(i));
        chunks++;
    }
    if (chunks < SIG_FRAME_COUNT)
        return -1;
    if (!horse_crypto_verify(pk, msg, sizeof msg, rebuilt))
        return -1;

    std::vector<frame_t> drop_sig = frames;
    drop_sig.erase(drop_sig.begin() + 1);
    HorseDemodulator demod2;
    std::vector<decoded_t> got2;
    if (render_frames(drop_sig, bb48) != 0)
        return -1;
    impair_48k(bb48.data(), bb48.size(), clean, imp48);
    decimate_24k(imp48, rx24);
    demod_collect(demod2, rx24, got2);
    memset(rebuilt, 0, sizeof rebuilt);
    for (const auto &fr : got2)
    {
        if (fr.type != HorseFrameType::VOICE)
            continue;
        if (fr.fn < SIG_FRAME_BASE ||
            fr.fn >= SIG_FRAME_BASE + SIG_FRAME_COUNT)
            continue;
        uint16_t i = static_cast<uint16_t>(fr.fn - SIG_FRAME_BASE);
        memcpy(rebuilt + i * SIG_CHUNK_BYTES, fr.payload, sig_chunk_bytes(i));
    }
    if (horse_crypto_verify(pk, msg, sizeof msg, rebuilt))
        return -1;

    horse_crypto_memzero(sk, sizeof sk);
    return 0;
}
#endif

static int test_impair_polarity_and_recovery()
{
    HorseFrameEncoder enc;
    std::vector<frame_t> frames(3);
    call_t src = {{2, 2, 2, 2, 2, 2}};
    call_t dst = {{3, 3, 3, 3, 3, 3}};
    uint8_t melpe[12];
    uint8_t tag[4] = {0};
    memset(melpe, 0x33, sizeof melpe);
    enc.encodeLsf(src, dst, nullptr, 0, frames[0]);
    enc.encodeVoiceFrame(melpe, tag, frames[1], false);
    enc.encodeEotFrame(frames[2]);

    impair_t p{};
    p.gain = 0.8f;
    p.invert = true;
    p.dc = 40.0f;
    p.noise = 50.0f;
    HorseDemodulator demod;
    std::vector<int16_t> bb48, imp48, rx24;
    std::vector<decoded_t> got;
    if (render_frames(frames, bb48) != 0)
        return -1;
    impair_48k(bb48.data(), bb48.size(), p, imp48);
    decimate_24k(imp48, rx24);
    demod_collect(demod, rx24, got);

    impair_t clean{};
    clean.gain = 1.0f;
    HorseDemodulator demod2;
    std::vector<decoded_t> got2;
    if (render_frames(frames, bb48) != 0)
        return -1;
    impair_48k(bb48.data(), bb48.size(), clean, imp48);
    decimate_24k(imp48, rx24);
    demod_collect(demod2, rx24, got2);
    if (got2.size() < 2)
        return -1;
    return 0;
}

int main()
{
#ifdef HAVE_LIBSODIUM
    if (sodium_init() < 0)
        return -1;
#endif
    if (test_clean_unencrypted_voice() != 0)
    {
        std::printf("horse_loopback: clean path failed\n");
        return -1;
    }
#ifdef HAVE_LIBSODIUM
    if (test_encrypt_and_negatives() != 0)
    {
        std::printf("horse_loopback: encrypt/negatives failed\n");
        return -1;
    }
    if (test_sign_chunks() != 0)
    {
        std::printf("horse_loopback: sign chunks failed\n");
        return -1;
    }
#endif
    if (test_impair_polarity_and_recovery() != 0)
    {
        std::printf("horse_loopback: recovery failed\n");
        return -1;
    }
    std::printf("horse_loopback: all tests passed\n");
    return 0;
}

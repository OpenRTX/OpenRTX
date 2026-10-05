/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Three-mode (encrypt / sign / both) modem loopback plus negative cases.
 */

#include "protocols/horse/HorseFrameEncoder.hpp"
#include "protocols/horse/HorseFrameDecoder.hpp"
#include "protocols/horse/HorseModulator.hpp"
#include "protocols/horse/HorseDemodulator.hpp"
#include "protocols/horse/HorseConstants.hpp"
#include "protocols/horse/horse_crypto.h"
#include <cstdio>
#include <cstring>
#include <vector>

#ifdef HAVE_LIBSODIUM
#include <sodium.h>
#endif

using namespace horse;

extern int horse_render_frames(const std::vector<frame_t> &frames,
                               std::vector<int16_t> &bb48, bool preamble);
extern void horse_to_24k(const std::vector<int16_t> &bb48,
                         std::vector<int16_t> &rx24);

int test_three_mode_loopback(void);

#ifdef HAVE_LIBSODIUM
static int demod_raw(const std::vector<int16_t> &rx24,
                     std::vector<frame_t> &out)
{
    HorseDemodulator demod;
    demod.init();
    demod.resetImmediate();
    demod.setSkipDcBlock(true);
    out.clear();
    for (int16_t s : rx24) {
        demod.feedSample(s, false);
        frame_t frame;
        if (demod.takeFrame(frame))
            out.push_back(frame);
    }
    demod.terminate();
    return 0;
}

static int count_good_voice(const std::vector<frame_t> &onair,
                            const uint8_t *rx_sk, const uint8_t *ed_pk,
                            bool want_enc, bool want_sign)
{
    HorseFrameDecoder decoder;
    uint8_t eph[32], flags = 0, version = 0;
    call_t src{}, dst{};
    bool have_lsf = false;
    uint8_t sig[64];
    unsigned sig_n = 0;
    int voice_ok = 0;
    uint16_t last_fn = 0;
    bool have_fn = false;
    uint8_t k_enc[32], k_tag[32];
    bool have_keys = false;
    memset(sig, 0, sizeof sig);

    bool sig_ok = !want_sign;
    for (const auto &fr : onair) {
        HorseFrameType t = decoder.decodeFrame(fr);
        if (!have_lsf && decoder.lsfReady()) {
            decoder.getLsfCallsigns(src, dst);
            if (!decoder.getLsfCrypto(eph, &flags, &version))
                return -1;
            if (!horse_crypto_lsf_version_ok(version))
                return -1;
            if (!horse_crypto_derive_session_keys(rx_sk, eph, src.data(),
                                                  dst.data(), eph, flags,
                                                  version, k_enc, k_tag))
                return -1;
            have_keys = true;
            have_lsf = true;
        }
        if (t != HorseFrameType::VOICE)
            continue;
        uint8_t payload[12], tag[4];
        uint16_t fn = 0;
        decoder.getVoicePayload(fr, payload, tag, &fn);
        if (fn >= SIG_FRAME_BASE && fn < SIG_FRAME_BASE + SIG_FRAME_COUNT) {
            horse_sig_store_chunk(sig, fn - SIG_FRAME_BASE, payload);
            sig_n++;
            continue;
        }
        if (want_sign && !sig_ok) {
            uint8_t frag_sig[64];
            bool have_sig = (sig_n >= SIG_FRAME_COUNT);
            if (!have_sig && decoder.getSigFragments(frag_sig)) {
                memcpy(sig, frag_sig, sizeof sig);
                have_sig = true;
            }
            if (have_sig && have_lsf) {
                uint8_t msg[HORSE_SESSION_MSG_BYTES];
                horse_crypto_build_session_message(src.data(), dst.data(), eph,
                                                   flags, version, msg);
                if (horse_crypto_verify(ed_pk, msg, sizeof msg, sig))
                    sig_ok = true;
            }
            if (!sig_ok)
                continue;
        }
        if (!have_lsf || !have_keys)
            continue;
        uint8_t nonce[12], plain[12];
        horse_crypto_voice_nonce_from_fn(fn, nonce);
        bool ok;
        if (want_enc)
            ok = horse_crypto_voice_decrypt(k_enc, k_tag,
                                            HORSE_VOICE_DIR_FORWARD, fn, nonce,
                                            payload, 12, tag, plain);
        else
            ok = horse_crypto_voice_auth_verify(k_tag, HORSE_VOICE_DIR_FORWARD,
                                                fn, payload, tag);
        if (!ok)
            continue;
        if (!voice_fn_newer(have_fn, last_fn, fn))
            continue;
        have_fn = true;
        last_fn = fn;
        voice_ok++;
    }
    if (!have_lsf)
        return -1;
    return voice_ok;
}

enum class mut_t {
    NONE,
    WRONG_SK,
    FLIP_TAG,
    REPLAY,
    REORDER,
    FLAGS,
    VERSION,
    DROP_LSF,
    SHORT_SIG
};

static int run_mode(bool enc, bool sign, mut_t mut)
{
    uint8_t bob_pk[32], bob_sk[32], eph_pk[32], eph_sk[32];
    uint8_t ed_pk[32], ed_sk[64], k_enc[32], k_tag[32];
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 9, 8, 7, 6, 5, 4 } };
    uint8_t flags = 0;
    if (enc)
        flags |= LSF_FLAG_ENCRYPTED;
    if (sign)
        flags |= LSF_FLAG_SIGNED;
    if (crypto_sign_ed25519_keypair(ed_pk, ed_sk) != 0)
        return -1;
    if (!horse_crypto_x25519_keypair(bob_pk, bob_sk) ||
        !horse_crypto_x25519_keypair(eph_pk, eph_sk))
        return -1;
    uint8_t tx_flags = flags;
    if (mut == mut_t::FLAGS)
        tx_flags ^= LSF_FLAG_ENCRYPTED;
    if (!horse_crypto_derive_session_keys(eph_sk, bob_pk, src.data(),
                                          dst.data(), eph_pk, flags,
                                          HORSE_LSF_VERSION, k_enc, k_tag))
        return -1;

    HorseFrameEncoder encf;
    std::vector<frame_t> frames;
    frame_t lsf3[LSF_OPENING_FRAMES];
    encf.encodeLsf(src, dst, eph_pk, tx_flags, lsf3);
    if (mut == mut_t::VERSION) {
        /* Corrupt coded LSF so CRC fails / version path rejects. */
        for (size_t i = 0; i < LSF_OPENING_FRAMES; i++)
            for (size_t b = 2; b < FRAME_BYTES; b++)
                lsf3[i][b] ^= 0xA5;
    }
    if (mut != mut_t::DROP_LSF) {
        for (size_t i = 0; i < LSF_OPENING_FRAMES; i++)
            frames.push_back(lsf3[i]);
    }

    uint8_t signature[64];
    memset(signature, 0, sizeof signature);
    if (sign) {
        uint8_t msg[HORSE_SESSION_MSG_BYTES];
        horse_crypto_build_session_message(src.data(), dst.data(), eph_pk,
                                           tx_flags, HORSE_LSF_VERSION, msg);
        if (!horse_crypto_sign(ed_sk, msg, sizeof msg, signature))
            return -1;
        encf.setSignatureFragments(signature);
        unsigned nchunk = (mut == mut_t::SHORT_SIG) ? 3u : SIG_FRAME_COUNT;
        for (unsigned i = 0; i < nchunk; i++) {
            uint8_t chunk[12];
            memset(chunk, 0, sizeof chunk);
            memcpy(chunk, signature + i * SIG_CHUNK_BYTES, sig_chunk_bytes(i));
            uint8_t ztag[4] = { 0, 0, 0, 0 };
            frame_t vf{};
            encf.encodeVoiceFrameWithFn(chunk, ztag,
                                        static_cast<uint16_t>(SIG_FRAME_BASE + i),
                                        vf, false);
            frames.push_back(vf);
        }
    } else
        encf.setSignatureFragments(nullptr);

    /* Enough voice frames for a full fragment cycle on late entry. */
    const int nvoice = (mut == mut_t::DROP_LSF) ? 12 : 2;
    std::vector<frame_t> voices;
    for (int i = 0; i < nvoice; i++) {
        uint8_t plain[12], cipher[12], tag[4], nonce[12];
        memset(plain, static_cast<int>(0xA0 + i), sizeof plain);
        horse_crypto_voice_nonce_from_fn(static_cast<uint16_t>(i), nonce);
        if (enc)
            horse_crypto_voice_encrypt(k_enc, k_tag, HORSE_VOICE_DIR_FORWARD,
                                       static_cast<uint16_t>(i), nonce, plain,
                                       12, cipher, tag);
        else {
            memcpy(cipher, plain, 12);
            horse_crypto_voice_auth_tag(k_tag, HORSE_VOICE_DIR_FORWARD,
                                        static_cast<uint16_t>(i), cipher, tag);
        }
        if (mut == mut_t::FLIP_TAG && i == 0)
            tag[0] ^= 1u;
        frame_t vf{};
        encf.encodeVoiceFrameWithFn(cipher, tag, static_cast<uint16_t>(i), vf,
                                    false);
        voices.push_back(vf);
    }
    if (mut == mut_t::REORDER && voices.size() >= 2) {
        frames.push_back(voices[1]);
        frames.push_back(voices[0]);
    } else {
        for (size_t i = 0; i < voices.size(); i++) {
            frames.push_back(voices[i]);
            if (mut == mut_t::REPLAY && i == 0)
                frames.push_back(voices[0]);
        }
    }
    frame_t eot{};
    encf.encodeEotFrame(eot);
    frames.push_back(eot);

    std::vector<int16_t> bb48, rx24;
    if (horse_render_frames(frames, bb48, true) != 0)
        return -1;
    horse_to_24k(bb48, rx24);
    std::vector<frame_t> onair;
    demod_raw(rx24, onair);

    uint8_t rx_sk[32];
    memcpy(rx_sk, bob_sk, 32);
    if (mut == mut_t::WRONG_SK) {
        uint8_t junk_pk[32];
        if (!horse_crypto_x25519_keypair(junk_pk, rx_sk))
            return -1;
    }
    int n = count_good_voice(onair, rx_sk, ed_pk, enc, sign);
    horse_crypto_memzero(eph_sk, sizeof eph_sk);
    horse_crypto_memzero(bob_sk, sizeof bob_sk);
    horse_crypto_memzero(ed_sk, sizeof ed_sk);
    return n;
}

int test_three_mode_loopback(void)
{
    if (sodium_init() < 0)
        return -1;
    struct {
        bool enc;
        bool sign;
        const char *name;
    } modes[3] = { { true, false, "encrypt" },
                   { false, true, "sign" },
                   { true, true, "both" } };
    for (auto &m : modes) {
        int n = run_mode(m.enc, m.sign, mut_t::NONE);
        if (n < 2) {
            std::printf("modes: %s clean recovered %d\n", m.name, n);
            return -1;
        }
        if (run_mode(m.enc, m.sign, mut_t::WRONG_SK) > 0)
            return -1;
        n = run_mode(m.enc, m.sign, mut_t::FLIP_TAG);
        if (n >= 2)
            return -1;
        n = run_mode(m.enc, m.sign, mut_t::REPLAY);
        if (n != 2) {
            std::printf("modes: %s replay %d want 2\n", m.name, n);
            return -1;
        }
        n = run_mode(m.enc, m.sign, mut_t::REORDER);
        if (n != 1) {
            std::printf("modes: %s reorder %d want 1\n", m.name, n);
            return -1;
        }
        if (run_mode(m.enc, m.sign, mut_t::VERSION) > 0)
            return -1;
        /* Late entry: opening LSF omitted; fragments rebuild it. */
        n = run_mode(m.enc, m.sign, mut_t::DROP_LSF);
        if (n < 2) {
            std::printf("modes: %s late-entry recovered %d\n", m.name, n);
            return -1;
        }
        if (m.sign && run_mode(m.enc, m.sign, mut_t::SHORT_SIG) > 0)
            return -1;
        std::printf("modes: %s clean+negatives OK\n", m.name);
    }
    if (run_mode(true, false, mut_t::FLAGS) > 0) {
        std::printf("modes: mismatched flags produced audio\n");
        return -1;
    }
    std::printf("modes: three-mode modem loopback OK\n");
    return 0;
}
#else
int test_three_mode_loopback(void)
{
    std::printf("modes: skipped (no libsodium)\n");
    return 0;
}
#endif

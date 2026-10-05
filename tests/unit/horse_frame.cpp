/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Unit test for Horse protocol frame encode/decode round-trip.
 */

#include "protocols/horse/HorseFrameEncoder.hpp"
#include "protocols/horse/HorseFrameDecoder.hpp"
#include "protocols/horse/HorseConstants.hpp"
#include "protocols/horse/HorseUtils.hpp"
#include "protocols/horse/HorseVoiceCodec.hpp"
#include "protocols/horse/horse_crypto.h"
#include <cstdio>
#include <cstring>
#include <cstdint>

using namespace horse;

static int test_lsf_roundtrip()
{
    HorseFrameEncoder enc;
    HorseFrameDecoder dec;
    call_t src = {{'A', 'B', '1', '2', '3', '4'}};
    call_t dst = {{'C', 'D', '5', '6', '7', '8'}};
    frame_t frames[LSF_OPENING_FRAMES];

    enc.encodeLsf(src, dst, nullptr, 0, frames);
    for (size_t i = 0; i < LSF_OPENING_FRAMES; i++) {
        HorseFrameType type = dec.decodeFrame(frames[i]);
        if (type != HorseFrameType::LINK_SETUP) {
            std::printf("horse_frame_test: LSF type fail at %zu\n", i);
            return -1;
        }
    }
    if (!dec.lsfReady()) {
        std::printf("horse_frame_test: LSF not ready after 3 frames\n");
        return -1;
    }
    call_t outSrc, outDst;
    dec.getLsfCallsigns(outSrc, outDst);
    if (outSrc != src || outDst != dst) {
        std::printf("horse_frame_test: LSF callsign round-trip fail\n");
        return -1;
    }
    uint8_t flags = 0xFF, ver = 0xFF;
    uint8_t eph[32];
    if (!dec.getLsfCrypto(eph, &flags, &ver) || flags != 0 || ver != LSF_PROTOCOL_VERSION) {
        std::printf("horse_frame_test: LSF crypto fields fail\n");
        return -1;
    }
    return 0;
}

static int test_lsf_crc_tamper()
{
    HorseFrameEncoder enc;
    HorseFrameDecoder dec;
    call_t src = {{'A', 'B', '1', '2', '3', '4'}};
    call_t dst = {{'C', 'D', '5', '6', '7', '8'}};
    frame_t frames[LSF_OPENING_FRAMES];
    enc.encodeLsf(src, dst, nullptr, 0, frames);
    for (size_t i = 2; i < FRAME_BYTES; i++)
        frames[1][i] ^= 0xFF;
    for (size_t i = 0; i < LSF_OPENING_FRAMES; i++)
        dec.decodeFrame(frames[i]);
    if (dec.lsfReady()) {
        std::printf("horse_frame_test: tampered LSF accepted\n");
        return -1;
    }
    return 0;
}

static int test_voice_roundtrip()
{
    HorseFrameEncoder enc;
    HorseFrameDecoder dec;
    uint8_t melpe[12] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x11, 0x22, 0x33, 0x44};
    uint8_t tag[4]   = {0xAA, 0xBB, 0xCC, 0xDD};
    frame_t frame;

    enc.encodeVoiceFrame(melpe, tag, frame, false);
    HorseFrameType type = dec.decodeFrame(frame);
    if (type != HorseFrameType::VOICE)
    {
        std::printf("horse_frame_test: voice decode type fail (got %u)\n", static_cast<unsigned>(type));
        return -1;
    }
    uint8_t outMelpe[12], outTag[4];
    uint16_t outFn = 0;
    dec.getVoicePayload(frame, outMelpe, outTag, &outFn);
    if (std::memcmp(outMelpe, melpe, 12) != 0 || std::memcmp(outTag, tag, 4) != 0)
    {
        std::printf("horse_frame_test: voice payload round-trip fail\n");
        return -1;
    }
    if (outFn != 0)
    {
        std::printf("horse_frame_test: voice frame number fail (got %u)\n", outFn);
        return -1;
    }
    return 0;
}

/* Fails on repeat-2: M17 coded bytes must not equal bit-duplication. */
static int test_voice_m17_not_repeat2()
{
    uint8_t info[HORSE_VOICE_INFO_BYTES];
    uint8_t coded[HORSE_VOICE_CODED_BYTES];
    uint8_t out[HORSE_VOICE_INFO_BYTES];
    uint8_t repeat2[HORSE_VOICE_CODED_BYTES];
    std::memset(info, 0xA5, sizeof info);
    info[0] = 0x00;
    info[1] = 0x07;
    voice_encode(info, coded);
    voice_decode(coded, out);
    if (std::memcmp(out, info, sizeof info) != 0) {
        std::printf("horse_frame_test: M17 voice round-trip fail\n");
        return -1;
    }
    std::memset(repeat2, 0, sizeof repeat2);
    for (size_t i = 0; i < HORSE_VOICE_INFO_BYTES * 8 && (2 * i + 1) < 368;
         i++) {
        bool b = (info[i / 8] >> (7 - (i % 8))) & 1u;
        if (b) {
            repeat2[i / 4] |= static_cast<uint8_t>(0xC0u >> (2 * (i % 4)));
        }
    }
    if (std::memcmp(coded, repeat2, sizeof coded) == 0) {
        std::printf("horse_frame_test: voice still looks like repeat-2\n");
        return -1;
    }
    return 0;
}

static int test_eot_detect()
{
    HorseFrameEncoder enc;
    HorseFrameDecoder dec;
    frame_t frame;

    enc.encodeEotFrame(frame);
    HorseFrameType type = dec.decodeFrame(frame);
    if (type != HorseFrameType::EOT)
    {
        std::printf("horse_frame_test: EOT decode type fail (got %u)\n", static_cast<unsigned>(type));
        return -1;
    }
    return 0;
}

static int test_voice_frame_number()
{
    HorseFrameEncoder enc;
    HorseFrameDecoder dec;
    uint8_t melpe[12] = {0};
    uint8_t tag[4]    = {0};
    frame_t frame;

    enc.reset();
    enc.encodeVoiceFrame(melpe, tag, frame, false);
    dec.decodeFrame(frame);
    uint16_t fn0 = 0;
    dec.getVoicePayload(frame, nullptr, nullptr, &fn0);

    enc.encodeVoiceFrame(melpe, tag, frame, false);
    dec.decodeFrame(frame);
    uint16_t fn1 = 0;
    dec.getVoicePayload(frame, nullptr, nullptr, &fn1);

    if (fn0 != 0 || fn1 != 1)
    {
        std::printf("horse_frame_test: voice frame number sequence fail (fn0=%u fn1=%u)\n", fn0, fn1);
        return -1;
    }
    return 0;
}

static int test_lsf_crypto_roundtrip()
{
    HorseFrameEncoder enc;
    HorseFrameDecoder dec;
    call_t src = {{'H', 'O', 'R', 'S', 'E', '1'}};
    call_t dst = {{'H', 'O', 'R', 'S', 'E', '2'}};
    uint8_t eph_pk[32];
    for (size_t i = 0; i < sizeof eph_pk; i++)
        eph_pk[i] = (uint8_t)(i + 1);
    frame_t frames[LSF_OPENING_FRAMES];

    enc.encodeLsf(src, dst, eph_pk, 0x01, frames);
    for (size_t i = 0; i < LSF_OPENING_FRAMES; i++) {
        if (dec.decodeFrame(frames[i]) != HorseFrameType::LINK_SETUP)
            return -1;
    }
    if (!dec.lsfReady())
        return -1;

    uint8_t out_pk[32];
    uint8_t flags = 0;
    uint8_t version = 0;
    if (!dec.getLsfCrypto(out_pk, &flags, &version))
        return -1;
    if (flags != 0x01 || version != LSF_PROTOCOL_VERSION ||
        std::memcmp(out_pk, eph_pk, 32) != 0)
        return -1;
    return 0;
}

static int test_lsf_unknown_version_is_rejected()
{
    if (horse_crypto_lsf_version_ok(99))
        return -1;
    if (!horse_crypto_lsf_version_ok(LSF_PROTOCOL_VERSION))
        return -1;
    return 0;
}

static int test_sig_frames_roundtrip()
{
    HorseFrameEncoder enc;
    HorseFrameDecoder dec;
    uint8_t signature[64];
    uint8_t rebuilt[64];
    uint8_t zeroTag[4] = {0};
    frame_t frame;

    for (size_t i = 0; i < sizeof signature; i++)
        signature[i] = (uint8_t)(i ^ 0x5A);

    memset(rebuilt, 0, sizeof rebuilt);
    for (uint16_t i = 0; i < SIG_FRAME_COUNT; i++)
    {
        const size_t n = sig_chunk_bytes(i);
        enc.encodeVoiceFrameWithFn(signature + (i * SIG_CHUNK_BYTES), zeroTag,
                                   SIG_FRAME_BASE + i, frame, false, n);
        HorseFrameType type = dec.decodeFrame(frame);
        if (type != HorseFrameType::VOICE)
            return -1;

        uint8_t chunk[12];
        uint16_t fn = 0;
        dec.getVoicePayload(frame, chunk, nullptr, &fn);
        if (fn != SIG_FRAME_BASE + i)
            return -1;
        memcpy(rebuilt + (i * SIG_CHUNK_BYTES), chunk, n);
    }

    if (std::memcmp(rebuilt, signature, sizeof signature) != 0)
        return -1;
    return 0;
}

static int test_sig_chunk_last_is_partial()
{
    if (sig_chunk_bytes(0) != 12)
        return -1;
    if (sig_chunk_bytes(4) != 12)
        return -1;
    if (sig_chunk_bytes(5) != 4)
        return -1;
    if (sig_chunk_bytes(6) != 0)
        return -1;
    return 0;
}

static int test_sig_last_chunk_does_not_overflow()
{
    uint8_t buf[SIG_BYTES + 8];
    uint8_t src[SIG_CHUNK_BYTES];

    memset(buf, 0xA5, sizeof buf);
    memset(src, 0x3C, sizeof src);
    if (!horse_sig_store_chunk(buf, 5, src))
        return -1;
    if (memcmp(buf + SIG_BYTES, "\xA5\xA5\xA5\xA5\xA5\xA5\xA5\xA5", 8) != 0)
        return -1;
    if (buf[60] != 0x3C || buf[63] != 0x3C)
        return -1;
    return 0;
}

static int test_sig_last_frame_payload_pad_zero()
{
    HorseFrameEncoder enc;
    HorseFrameDecoder dec;
    uint8_t chunk[12];
    uint8_t last[4] = { 0x11, 0x22, 0x33, 0x44 };
    uint8_t tag[4] = { 0 };
    frame_t frame;
    uint16_t fn = 0;

    enc.encodeVoiceFrameWithFn(last, tag, SIG_FRAME_BASE + 5, frame, false, 4);
    if (dec.decodeFrame(frame) != HorseFrameType::VOICE)
        return -1;
    memset(chunk, 0xFF, sizeof chunk);
    dec.getVoicePayload(frame, chunk, nullptr, &fn);
    if (fn != SIG_FRAME_BASE + 5)
        return -1;
    if (chunk[0] != 0x11 || chunk[1] != 0x22 || chunk[2] != 0x33 ||
        chunk[3] != 0x44)
        return -1;
    for (size_t i = 4; i < 12; i++)
    {
        if (chunk[i] != 0)
            return -1;
    }
    return 0;
}

static int test_sig_incomplete_does_not_fill_64()
{
    uint8_t sig[SIG_BYTES];
    uint8_t src[12];
    unsigned i;

    memset(sig, 0, sizeof sig);
    memset(src, 0x7E, sizeof src);
    for (i = 0; i < 5; i++)
    {
        if (!horse_sig_store_chunk(sig, i, src))
            return -1;
    }
    for (i = 60; i < SIG_BYTES; i++)
    {
        if (sig[i] != 0)
            return -1;
    }
    return 0;
}

static int test_voice_fn_never_enters_sig_range()
{
    HorseFrameEncoder enc;
    HorseFrameDecoder dec;
    uint8_t melpe[12] = { 0 };
    uint8_t tag[4] = { 0 };
    frame_t frame;

    if (!voice_fn_newer(false, 0, 0))
        return -1;
    if (voice_fn_newer(true, 5, 5) || voice_fn_newer(true, 5, 4))
        return -1;
    if (!voice_fn_newer(true, 5, 8))
        return -1;
    if (voice_fn_newer(true, 5, SIG_FRAME_BASE))
        return -1;

    enc.reset();
    for (uint32_t i = 0; i < 4; i++) {
        uint16_t fn = enc.encodeVoiceFrame(melpe, tag, frame, false);
        dec.decodeFrame(frame);
        uint16_t got = 0;
        dec.getVoicePayload(frame, nullptr, nullptr, &got);
        if (fn != i || got != i || !voice_fn_in_session(fn))
            return -1;
    }
    enc.encodeVoiceFrameWithFn(melpe, tag, VOICE_FN_MAX, frame, false);
    dec.decodeFrame(frame);
    uint16_t got = 0;
    dec.getVoicePayload(frame, nullptr, nullptr, &got);
    if (got != VOICE_FN_MAX)
        return -1;
    return 0;
}

static int test_lsf_syncword_symbols()
{
    const std::array<int8_t, 8> lsf = {+3, +3, -1, -1, -1, -1, +3, -3};
    const std::array<int8_t, 8> voice = {+3, -3, -3, -1, -1, +3, -1, -3};
    const std::array<int8_t, 8> eot = {+1, -3, -3, +1, -3, +3, -1, +1};
    if (syncwordSymbols(LSF_SYNC_WORD) != lsf)
        return -1;
    if (syncwordSymbols(VOICE_SYNC_WORD) != voice)
        return -1;
    if (syncwordSymbols(EOT_SYNC_WORD) != eot)
        return -1;
    return 0;
}

int main()
{
    if (test_lsf_roundtrip() != 0) return -1;
    if (test_lsf_crc_tamper() != 0) return -1;
    if (test_lsf_crypto_roundtrip() != 0) return -1;
    if (test_lsf_unknown_version_is_rejected() != 0) return -1;
    if (test_voice_roundtrip() != 0) return -1;
    if (test_voice_m17_not_repeat2() != 0) return -1;
    if (test_eot_detect() != 0) return -1;
    if (test_voice_frame_number() != 0) return -1;
    if (test_sig_frames_roundtrip() != 0) return -1;
    if (test_sig_chunk_last_is_partial() != 0) return -1;
    if (test_sig_last_chunk_does_not_overflow() != 0) return -1;
    if (test_sig_last_frame_payload_pad_zero() != 0) return -1;
    if (test_sig_incomplete_does_not_fill_64() != 0) return -1;
    if (test_voice_fn_never_enters_sig_range() != 0) return -1;
    if (test_lsf_syncword_symbols() != 0) return -1;
    std::printf("horse_frame_test: all tests passed\n");
    return 0;
}

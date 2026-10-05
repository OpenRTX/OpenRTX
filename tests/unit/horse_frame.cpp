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
#include "protocols/horse/ldpc_horse.h"
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
    frame_t frame;

    enc.encodeLsf(src, dst, nullptr, 0, frame);
    HorseFrameType type = dec.decodeFrame(frame);
    if (type != HorseFrameType::LINK_SETUP)
    {
        std::printf("horse_frame_test: LSF decode type fail (got %u)\n", static_cast<unsigned>(type));
        return -1;
    }
    call_t outSrc, outDst;
    dec.getLsfCallsigns(outSrc, outDst);
    if (outSrc != src || outDst != dst)
    {
        std::printf("horse_frame_test: LSF callsign round-trip fail\n");
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
    frame_t frame;

    enc.encodeLsf(src, dst, eph_pk, 0x01, frame);
    HorseFrameType type = dec.decodeFrame(frame);
    if (type != HorseFrameType::LINK_SETUP)
        return -1;

    uint8_t out_pk[32];
    uint8_t flags = 0;
    uint8_t version = 0;
    if (!dec.getLsfCrypto(frame, out_pk, &flags, &version))
        return -1;
    if (flags != 0x01 || version != LSF_PROTOCOL_VERSION ||
        std::memcmp(out_pk, eph_pk, 32) != 0)
        return -1;
    return 0;
}

static int test_lsf_unknown_version_is_rejected()
{
    HorseFrameEncoder enc;
    HorseFrameDecoder dec;
    call_t src = { { 1, 2, 3, 4, 5, 6 } };
    call_t dst = { { 6, 5, 4, 3, 2, 1 } };
    uint8_t eph_pk[32] = { 0 };
    frame_t frame;

    enc.encodeLsf(src, dst, eph_pk, LSF_FLAG_SIGNED, frame);
    frame[2 + LSF_VERSION_OFFSET] = 99;
    uint8_t flags = 0;
    uint8_t version = 0;
    if (!dec.getLsfCrypto(frame, nullptr, &flags, &version))
        return -1;
    if (version == LSF_PROTOCOL_VERSION)
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
    if (test_lsf_crypto_roundtrip() != 0) return -1;
    if (test_lsf_unknown_version_is_rejected() != 0) return -1;
    if (test_voice_roundtrip() != 0) return -1;
    if (test_eot_detect() != 0) return -1;
    if (test_voice_frame_number() != 0) return -1;
    if (test_sig_frames_roundtrip() != 0) return -1;
    if (test_sig_chunk_last_is_partial() != 0) return -1;
    if (test_lsf_syncword_symbols() != 0) return -1;
    std::printf("horse_frame_test: all tests passed\n");
    return 0;
}

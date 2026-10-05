/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/horse/HorseFrameDecoder.hpp"
#include "protocols/horse/HorseVoiceCodec.hpp"
#include "protocols/horse/horse_crypto.h"
#include "core/crc.h"
#include <cstring>

namespace horse
{

static uint8_t hammingDistance(uint8_t x, uint8_t y)
{
    return __builtin_popcount(x ^ y);
}

HorseFrameDecoder::HorseFrameDecoder()
    : lsfNextChunk(0), lsfComplete(false), lastVoiceFrameNum(0)
{
    lsfSrc.fill(0);
    lsfDst.fill(0);
    std::memset(lsfAssembled, 0, sizeof lsfAssembled);
    std::memset(lsfChunkOk, 0, sizeof lsfChunkOk);
    std::memset(fragCopy, 0, sizeof fragCopy);
    std::memset(fragCount, 0, sizeof fragCount);
    std::memset(fragHead, 0, sizeof fragHead);
}

HorseFrameDecoder::~HorseFrameDecoder()
{
}

void HorseFrameDecoder::reset()
{
    lsfSrc.fill(0);
    lsfDst.fill(0);
    lastVoiceFrameNum = 0;
    lsfNextChunk = 0;
    lsfComplete = false;
    std::memset(lsfAssembled, 0, sizeof lsfAssembled);
    std::memset(lsfChunkOk, 0, sizeof lsfChunkOk);
    std::memset(fragCopy, 0, sizeof fragCopy);
    std::memset(fragCount, 0, sizeof fragCount);
    std::memset(fragHead, 0, sizeof fragHead);
}

void HorseFrameDecoder::majoritySlot(size_t slot,
                                     uint8_t out[HORSE_FRAG_BYTES]) const
{
    std::memset(out, 0, HORSE_FRAG_BYTES);
    if (slot >= HORSE_FRAG_CYCLE || fragCount[slot] == 0)
        return;
    const unsigned n = fragCount[slot];
    for (size_t bit = 0; bit < HORSE_FRAG_BYTES * 8; bit++) {
        unsigned ones = 0;
        for (unsigned c = 0; c < n; c++) {
            const uint8_t *copy = fragCopy[slot][c];
            if ((copy[bit / 8] >> (7 - (bit % 8))) & 1u)
                ones++;
        }
        /* Ties count as 0. Need strict majority (> n/2). */
        if (ones * 2 > n)
            out[bit / 8] |= static_cast<uint8_t>(0x80u >> (bit % 8));
    }
}

void HorseFrameDecoder::tryAssembleLsfFromFrags()
{
    if (lsfComplete)
        return;
    for (size_t s = 0; s < HORSE_FRAG_LSF_SLOTS; s++) {
        if (fragCount[s] == 0)
            return;
    }
    uint8_t block[LSF_WITH_CRC_BYTES];
    for (size_t s = 0; s < HORSE_FRAG_LSF_SLOTS; s++)
        majoritySlot(s, block + s * HORSE_FRAG_BYTES);
    uint16_t want = (static_cast<uint16_t>(block[46]) << 8) | block[47];
    if (crc_m17(block, LSF_RAW_BYTES) != want)
        return;
    std::memcpy(lsfAssembled, block, LSF_WITH_CRC_BYTES);
    std::copy(lsfAssembled, lsfAssembled + 6, lsfSrc.begin());
    std::copy(lsfAssembled + 6, lsfAssembled + 12, lsfDst.begin());
    lsfComplete = true;
}

void HorseFrameDecoder::ingestFragment(uint16_t fn,
                                       const uint8_t spare[HORSE_FRAG_BYTES])
{
    const size_t slot = horse_frag_slot(fn);
    if (slot >= HORSE_FRAG_CYCLE)
        return;
    const uint8_t head = fragHead[slot];
    std::memcpy(fragCopy[slot][head], spare, HORSE_FRAG_BYTES);
    fragHead[slot] = static_cast<uint8_t>((head + 1) % HORSE_FRAG_MAJORITY);
    if (fragCount[slot] < HORSE_FRAG_MAJORITY)
        fragCount[slot] = static_cast<uint8_t>(fragCount[slot] + 1);
    if (slot < HORSE_FRAG_LSF_SLOTS)
        tryAssembleLsfFromFrags();
}

HorseFrameType HorseFrameDecoder::decodeFrame(const frame_t &frame)
{
    uint8_t lsfHd = hammingDistance(frame[0], LSF_SYNC_WORD[0])
                  + hammingDistance(frame[1], LSF_SYNC_WORD[1]);
    uint8_t voiceHd = hammingDistance(frame[0], VOICE_SYNC_WORD[0])
                    + hammingDistance(frame[1], VOICE_SYNC_WORD[1]);
    uint8_t eotHd = hammingDistance(frame[0], EOT_SYNC_WORD[0])
                  + hammingDistance(frame[1], EOT_SYNC_WORD[1]);

    if (lsfHd <= HAMMING_SYNC_MAX) {
        if (!lsfComplete && frame.size() >= 2 + HORSE_VOICE_CODED_BYTES
            && lsfNextChunk < LSF_OPENING_FRAMES) {
            uint8_t chunk[HORSE_VOICE_INFO_BYTES];
            voice_decode(frame.data() + 2, chunk);
            const size_t slot = lsfNextChunk;
            std::memcpy(lsfAssembled + slot * LSF_CHUNK_BYTES, chunk,
                        LSF_CHUNK_BYTES);
            lsfChunkOk[slot] = 1;
            lsfNextChunk = static_cast<uint8_t>(slot + 1);
            if (lsfChunkOk[0] && lsfChunkOk[1] && lsfChunkOk[2]) {
                uint16_t want = (static_cast<uint16_t>(lsfAssembled[46]) << 8)
                              | lsfAssembled[47];
                if (crc_m17(lsfAssembled, LSF_RAW_BYTES) == want) {
                    std::copy(lsfAssembled, lsfAssembled + 6, lsfSrc.begin());
                    std::copy(lsfAssembled + 6, lsfAssembled + 12,
                              lsfDst.begin());
                    lsfComplete = true;
                } else {
                    std::memset(lsfChunkOk, 0, sizeof lsfChunkOk);
                    lsfNextChunk = 0;
                }
            }
        }
        return HorseFrameType::LINK_SETUP;
    }
    if (voiceHd <= HAMMING_SYNC_MAX) {
        if (frame.size() >= 2 + HORSE_VOICE_CODED_BYTES) {
            uint8_t info[HORSE_VOICE_INFO_BYTES];
            voice_decode(frame.data() + 2, info);
            lastVoiceFrameNum = (static_cast<uint16_t>(info[0]) << 8) | info[1];
            uint8_t spare[HORSE_FRAG_BYTES];
            voice_extract_spare(frame.data() + 2, spare);
            ingestFragment(lastVoiceFrameNum & 0x7FFF, spare);
        }
        return HorseFrameType::VOICE;
    }
    if (eotHd <= HAMMING_SYNC_MAX)
        return HorseFrameType::EOT;
    return HorseFrameType::UNKNOWN;
}

void HorseFrameDecoder::getLsfCallsigns(call_t &src, call_t &dst)
{
    src = lsfSrc;
    dst = lsfDst;
}

bool HorseFrameDecoder::getLsfCrypto(uint8_t eph_pk[32], uint8_t *flags,
                                     uint8_t *version)
{
    if (!lsfComplete)
        return false;
    if (eph_pk != nullptr)
        std::memcpy(eph_pk, lsfAssembled + LSF_EPH_PK_OFFSET,
                    HORSE_X25519_PUBLICKEY_BYTES);
    if (flags != nullptr)
        *flags = lsfAssembled[LSF_FLAGS_OFFSET];
    if (version != nullptr)
        *version = lsfAssembled[LSF_VERSION_OFFSET];
    return true;
}

void HorseFrameDecoder::getVoicePayload(const frame_t &frame,
                                        uint8_t *melpe96bits,
                                        uint8_t *tag32bits, uint16_t *frameNum)
{
    if (frame.size() < 2 + HORSE_VOICE_CODED_BYTES)
        return;
    uint8_t info[HORSE_VOICE_INFO_BYTES];
    voice_decode(frame.data() + 2, info);
    if (frameNum != nullptr) {
        *frameNum = (static_cast<uint16_t>(info[0]) << 8) | info[1];
        *frameNum &= 0x7FFF;
    }
    if (melpe96bits != nullptr)
        std::memcpy(melpe96bits, info + 2, 12);
    if (tag32bits != nullptr)
        std::memcpy(tag32bits, info + 14, 4);
}

bool HorseFrameDecoder::getSigFragments(uint8_t sig64[SIG_BYTES]) const
{
    if (sig64 == nullptr)
        return false;
    for (size_t s = HORSE_FRAG_LSF_SLOTS; s < HORSE_FRAG_CYCLE; s++) {
        if (fragCount[s] == 0)
            return false;
    }
    std::memset(sig64, 0, SIG_BYTES);
    for (size_t s = HORSE_FRAG_LSF_SLOTS; s < HORSE_FRAG_CYCLE; s++) {
        uint8_t frag[HORSE_FRAG_BYTES];
        majoritySlot(s, frag);
        const size_t off = (s - HORSE_FRAG_LSF_SLOTS) * HORSE_FRAG_BYTES;
        size_t n = SIG_BYTES - off;
        if (n > HORSE_FRAG_BYTES)
            n = HORSE_FRAG_BYTES;
        std::memcpy(sig64 + off, frag, n);
    }
    return true;
}

} // namespace horse

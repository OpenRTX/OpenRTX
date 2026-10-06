/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HORSE_FRAME_DECODER_H
#define HORSE_FRAME_DECODER_H

#include "HorseDatatypes.hpp"
#include "HorseConstants.hpp"
#include "protocols/horse/HorseVoiceCodec.hpp"
#include <cstdint>

#ifndef __cplusplus
#error This header is C++ only!
#endif

namespace horse
{

class HorseFrameDecoder
{
public:
    HorseFrameDecoder();
    ~HorseFrameDecoder();

    void reset();

    HorseFrameType decodeFrame(const frame_t &frame);
    HorseFrameType decodeFrame(const frame_t &frame,
                               const uint16_t soft384[FRAME_BITS]);

    /** True after opening chunks or fragment majority pass crc_m17. */
    bool lsfReady() const
    {
        return lsfComplete;
    }

    void getLsfCallsigns(call_t &src, call_t &dst);

    bool getLsfCrypto(uint8_t eph_pk[32], uint8_t *flags,
                      uint8_t *version = nullptr);

    void getVoicePayload(const frame_t &frame, uint8_t *melpe96bits,
                         uint8_t *tag32bits, uint16_t *frameNum);

    /**
     * \brief Copy majority-combined signature bytes (slots 4..9).
     * @return true if all six slots have at least one copy.
     */
    bool getSigFragments(uint8_t sig64[SIG_BYTES]) const;

private:
    void acceptVoice(const frame_t &frame, const uint16_t *soft384);
    void ingestFragment(uint16_t fn, const uint8_t spare[HORSE_FRAG_BYTES]);
    void ingestFragmentSoft(uint16_t fn,
                            const uint16_t spare96[HORSE_VOICE_SPARE_BITS]);
    void majoritySlot(size_t slot, uint8_t out[HORSE_FRAG_BYTES]) const;
    void tryAssembleLsfFromFrags();

    call_t lsfSrc;
    call_t lsfDst;
    uint8_t lsfAssembled[LSF_CHUNK_BYTES * LSF_OPENING_FRAMES];
    uint8_t lsfChunkOk[LSF_OPENING_FRAMES];
    uint8_t lsfNextChunk;
    bool lsfComplete;
    bool haveVoiceFrame;
    uint16_t lastVoiceFrameNum;

    uint8_t fragCopy[HORSE_FRAG_CYCLE][HORSE_FRAG_MAJORITY][HORSE_FRAG_BYTES];
    uint8_t fragCount[HORSE_FRAG_CYCLE];
    uint8_t fragHead[HORSE_FRAG_CYCLE];
    int32_t fragSoftAcc[HORSE_FRAG_CYCLE][HORSE_VOICE_SPARE_BITS];
    HorseVoiceCodec voiceCodec;
    uint8_t lastInfo[HORSE_VOICE_INFO_BYTES];
    uint16_t lastSpareSoft[HORSE_VOICE_SPARE_BITS];
};

} // namespace horse

#endif // HORSE_FRAME_DECODER_H

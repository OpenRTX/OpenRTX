/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/horse/HorseVoiceCodec.hpp"
#include "protocols/horse/HorseSoft.hpp"
#include "protocols/M17/ConvolutionalEncoder.hpp"
#include "protocols/M17/CodePuncturing.hpp"
#include "protocols/M17/Interleaver.hpp"
#include "protocols/M17/Decorrelator.hpp"
#include <cstring>

namespace horse
{

void HorseVoiceCodec::encode_with_spare(
    const uint8_t info[HORSE_VOICE_INFO_BYTES], const uint8_t *spare12,
    uint8_t coded[HORSE_VOICE_CODED_BYTES])
{
    M17::ConvolutionalEncoder enc;
    enc.reset();
    enc.encode(info, convBuf.data(), HORSE_VOICE_INFO_BYTES);
    convBuf[36] = static_cast<uint8_t>(enc.flush());
    M17::puncture(convBuf, punctBuf, M17::DATA_PUNCTURE);
    std::memcpy(frameBuf.data(), punctBuf.data(), HORSE_VOICE_PUNCT_BYTES);
    const size_t punct_bits = HORSE_VOICE_PUNCT_BYTES * 8;
    for (size_t i = 0; i < HORSE_VOICE_SPARE_BITS; i++) {
        bool b;
        if (spare12 != nullptr)
            b = (spare12[i / 8] >> (7 - (i % 8))) & 1u;
        else {
            size_t src = i % punct_bits;
            b = M17::getBit(punctBuf, src);
        }
        M17::setBit(frameBuf, punct_bits + i, b);
    }
    M17::interleave(frameBuf);
    M17::decorrelate(frameBuf);
    std::memcpy(coded, frameBuf.data(), HORSE_VOICE_CODED_BYTES);
}

void HorseVoiceCodec::encode(const uint8_t info[HORSE_VOICE_INFO_BYTES],
                             uint8_t coded[HORSE_VOICE_CODED_BYTES])
{
    encode_with_spare(info, nullptr, coded);
}

void HorseVoiceCodec::decode(const uint8_t coded[HORSE_VOICE_CODED_BYTES],
                             uint8_t info[HORSE_VOICE_INFO_BYTES])
{
    std::memcpy(frameBuf.data(), coded, HORSE_VOICE_CODED_BYTES);
    M17::decorrelate(frameBuf);
    M17::deinterleave(frameBuf);
    std::memcpy(punctBuf.data(), frameBuf.data(), HORSE_VOICE_PUNCT_BYTES);
    vit.decodePunctured(punctBuf, infoBuf, M17::DATA_PUNCTURE);
    std::memcpy(info, infoBuf.data(), HORSE_VOICE_INFO_BYTES);
}

void HorseVoiceCodec::decode_soft(
    const uint16_t payload368[HORSE_VOICE_CODED_BITS],
    uint8_t info[HORSE_VOICE_INFO_BYTES])
{
    std::memcpy(softBuf.data(), payload368, sizeof(softBuf));
    soft_decorrelate(softBuf.data());
    soft_deinterleave(softBuf.data(), deintTmp.data());
    for (size_t i = 0; i < HORSE_VOICE_PUNCT_BYTES * 8; i++)
        punctSoft[i] = softBuf[i];
    vitSoft.decodePunctured(punctSoft, infoBuf, M17::DATA_PUNCTURE);
    std::memcpy(info, infoBuf.data(), HORSE_VOICE_INFO_BYTES);
}

void HorseVoiceCodec::extract_spare(const uint8_t coded[HORSE_VOICE_CODED_BYTES],
                                    uint8_t spare12[HORSE_VOICE_SPARE_BYTES])
{
    std::memcpy(frameBuf.data(), coded, HORSE_VOICE_CODED_BYTES);
    M17::decorrelate(frameBuf);
    M17::deinterleave(frameBuf);
    std::memset(spare12, 0, HORSE_VOICE_SPARE_BYTES);
    const size_t punct_bits = HORSE_VOICE_PUNCT_BYTES * 8;
    for (size_t i = 0; i < HORSE_VOICE_SPARE_BITS; i++) {
        if (M17::getBit(frameBuf, punct_bits + i))
            spare12[i / 8] |= static_cast<uint8_t>(0x80u >> (i % 8));
    }
}

void HorseVoiceCodec::extract_spare_soft(
    const uint16_t payload368[HORSE_VOICE_CODED_BITS],
    uint16_t spare96[HORSE_VOICE_SPARE_BITS])
{
    std::memcpy(softBuf.data(), payload368, sizeof(softBuf));
    soft_decorrelate(softBuf.data());
    soft_deinterleave(softBuf.data(), deintTmp.data());
    const size_t punct_bits = HORSE_VOICE_PUNCT_BYTES * 8;
    for (size_t i = 0; i < HORSE_VOICE_SPARE_BITS; i++)
        spare96[i] = softBuf[punct_bits + i];
}

static HorseVoiceCodec &voice_codec_instance()
{
    static HorseVoiceCodec inst;
    return inst;
}

void voice_encode(const uint8_t info[HORSE_VOICE_INFO_BYTES],
                  uint8_t coded[HORSE_VOICE_CODED_BYTES])
{
    voice_codec_instance().encode(info, coded);
}

void voice_decode(const uint8_t coded[HORSE_VOICE_CODED_BYTES],
                  uint8_t info[HORSE_VOICE_INFO_BYTES])
{
    voice_codec_instance().decode(coded, info);
}

void voice_encode_with_spare(const uint8_t info[HORSE_VOICE_INFO_BYTES],
                             const uint8_t *spare12,
                             uint8_t coded[HORSE_VOICE_CODED_BYTES])
{
    voice_codec_instance().encode_with_spare(info, spare12, coded);
}

void voice_extract_spare(const uint8_t coded[HORSE_VOICE_CODED_BYTES],
                         uint8_t spare12[HORSE_VOICE_SPARE_BYTES])
{
    voice_codec_instance().extract_spare(coded, spare12);
}

} /* namespace horse */

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/horse/HorseVoiceCodec.hpp"
#include "protocols/M17/ConvolutionalEncoder.hpp"
#include "protocols/M17/CodePuncturing.hpp"
#include "protocols/M17/Interleaver.hpp"
#include "protocols/M17/Decorrelator.hpp"
#include "protocols/M17/Viterbi.hpp"
#include <array>
#include <cstring>

namespace horse
{

void voice_encode_with_spare(const uint8_t info[HORSE_VOICE_INFO_BYTES],
                             const uint8_t *spare12,
                             uint8_t coded[HORSE_VOICE_CODED_BYTES])
{
    M17::ConvolutionalEncoder enc;
    std::array<uint8_t, 37> encoded{};
    enc.reset();
    enc.encode(info, encoded.data(), HORSE_VOICE_INFO_BYTES);
    encoded[36] = static_cast<uint8_t>(enc.flush());
    std::array<uint8_t, HORSE_VOICE_PUNCT_BYTES> punct{};
    M17::puncture(encoded, punct, M17::DATA_PUNCTURE);
    std::array<uint8_t, HORSE_VOICE_CODED_BYTES> frame{};
    std::memcpy(frame.data(), punct.data(), HORSE_VOICE_PUNCT_BYTES);
    const size_t punct_bits = HORSE_VOICE_PUNCT_BYTES * 8;
    for (size_t i = 0; i < HORSE_VOICE_SPARE_BITS; i++) {
        bool b;
        if (spare12 != nullptr)
            b = (spare12[i / 8] >> (7 - (i % 8))) & 1u;
        else {
            size_t src = i % punct_bits;
            b = M17::getBit(punct, src);
        }
        M17::setBit(frame, punct_bits + i, b);
    }
    M17::interleave(frame);
    M17::decorrelate(frame);
    std::memcpy(coded, frame.data(), HORSE_VOICE_CODED_BYTES);
}

void voice_encode(const uint8_t info[HORSE_VOICE_INFO_BYTES],
                  uint8_t coded[HORSE_VOICE_CODED_BYTES])
{
    voice_encode_with_spare(info, nullptr, coded);
}

void voice_decode(const uint8_t coded[HORSE_VOICE_CODED_BYTES],
                  uint8_t info[HORSE_VOICE_INFO_BYTES])
{
    std::array<uint8_t, HORSE_VOICE_CODED_BYTES> frame{};
    std::memcpy(frame.data(), coded, HORSE_VOICE_CODED_BYTES);
    M17::decorrelate(frame);
    M17::deinterleave(frame);
    std::array<uint8_t, HORSE_VOICE_PUNCT_BYTES> punct{};
    std::memcpy(punct.data(), frame.data(), HORSE_VOICE_PUNCT_BYTES);
    std::array<uint8_t, HORSE_VOICE_INFO_BYTES> out{};
    M17::HardViterbi vit;
    vit.decodePunctured(punct, out, M17::DATA_PUNCTURE);
    std::memcpy(info, out.data(), HORSE_VOICE_INFO_BYTES);
}

void voice_extract_spare(const uint8_t coded[HORSE_VOICE_CODED_BYTES],
                         uint8_t spare12[HORSE_VOICE_SPARE_BYTES])
{
    std::array<uint8_t, HORSE_VOICE_CODED_BYTES> frame{};
    std::memcpy(frame.data(), coded, HORSE_VOICE_CODED_BYTES);
    M17::decorrelate(frame);
    M17::deinterleave(frame);
    std::memset(spare12, 0, HORSE_VOICE_SPARE_BYTES);
    const size_t punct_bits = HORSE_VOICE_PUNCT_BYTES * 8;
    for (size_t i = 0; i < HORSE_VOICE_SPARE_BITS; i++) {
        if (M17::getBit(frame, punct_bits + i))
            spare12[i / 8] |= static_cast<uint8_t>(0x80u >> (i % 8));
    }
}

} /* namespace horse */

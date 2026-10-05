/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HORSE_VOICE_CODEC_H
#define HORSE_VOICE_CODEC_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include "protocols/horse/HorseConstants.hpp"
#include <cstdint>
#include <cstddef>

namespace horse
{

/**
 * \brief Encode 18 info bytes to 46 coded bytes (M17 conv + puncture +
 *        interleave + decorrelate; spare filled by punctured repeat).
 */
void voice_encode(const uint8_t info[HORSE_VOICE_INFO_BYTES],
                  uint8_t coded[HORSE_VOICE_CODED_BYTES]);

/**
 * \brief Hard-decision decode 46 coded bytes to 18 info bytes.
 */
void voice_decode(const uint8_t coded[HORSE_VOICE_CODED_BYTES],
                  uint8_t info[HORSE_VOICE_INFO_BYTES]);

/**
 * \brief Optional spare overlay: write 12 bytes into the pre-interleave
 *        spare region (used by fragment packing). If null, encode uses
 *        punctured-stream repetition.
 */
void voice_encode_with_spare(const uint8_t info[HORSE_VOICE_INFO_BYTES],
                             const uint8_t *spare12,
                             uint8_t coded[HORSE_VOICE_CODED_BYTES]);

/**
 * \brief Extract the 12-byte spare after decorrelate + deinterleave.
 */
void voice_extract_spare(const uint8_t coded[HORSE_VOICE_CODED_BYTES],
                         uint8_t spare12[HORSE_VOICE_SPARE_BYTES]);

} /* namespace horse */

#endif

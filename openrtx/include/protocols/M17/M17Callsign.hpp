/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_CALLSIGN_H
#define M17_CALLSIGN_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <string>
#include "M17Datatypes.hpp"

namespace M17
{

/**
 * Encode a callsign in base-40 format, starting with the right-most character.
 * The final value is written out in "big-endian" form, with the most-significant
 * value first, leading to 0-padding of callsigns shorter than nine characters.
 *
 * \param callsign the callsign to encode.
 * \param encodedCall call_t data structure where to put the encoded data.
 * \param strict a flag (disabled by default) which indicates whether invalid
 * characters are allowed and assigned a value of 0 or not allowed, making the
 * function return an error.
 * @return true if the callsign was successfully encoded, false on error.
 */
bool encode_callsign(const std::string& callsign, call_t& encodedCall,
                     bool strict = false);

/**
 * Decode a base-40 encoded callsign to its text representation. This decodes
 * a 6-byte big-endian value into a string of up to 9 characters.
 *
 * \param encodedCall base-40 encoded callsign.
 * \return a string containing the decoded text.
 */
std::string decode_callsign(const call_t& encodedCall);

}      // namespace M17

#endif // M17_CALLSIGN_H

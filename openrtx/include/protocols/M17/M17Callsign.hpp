/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

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

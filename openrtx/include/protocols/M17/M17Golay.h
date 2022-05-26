/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Wojciech Kaczmarski SP5WWP               *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
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

#ifndef M17_GOLAY_H
#define M17_GOLAY_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>

namespace M17
{

namespace Golay24
{

/**
 * Function computing the Golay(24,12) checksum of a given 12-bit data block.
 *
 * @param value: input data.
 * @return Golay(24,12) checksum.
 */
uint16_t calcChecksum(const uint16_t& value);

/**
 * Detect and correct errors in a Golay(24,12) codeword.
 *
 * @param codeword: input codeword.
 * @return bitmask corresponding to detected bit errors in the codeword, or
 * 0xFFFFFFFF if bit errors are unrecoverable.
 */
uint32_t detectErrors(const uint32_t& codeword);

}   // namespace Golay24


/**
 * Compute the Golay(24,12) codeword of a given block of data using the
 * generator polynomial 0xC75.
 * Result is composed as follows:
 *
 * +--------+----------+--------+
 * | parity | checksum |  data  |
 * +--------+----------+--------+
 * | 1 bit  |   11 bit | 12 bit |
 * +--------+----------+--------+
 *
 * \param data: input data, upper four bits are discarded.
 * \return resulting 24 bit codeword.
 */
static inline uint32_t golay24_encode(const uint16_t& data)
{
    return (data << 12) | Golay24::calcChecksum(data);
}


/**
 * Decode a Golay(24,12) codeword, correcting eventual bit errors. In case the
 * bit errors are not correctable, the function returns 0xFFFF, a value outside
 * the range allowed for the 12-bit input data required by Golay coding.
 *
 * \param codeword: input Golay(24,12) codeword.
 * \return original data block or 0xFFFF in case of unrecoverable errors.
 */
static inline uint16_t golay24_decode(const uint32_t& codeword)
{
    uint32_t errors = Golay24::detectErrors(codeword);
    if(errors == 0xFFFFFFFF) return 0xFFFF;
    return ((codeword ^ errors) >> 12) & 0x0FFF;
}

}      // namespace M17

#endif // M17_GOLAY_H

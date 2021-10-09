/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   Adapted from original code written by Rob Riggs, Mobilinkd LLC        *
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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
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

/**
 * Compute the Golay(24,12) codeword of a given block of data using the M17
 * Golay polynomial.
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
static uint32_t golay_encode24(const uint16_t& data)
{
    // Compute [23,12] Golay codeword
    uint32_t codeword = data & 0x0FFFF;
    for(size_t i = 0; i < 12; i++)
    {
        if(codeword & 0x01) codeword ^= 0x0C75;
        codeword >>= 1;
    }

    // Append data to codeword, result is checkbits(11) | data(12)
    codeword |= (data << 11);

    // Compute parity and append it to codeword
    return (codeword << 1) | (__builtin_popcount(codeword) & 0x01);
}

#endif /* M17_GOLAY_H */

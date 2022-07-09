/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef M17_DECORRELATOR_H
#define M17_DECORRELATOR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <experimental/array>
#include <string>
#include <array>

namespace M17
{

/**
 * Decorrelator sequence for data randomisation.
 */
static constexpr std::array< uint8_t, 46 > sequence =
{
    0xd6, 0xb5, 0xe2, 0x30, 0x82, 0xFF, 0x84, 0x62,
    0xba, 0x4e, 0x96, 0x90, 0xd8, 0x98, 0xdd, 0x5d,
    0x0c, 0xc8, 0x52, 0x43, 0x91, 0x1d, 0xf8, 0x6e,
    0x68, 0x2F, 0x35, 0xda, 0x14, 0xea, 0xcd, 0x76,
    0x19, 0x8d, 0xd5, 0x80, 0xd1, 0x33, 0x87, 0x13,
    0x57, 0x18, 0x2d, 0x29, 0x78, 0xc3
};


/**
 * Apply M17 decorrelation scheme to a byte array.
 *
 * \param data: byte array to be decorrelated..
 */
template <size_t N >
inline void decorrelate(std::array< uint8_t, N >& data)
{
    for (size_t i = 0; i < N; i++)
    {
        data[i] ^= sequence[i];
    }
}

}      // namespace M17

#endif // M17_DECORRELATOR_H

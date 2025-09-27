/***************************************************************************
 *   Copyright (C)        2025 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef PROTOCOLS_FM_DTMF_HPP
#define PROTOCOLS_FM_DTMF_HPP

#include <cstdint>
#include <array>

struct DtmfKeyEntry {
    const char symbol;       // DTMF symbol (0-9, A-D, *, #)
    const uint16_t freqLow;  // Low frequency component in Hz
    const uint16_t freqHigh; // High frequency component in Hz
};

namespace DTMF
{
/**
 * This function looks up the DTMF frequencies for a given symbol.
 *
 * @param symbol: The DTMF symbol to look up (0-9, A-D, *, #)
 * @return A DtmfKeyEntry struct containing the symbol and its frequencies.
 *         If the symbol is not found, returns an entry with symbol '\0' and frequencies 0.
 */
DtmfKeyEntry lookupDtmfFreq(char symbol);
}

#endif // PROTOCOLS_FM_DTMF_HPP

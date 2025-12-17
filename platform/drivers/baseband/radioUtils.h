/***************************************************************************
 *   Copyright (C) 2021 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef RADIO_UTILS_H
#define RADIO_UTILS_H

#include <stdint.h>
#include "core/datatypes.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DtmfKeyEntry {
    const char symbol;       // DTMF symbol (0-9, A-D, *, #)
    const uint16_t freqLow;  // Low frequency component in Hz
    const uint16_t freqHigh; // High frequency component in Hz
};

static constexpr DtmfKeyEntry dtmfKeyMap[] = {
    { '1', 697, 1209 }, { '2', 697, 1336 }, { '3', 697, 1477 },
    { 'A', 697, 1633 }, { '4', 770, 1209 }, { '5', 770, 1336 },
    { '6', 770, 1477 }, { 'B', 770, 1633 }, { '7', 852, 1209 },
    { '8', 852, 1336 }, { '9', 852, 1477 }, { 'C', 852, 1633 },
    { '*', 941, 1209 }, { '0', 941, 1336 }, { '#', 941, 1477 },
    { 'D', 941, 1633 }
};

static const freq_t BAND_VHF_LO = 136000000;
static const freq_t BAND_VHF_HI = 174000000;
static const freq_t BAND_UHF_LO = 400000000;
static const freq_t BAND_UHF_HI = 470000000;

/**
 * Enumeration type for bandwidth identification.
 */
enum Band
{
    BND_NONE = -1,
    BND_VHF  = 0,
    BND_UHF  = 1
};

/**
 * \internal
 * Function to identify the current band (VHF or UHF), given an input frequency.
 *
 * @param freq frequency in Hz.
 * @return a value from @enum Band identifying the band to which the frequency
 * belong.
 */
static inline enum Band getBandFromFrequency(const freq_t freq)
{
    if((freq >= BAND_VHF_LO) && (freq <= BAND_VHF_HI)) return BND_VHF;
    if((freq >= BAND_UHF_LO) && (freq <= BAND_UHF_HI)) return BND_UHF;
    return BND_NONE;
}

/**
 * This function looks up the DTMF frequencies for a given symbol.
 *
 * @param symbol: The DTMF symbol to look up (0-9, A-D, *, #)
 * @return A DtmfKeyEntry struct containing the symbol and its frequencies.
 *         If the symbol is not found, returns an entry with symbol '\0' and frequencies 0.
 */
static inline DtmfKeyEntry lookupDtmfFreq(char symbol)
{
    for (const auto &entry : dtmfKeyMap)
        if (entry.symbol == symbol)
            return entry;
    return { '\0', 0, 0 }; // Not found
}

#ifdef __cplusplus
}
#endif

#endif /* RADIO_UTILS_H */

/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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
#include <datatypes.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* RADIO_UTILS_H */

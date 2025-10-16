/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RADIO_UTILS_H
#define RADIO_UTILS_H

#include <stdint.h>
#include "core/datatypes.h"

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

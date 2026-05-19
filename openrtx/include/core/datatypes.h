/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DATATYPES_H
#define DATATYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief CTCSS and DCS type definition.
 *
 * Continuous Tone Controlled Squelch System (CTCSS)
 * sub-audible tone frequency are expressed in \em tenth of Hz.
 * For example, the subaudible tone of 88.5 Hz is represented within Hamlib by
 * 885.
 *
 * Digitally-Coded Squelch codes are simple direct integers.
 */
typedef unsigned int tone_t;

/**
 * \brief Frequency type.
 *
 * Frequency type unit in Hz, able to hold SHF frequencies.
 */
typedef uint32_t freq_t;

/**
 * \brief RSSI type.
 *
 * Data type for RSSI, in dBm.
 */
typedef int32_t rssi_t;

#ifdef __cplusplus
}
#endif

#endif /* DATATYPES_H */

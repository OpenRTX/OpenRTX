/***************************************************************************
 *   Copyright (C) 2020 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef RADIO_H
#define RADIO_H

#include <stdbool.h>
#include <stdint.h>
#include "rtx/rtx.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This file provides a common interface for the platform-dependent low-level
 * rtx driver. Top level application code normally does not have to call directly
 * the API functions provided here, since all the transceiver management,
 * comprised the handling of digital protocols, is done by the 'rtx' module.
 *
 * The radio functionalities are controlled by means of an rtxStatus_t data
 * structure containing all the parameters required to define a given operating
 * configuration for the RF stage such as TX and RX frequencies, modulation mode,
 * squelch level, and so on. The data structure is internally accessed by each of
 * the API functions and is guaranteed to be used in read-only mode.
 */

/**
 * Initialise low-level radio transceiver.
 *
 * @param rtxState: pointer to an rtxStatus_t structure used to describe the
 * operating configuration of the radio module.
 */
void radio_init(const rtxStatus_t *rtxState);

/**
 * Shut down low-level radio transceiver.
 */
void radio_terminate();

/**
 * Set current operating mode.
 *
 * @param mode: new operating mode.
 */
void radio_setOpmode(const enum opmode mode);

/**
 * Check if digital squelch is opened, that is if a CTC/DCS code is being
 * detected.
 *
 * @return true if RX digital squelch is enabled and if the configured CTC/DCS
 * code is present alongside the carrier.
 */
bool radio_checkRxDigitalSquelch();

/**
 * Enable AF output towards the speakers.
 */
void radio_enableAfOutput();

/**
 * Disable AF output towards the speakers.
 */
void radio_disableAfOutput();

/**
 * Enable the RX stage.
 */
void radio_enableRx();

/**
 * Enable the TX stage.
 */
void radio_enableTx();

/**
 * Disable both the RX and TX stages, as long as transmission of CTC/DCS code
 * and digital squelch.
 */
void radio_disableRtx();

/**
 * Update configuration of the radio module to match the one currently described
 * by the rtxStatus_t configuration data structure.
 * This function has to be called whenever the configuration data structure has
 * been updated, to ensure all the operating parameters of the radio driver are
 * correctly configured.
 */
void radio_updateConfiguration();

/**
 * Get the current RSSI level in dBm.
 *
 * @return RSSI level in dBm.
 */
rssi_t radio_getRssi();

/**
 * Get the current operating status of the radio module.
 *
 * @return current operating status.
 */
enum opstatus radio_getStatus();

/* -------------------------------------------------------------------------- */
/*                        FM Frequency Scan Additions                         */
/* -------------------------------------------------------------------------- */

/**
 * Perform a frequency scan step and return the measured RSSI.
 * Used internally by FM scan feature.
 *
 * @param freq Frequency to tune to in Hz.
 * @param dwell_ms Time to wait before sampling RSSI (in milliseconds).
 * @return RSSI value in dBm.
 */
rssi_t radio_scanStep(freq_t freq, uint16_t dwell_ms);

/**
 * Perform a full FM frequency scan between start and stop frequencies.
 *
 * @param start Start frequency in Hz.
 * @param stop Stop frequency in Hz.
 * @param step Frequency step in Hz (e.g. 12500 or 25000).
 * @param threshold Minimum RSSI threshold to detect a signal.
 * @return Frequency where signal was found, or 0 if none.
 */
freq_t radio_scan(freq_t start, freq_t stop, freq_t step, rssi_t threshold);

#ifdef __cplusplus
}
#endif

#endif /* RADIO_H */

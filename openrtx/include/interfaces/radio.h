/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
 * the API functions provided here, since all the transceiver managment, comprised
 * the handling of digital protocols is done by the 'rtx' module.
 *
 * The radio functionalities are controlled by means of an rtxStatus_t data
 * structure containing all the parameters required to define a given operating
 * configuration for the RF stage such as TX and RX frequencies, ...
 * The data structure is internally accessed by each of the API functions and is
 * guaranteed that the access is performed in read only mode.
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

#ifdef __cplusplus
}
#endif

#endif /* RADIO_H */

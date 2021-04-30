/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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
#include <rtx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This file provides a common interface for the platform-dependent low-level
 * rtx driver. Top level application code normally does not have to call directly
 * the API functions provided here, since all the transceiver managment, comprised
 * the handling of digital protocols is done by the 'rtx' module.
 */

/**
 * Initialise low-level radio transceiver.
 */
void radio_init(const rtxStatus_t *rtxState);

/**
 * Shut down low-level radio transceiver.
 */
void radio_terminate();

/**
 *
 */
void radio_setOpmode(const enum opmode mode);

/**
 *
 */
bool radio_checkRxDigitalSquelch();

/**
 *
 */
void radio_enableRx();

/**
 *
 */
void radio_enableTx();

/**
 *
 */
void radio_disableRtx();

/**
 *
 */
void radio_updateConfiguration();

/**
 *
 */
float radio_getRssi();

/**
 *
 */
enum opstatus radio_getStatus();

#ifdef __cplusplus
}
#endif

#endif /* RADIO_H */

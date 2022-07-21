/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef NVMEM_H
#define NVMEM_H

#include <stdint.h>

#include "core/cps.h"
#include "core/settings.h"
#include "interfaces/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Interface for nonvolatile memory management, usually an external SPI flash
 * memory, containing calibration, contact data and so on.
 */

/**
 * Initialise NVM driver.
 */
void nvm_init();

/**
 * Terminate NVM driver.
 */
void nvm_terminate();

/**
 * Load calibration data from nonvolatile memory.
 *
 * @param buf: destination buffer for calibration data.
 */
void nvm_readCalibData(void *buf);

/**
 * Load all or some hardware information parameters from nonvolatile memory.
 *
 * @param info: destination data structure for hardware information data.
 */
void nvm_loadHwInfo(hwInfo_t *info);

/**
 * Read from storage the channel data corresponding to the VFO channel A.
 *
 * @param channel: pointer to the channel_t data structure to be populated.
 * @return 0 on success, -1 on failure
 */
int nvm_readVFOChannelData(channel_t *channel);

/**
 * Read OpenRTX settings from storage.
 *
 * @param settings: pointer to the settings_t data structure to be populated.
 * @return 0 on success, -1 on failure
 */
int nvm_readSettings(settings_t *settings);

/**
 * Write OpenRTX settings to storage.
 *
 * @param settings: pointer to the settings_t data structure to be written.
 * @return 0 on success, -1 on failure
 */
int nvm_writeSettings(const settings_t *settings);

/**
 * Write OpenRTX settings and VFO channel configuration to storage.
 *
 * @param settings: pointer to the settings_t data structure to be written.
 * @param vfo: pointer to the VFO data structure to be written.
 * @return 0 on success, -1 on failure
 */
int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo);

#ifdef __cplusplus
}
#endif

#endif /* NVMEM_H */

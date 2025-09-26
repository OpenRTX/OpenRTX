/***************************************************************************
 *   Copyright (C) 2023 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef FLASH_ZEPHYR_H
#define FLASH_ZEPHYR_H

#include "interfaces/nvmem.h"

/**
 * Wrapper interface for the Zephyr RTOS flash memory device driver.
 */

/**
 *  Device driver API for Zephyr RTOS flash memory.
 */
extern const struct nvmOps zephyr_flash_ops;

/**
 * Instantiate a nonvolatile memory device based on Zephyr RTOS flash device
 * driver.
 *
 * @param name: device name.
 * @param alias: devicetree alias of the flash device.
 * @param sz: memory size, in bytes.
 */
#define ZEPHYR_FLASH_DEVICE_DEFINE(name, alias, sz) \
static struct nvmInfo nvm_devInfo_##name;           \
static const struct nvmDevice name =                \
{                                                   \
    .priv   = DEVICE_DT_GET(DT_ALIAS(alias)),       \
    .ops    = &zephyr_flash_ops,                    \
    .info   = &nvm_devInfo_##name,                  \
    .size   = sz                                    \
};


/**
 * Initialize a Zephyr RTOS flash device driver instance.
 *
 * @param dev: device handle.
 * @return zero on success, a negative error code otherwise.
 */
int zephirFlash_init(const struct nvmDevice* dev);

#endif /* FLASH_ZEPHYR_H */

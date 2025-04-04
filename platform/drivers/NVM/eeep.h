/***************************************************************************
 *   Copyright (C) 2024 - 2025 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                Morgan Diepart ON4MOD                    *
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

#ifndef EEEP_H
#define EEEP_H

#include <interfaces/nvmem.h>

/**
 * Driver for Emulated EEPROM, providing a means to store recurrent data without
 * stressing too much the same sector of a NOR or NAND flash memory.
 */

/**
 * Device driver and information block for EEEPROM memory.
 */
extern const struct nvmOps  eeep_ops;
extern const struct nvmInfo eeep_info;

/**
 * Driver private data.
 */
struct eeepData
{
    const struct nvmDevice    *nvm;         ///< Underlying NVM device
    const struct nvmPartition *part;        ///< Memory partition used for EEPROM emulation
    uint32_t                  readAddr;     ///< Physical start address for EEEPROM reads
    uint32_t                  writeAddr;    ///< Physical start address for EEEPROM writes
};

/**
 * Instantiate a EEEPROM NVM device.
 *
 * @param name: device name.
 * @param path: full path of the file used for data storage.
 * @param size: size of the storage file, in bytes.
 */
#define EEEP_DEVICE_DEFINE(name)        \
static struct eeepData eeepData_##name; \
struct nvmDevice name =                 \
{                                       \
    .priv = &eeepData_##name,           \
    .ops  = &eeep_ops,                  \
    .info = &eeep_info,                 \
    .size = 65536                       \
};

/**
 * Initialize an EEEP driver instance.
 *
 * @param dev: pointer to device descriptor.
 * @param nvm: index of the underlying NVM device used for data storage.
 * @param part: NVM partition used for data storage.
 * @return zero on success, a negative error code otherwise.
 */
int eeep_init(const struct nvmDevice *dev, const uint32_t nvm,
              const uint32_t part);

/**
 * Shut down an EEEP driver instance.
 *
 * @param dev: pointer to device descriptor.
 * @return zero on success, a negative error code otherwise.
 */
int eeep_terminate(const struct nvmDevice *dev);

#endif /* EEEP_H */

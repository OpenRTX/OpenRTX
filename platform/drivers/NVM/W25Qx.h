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

#ifndef W25Qx_H
#define W25Qx_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <peripherals/spi.h>
#include <peripherals/gpio.h>
#include <interfaces/nvmem.h>

/**
 * Driver for Winbond W25Qx family of SPI flash devices, used as external non
 * volatile memory on various radios to store both calibration and contact data.
 */

/**
 * Configuration data structure for W25Qx memory device.
 */
struct W25QxCfg
{
    const struct spiDevice *spi;
    const struct gpioPin    cs;
};

/**
 * Driver data structure for W25Qx security registers.
 */
struct W25QxSecRegDevice
{
    const void           *priv;        ///< Device driver private data
    const struct nvmOps  *ops;         ///< Device operations
    const struct nvmInfo *info;        ///< Device info
    const size_t          size;        ///< Device size
    const uint32_t        baseAddr;    ///< Register base address
};

/**
 * Device driver and information block for W25Qx main memory.
 */
extern const struct nvmOps  W25Qx_ops;
extern const struct nvmInfo W25Qx_info;

/**
 *  Instantiate an W25Qx nonvolatile memory device.
 *
 * @param name: device name.
 * @param sz: memory size, in bytes.
 */
#define W25Qx_DEVICE_DEFINE(name, config, sz) \
const struct nvmDevice name =                 \
{                                             \
    .priv = &config,                          \
    .ops  = &W25Qx_ops,                       \
    .info = &W25Qx_info,                      \
    .size = sz,                               \
};

/**
 * Device driver and information block for W25Qx security registers area.
 */
extern const struct nvmOps  W25Qx_secReg_ops;
extern const struct nvmInfo W25Qx_secReg_info;

/**
 *  Instantiate an W25Qx security register memory device.
 *
 * @param name: device name.
 * @param base: security register base address.
 * @param sz: memory size, in bytes.
 */
#define W25Qx_SECREG_DEFINE(name, config, base, sz) \
const struct W25QxSecRegDevice name =               \
{                                                   \
    .priv     = &config,                            \
    .ops      = &W25Qx_secReg_ops,                  \
    .info     = &W25Qx_secReg_info,                 \
    .size     = sz,                                 \
    .baseAddr = base                                \
};

/**
 * Initialise driver for external flash.
 */
void W25Qx_init(const struct nvmDevice *dev);

/**
 * Terminate driver for external flash.
 */
void W25Qx_terminate(const struct nvmDevice *dev);

/**
 * Release flash chip from power down mode, this function should be called at
 * least once after the initialisation of the driver and every time after the
 * chip has been put in low power mode.
 * Application code must wait at least 3us before issuing any other command
 * after this one.
 */
void W25Qx_wakeup(const struct nvmDevice *dev);

/**
 * Put flash chip in low power mode.
 */
void W25Qx_sleep(const struct nvmDevice *dev);

#endif /* W25Qx_H */

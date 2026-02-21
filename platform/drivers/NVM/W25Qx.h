/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef W25Qx_H
#define W25Qx_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include "peripherals/spi.h"
#include "peripherals/gpio.h"
#include "interfaces/nvmem.h"

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
#define W25Qx_DEVICE_DEFINE(name, config)     \
const struct nvmDevice name =                 \
{                                             \
    .priv = &config,                          \
    .ops  = &W25Qx_ops,                       \
    .info = &W25Qx_info,                      \
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
#define W25Qx_SECREG_DEFINE(name, config)           \
const struct nvmDevice name =                       \
{                                                   \
    .priv     = &config,                            \
    .ops      = &W25Qx_secReg_ops,                  \
    .info     = &W25Qx_secReg_info,                 \
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

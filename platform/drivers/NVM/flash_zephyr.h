/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
#define ZEPHYR_FLASH_DEVICE_DEFINE(name, alias)     \
static struct nvmInfo nvm_devInfo_##name;           \
static const struct nvmDevice name =                \
{                                                   \
    .priv   = DEVICE_DT_GET(DT_ALIAS(alias)),       \
    .ops    = &zephyr_flash_ops,                    \
    .info   = &nvm_devInfo_##name,                  \
};


/**
 * Initialize a Zephyr RTOS flash device driver instance.
 *
 * @param dev: device handle.
 * @return zero on success, a negative error code otherwise.
 */
int zephirFlash_init(const struct nvmDevice* dev);

#endif /* FLASH_ZEPHYR_H */

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INTERNAL_FLASH_H
#define INTERNAL_FLASH_H

#include <unistd.h>
#include <stdint.h>
#include "interfaces/nvmem.h"

#ifdef __cplusplus
extern "C" {
#endif

// Currently, only STM32F405 and STM32H743 are used. STM32F405 can have a variable flash size.
// STM32F743/753 is derived in STM32F743xI (2MB size) and STM32F743xG (1MB size).

/**
 * Parameters of a an internal flash area of an mcu. An area is a contiguous
  * memory zone composed of sectors of a same size.
 */
struct internalFlashArea {
    size_t sector_size;
    uint32_t address_low;      ///< Area lower address.
    uint32_t address_high;     ///< Area upper address. 0xFFFFFFFF if variable.
    unsigned int first_sector; ///< Number of the first sector of the area.
};

/**
 *Driver API functions, info and priv parameters.
 */
extern struct nvmOps internalFlash_ops;
extern struct nvmInfo internalFlash_infos;

/**
 * Instantiates an internal flash nonvolatile memory device.
 *
 * @param name device name.
 * @param sector_size sector size to use
 */
#define INTERNAL_FLASH_DEVICE_DEFINE(name)                      \
    static struct nvmInfo name##_nvmInfo;                       \
    static struct nvmDevice name = { .priv = NULL,              \
                                     .ops = &internalFlash_ops, \
                                     .info = &name##_nvmInfo };

/**
 * Initialize an internal flash NVM device. The device must have been defined
 * using INTERNAL_FLASH_DEVICE_DEFINE beforehand.
 *
 * @param dev: pointer to an internal flash nvmem device
 *
 * @return 0 on success, negative error code otherwise
 */
int internalFlash_init(struct nvmDevice *dev, size_t sector_size);

/**
 * Terminates an internal flash NVM device.
 *
 * @param dev: nvmDevice structure
 *
 * @return 0 on success, negative error code otherwise
 */
int internalFlash_terminate(struct nvmDevice *dev);

/**
 * Returns the size of the internal flash in kB.
 *
 * @returns the size of the internal flash of the current device (in kB).
 */
uint16_t internalFlash_size();

#ifdef __cplusplus
}
#endif

#endif

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FLASH_STM32_H
#define FLASH_STM32_H

#include <unistd.h>
#include <stdint.h>
#include "interfaces/nvmem.h"

#ifdef __cplusplus
extern "C" {
#endif

// Currently, only STM32F405 and STM32H743 are used. STM32F405 can have a variable flash size. 
// STM32F743/753 is derived in STM32F743xI (2MB size) and STM32F743xG (1MB size).

/**
 * Flash page size enum
 */
enum STM32_Sector_Size_e{
#if defined(STM32F405xx) || defined(STM32F415xx) || \
    defined(STM32F407xx) || defined(STM32F417xx)
    SECTOR_16K, ///< 16kB sector size
    SECTOR_64K, ///< 64kB sector size
    SECTOR_128K, ///< 128kB sector size
#elif defined(STM32H743xx)
    SECTOR_128K, ///< 128kB sector size
#else
    #error "Missing sector size list for device."
#endif   
};

/**
 * Parameters of a flash area within an STM32 mcu. An area is a contiguous
  * memory zone composed of sectors of a same size.
 */
struct STM32FlashArea{
    uint32_t address_low;       ///< Lowest address of the area.
    uint32_t address_high;      ///< Highest address of the area. 0xFFFFFFFF if variable.
    unsigned int first_sector;  ///< Number of the first sector of the area.
} ;

/**
 *Driver API functions, info and priv parameters.
 */
extern const struct nvmOps stm32_flash_ops;
extern const struct nvmInfo stm32_flash_infos[];
extern const struct STM32FlashArea stm32_flash_priv[];

/**
 * Instantiates an STM32 Flash nonvolatile memory device.
 *
 * @param name device name.
 * @param sector_size sector size to use
 */
#define STM32_FLASH_DEVICE_DEFINE(name, sector_size)    \
static struct nvmDevice name =                \
{                                                       \
    .priv   = &(stm32_flash_priv[sector_size]),         \
    .ops    = &stm32_flash_ops,                         \
    .info   = &(stm32_flash_infos[sector_size]),        \
};                                                  

/**
 * Initialize an STM32Flash NVM device. The device must have been defined 
 * using STM32_FLASH_DEVICE_DEFINE beforehand.
 *
 * @param dev: pointer to a STM32 nonvolatile memory device
 * 
 * @return 0 on success, negative error code otherwise
 */
int STM32Flash_init(struct nvmDevice *dev);

/**
 * Terminates an STM32Flash NVM device.
 *
 * @param dev: nvmDevice structure initialized with STM32_FLASH_DEVICE_DEFINE
 * 
 * @return 0 on success, negative error code otherwise
 */
int STM32Flash_terminate(struct nvmDevice *dev);

#ifdef __cplusplus
}
#endif

#endif

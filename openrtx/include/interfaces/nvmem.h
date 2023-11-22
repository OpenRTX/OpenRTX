/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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
#include <cps.h>
#include <settings.h>
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enumeration field for nonvolatile memory device type.
 */
enum nvmType
{
    NVM_FLASH  = 0,   ///< FLASH type non volatile memory
    NVM_EEPROM,       ///< EEPROM type non volatile memory
    NVM_FILE          ///< File type non volatile memory
};

/**
 * Nonvolatile memory device parameters. The content of this data structure is
 * filled by the device driver and then kept constant.
 */
struct nvmParams
{
    size_t  write_size;    ///< Minimum write size (write unit)
    size_t  erase_size;    ///< Minimum erase size (erase unit)
    size_t  erase_cycles;  ///< Maximum allowed erase cycles of a block
    uint8_t type;          ///< Device type
};

struct nvmDevice;

/**
 * Standard API for nonvolatile memory driver.
 */
struct nvmApi
{
    /**
     * Read data from nonvolatile memory device.
     *
     * @param dev: pointer to NVM device descriptor.
     * @param offset: offset to read, byte aligned.
     * @param data: destination buffer for data read.
     * @param len: number of bytes to read.
     * @return 0 on success, negative errno code on fail.
     */
    int (*read)(const struct nvmDevice *dev, uint32_t offset, void *data, size_t len);

    /**
     * Write data to nonvolatile memory device. On flash memory devices the area
     * has to be erased before the write. This function pointer may be set to
     * NULL if the device does not support writing.
     *
     * @param dev: pointer to NVM device descriptor.
     * @param offset: starting offset for the write, byte aligned.
     * @param data: data to write.
     * @param len: number of bytes to write.
     * @return 0 on success, negative errno code on fail.
     */
    int (*write)(const struct nvmDevice *dev, uint32_t offset, const void *data, size_t len);

    /**
     * Erase part or all of the nonvolatile memory.
     * Acceptable values of erase size and offset are subject to hardware-specific
     * multiples of page size and offset. If the device does not support erase
     * this function pointer is set to NULL.
     *
     * @param dev: pointer to NVM device descriptor.
     * @param offset: starting offset for the erase, byte aligned.
     * @param size: size of the area to be erased.
     * @return 0 on success, negative errno code on fail.
     */
    int (*erase)(const struct nvmDevice *dev, uint32_t offset, size_t size);

    /**
     * Sync device cache and state to its underlying hardware.
     * If the device does not support sync this function pointer is set to NULL.
     *
     * @param dev: pointer to NVM device descriptor.
     * @return 0 on success, negative errno code on fail.
     */
    int (*sync)(const struct nvmDevice *dev);

    /**
     * Get device parameters.
     *
     * @param dev: pointer to NVM device descriptor.
     * @return pointer to the device parameters' data structure.
     */
    const struct nvmParams *(*params)(const struct nvmDevice *dev);
};

/**
 * Nonvolatile memory device driver.
 */
struct nvmDevice
{
    const struct nvmApi *api;       ///< Driver API
    const void          *config;    ///< Driver configuration data
    void  *const         priv;      ///< Driver runtime data
};

/**
 * Data structure representing a partition of a NVM area. The offset of the
 * partition is referred to the beginning of the area itself.
 */
struct nvmPartition
{
    const size_t offset;    ///< Offset from the beginning of the NVM area
    const size_t size;      ///< Size in bytes
};

/**
 * Nonvolatile memory area descriptor. This data structure contains all the data
 * relative to an area of nonvolatile memory with a fixed size, managed by a
 * given device and with zero or more partition.
 */
struct nvmArea
{
    const char                *name;          ///< Area name
    const struct nvmDevice    *dev;           ///< Device driver to manage the area
    const size_t               startAddr;     ///< Start address of the area from the beginning of the device
    const size_t               size;          ///< Size of the area, in bytes
    const struct nvmPartition *partitions;    ///< List of partitions
};


/**
 * Initialise NVM driver.
 */
void nvm_init();

/**
 * Terminate NVM driver.
 */
void nvm_terminate();

/**
 * Get a list of the available nonvolatile memory areas of the device.
 *
 * @param list: pointer where to store the pointer to the list head.
 * @return number of elements in the list.
 */
size_t nvm_getMemoryAreas(const struct nvmArea **list);

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
void nvm_readHwInfo(hwInfo_t *info);

/**
 * Read from storage the channel data corresponding to the VFO channel A.
 *
 * @param channel: pointer to the channel_t data structure to be populated.
 * @return 0 on success, -1 on failure
 */
int nvm_readVfoChannelData(channel_t *channel);

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

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
    NVM_FILE,         ///< File type non volatile memory
    NVM_EEEPROM       ///< Emulated EEPROM type non volatile memory
};

/**
 * Enumeration field for nonvolatile memory properties.
 */
enum nvmProperties
{
    NVM_WRITE    = 0x100,    ///< Device allows write access
    NVM_BITWRITE = 0x200,    ///< Device allows to change the value of single bits
    NVM_ERASE    = 0x400,    ///< Device memory needs to be erased before writing
};

/**
 * Nonvolatile memory device information block. The content of this data structure
 * is defined by the device driver and remains constant.
 */
struct nvmInfo
{
    size_t   write_size;    ///< Minimum write size (write unit)
    size_t   erase_size;    ///< Minimum erase size (erase unit)
    size_t   erase_cycles;  ///< Maximum allowed erase cycles of a block
    uint32_t device_info;   ///< Device type and flags
};

struct nvmDevice;

/**
 * Nonvolatile memory device driver.
 */
struct nvmOps
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
};

struct nvmDevice
{
    const void           *priv;    ///< Device driver private data
    const struct nvmOps  *ops;     ///< Device operations
    const struct nvmInfo *info;    ///< Device info
    const size_t          size;    ///< Device size
};

/**
 * Data structure representing a partition of a nonvolatile memory. The offset
 * of the partition is referred to the beginning of the memory area.
 */
struct nvmPartition
{
    const size_t offset;    ///< Offset from the beginning of the NVM area
    const size_t size;      ///< Size in bytes
};

/**
 * Nonvolatile memory descriptor. This data structure contains all the data
 * relative to an area of nonvolatile memory with a fixed size, managed by a
 * given device and with zero or more partition.
 */
struct nvmDescriptor
{
    const char                *name;        ///< Name
    const struct nvmDevice    *dev;         ///< Associated device driver
    const size_t               partNum;     ///< Number of partitions
    const struct nvmPartition *partitions;  ///< Partion table
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
 * Obtain the descriptor of a given nonvolatile memory.
 *
 * @param index: index of the nonvolatile memory.
 * @return a pointer to the memory descriptor or NULL if the requested descriptor
 * does not exist.
 */
const struct nvmDescriptor *nvm_getDesc(const size_t index);

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

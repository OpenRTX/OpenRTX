/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _EMULATED_FLASH_H_
#define _EMULATED_FLASH_H_

#include "interfaces/nvmem.h"

/**
 * Device driver for file-based emulated flash storage. The driver
 * implementation is based on the POSIX syscalls for file management.
 */

/**
 * Driver configuration data structure.
 */
struct nvmEmulatedFlashDevice {
    const void *priv;           ///< Device driver private data
    const struct nvmOps *ops;   ///< Device operations
    const struct nvmInfo *info; ///< Device info
    int fd;                     ///< File descriptor
};

/**
 * Driver API functions and info.
 */
extern const struct nvmOps emulated_flash_ops;
extern const struct nvmInfo emulated_flash_info;

/**
 * Instantiate a POSIX file storage NVM device.
 *
 * @param name: device name.
 * @param path: full path of the file used for data storage.
 * @param dim: size of the storage file, in bytes.
 */
#define EMULATED_FLASH_DEVICE_DEFINE(name)                                 \
    static struct nvmEmulatedFlashDevice name = {                          \
        .ops = &emulated_flash_ops, .info = &emulated_flash_info, .fd = -1 \
    };

/**
 * Initialize an emulated flash driver instance.
 * This function allows also to override the path of the file used for data
 * storage, where necessary.
 *
 * @param dev: pointer to device descriptor.
 * @param fileName: full path of the file used for data storage.
 * @param size: file size.
 * @return zero on success, a negative error code otherwise.
 */
int emulatedFlash_init(struct nvmEmulatedFlashDevice *dev, const char *fileName,
                       const size_t size);

/**
 * Shut down an emulated flash driver instance.
 *
 * @param dev: pointer to device descriptor.
 * @param maxSize: maximum size for the storage file, in bytes.
 * @return zero on success, a negative error code otherwise.
 */
int emulatedFlash_terminate(struct nvmEmulatedFlashDevice *dev);

#endif /* _EMULATED_FLASH_H_ */

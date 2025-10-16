/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef POSIX_FILE_H
#define POSIX_FILE_H

#include "interfaces/nvmem.h"

/**
 * Device driver for file-based nonvolatile memory storage. The driver
 * implementation is based on the POSIX syscalls for file management.
 */


/**
 * Driver configuration data structure.
 */
struct nvmFileDevice
{
    const void           *priv;    ///< Device driver private data
    const struct nvmOps  *ops;     ///< Device operations
    const struct nvmInfo *info;    ///< Device info
    const size_t          size;    ///< Device size
    int                     fd;    ///< File descriptor
};

/**
 * Driver API functions and info.
 */
extern const struct nvmOps  posix_file_ops;
extern const struct nvmInfo posix_file_info;

/**
 * Instantiate a POSIX file storage NVM device.
 *
 * @param name: device name.
 * @param path: full path of the file used for data storage.
 * @param dim: size of the storage file, in bytes.
 */
#define POSIX_FILE_DEVICE_DEFINE(name, dim) \
static struct nvmFileDevice name =          \
{                                           \
    .ops  = &posix_file_ops,                \
    .info = &posix_file_info,               \
    .size = dim,                            \
    .fd   = -1                              \
};

/**
 * Initialize a POSIX file driver instance.
 * This function allows also to override the path of the file used for data
 * storage, where necessary.
 *
 * @param dev: pointer to device descriptor.
 * @param fileName: full path of the file used for data storage.
 * @return zero on success, a negative error code otherwise.
 */
int posixFile_init(struct nvmFileDevice *dev, const char *fileName);

/**
 * Shut down a POSIX file driver instance.
 *
 * @param dev: pointer to device descriptor.
 * @param maxSize: maximum size for the storage file, in bytes.
 * @return zero on success, a negative error code otherwise.
 */
int posixFile_terminate(struct nvmFileDevice *dev);

#endif /* POSIX_FILE_H */

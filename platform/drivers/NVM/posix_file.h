/***************************************************************************
 *   Copyright (C) 2023 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef POSIX_FILE_H
#define POSIX_FILE_H

#include <interfaces/nvmem.h>

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

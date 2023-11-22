/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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
struct posixFileCfg
{
    const char  *fileName;     ///< Full path of the file used for data storage
    const size_t fileSize;     ///< File size, in bytes
};

/**
 * Driver API functions.
 */
extern const struct nvmApi posix_file_api;


/**
 * Instantiate a POSIX file storage NVM device.
 *
 * @param name: device name.
 * @param path: full path of the file used for data storage.
 * @param size: size of the storage file, in bytes.
 */
#define POSIX_FILE_DEVICE_DEFINE(name, path, size)  \
static int fd_##name;                               \
static const struct posixFileCfg cfg_##name =       \
{                                                   \
    .fileName = path,                               \
    .fileSize = size                                \
};                                                  \
static const struct nvmDevice name =                \
{                                                   \
    .config = &cfg_##name,                          \
    .priv   = &fd_##name,                           \
    .api    = &posix_file_api                       \
};

/**
 * Initialize a POSIX file driver instance.
 * This function allows also to override the path of the file used for data
 * storage, where necessary.
 *
 * @param dev: pointer to device descriptor.
 * @param fileName: alternative path of the file used for data storage or NULL.
 * @return zero on success, a negative error code otherwise.
 */
int posixFile_init(const struct nvmDevice *dev, const char *fileName);

/**
 * Shut down a POSIX file driver instance.
 *
 * @param dev: pointer to device descriptor.
 * @param maxSize: maximum size for the storage file, in bytes.
 * @return zero on success, a negative error code otherwise.
 */
int posixFile_terminate(const struct nvmDevice *dev);

#endif /* POSIX_FILE_H */

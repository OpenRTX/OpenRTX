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

#ifndef CHARDEV_H
#define CHARDEV_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

struct chardev;

/**
 * Character device ioctl operations.
 */
enum ChardevIoctl
{
    IOCTL_SYNC     = 100,
    IOCTL_FLUSH    = 101,
    IOCTL_SETSPEED = 102,
};

/**
 * Character device API.
 */
struct chardev_api
{
    int     (*init)      (const struct chardev *dev);
    int     (*terminate) (const struct chardev *dev);
    ssize_t (*read)      (const struct chardev *dev, void *data, const size_t len);
    ssize_t (*write)     (const struct chardev *dev, const void *data, const size_t len);
    int     (*ioctl)     (const struct chardev *dev, const int cmd, const void *arg);
};


/**
 * Character device descriptor.
 */
struct chardev
{
    const void *config;              ///< Device config
    void * const data;               ///< Device private data
    const struct chardev_api *api;   ///< Device driver API
};


/**
 * Initialize a character device.
 *
 * @return zero on success or a negative errno code on fail.
 */
static inline int chardev_init(const struct chardev *dev)
{
    return dev->api->init(dev);
}

/**
 * Shut down a character device and free all its resources.
 *
 * @return zero on success or a negative errno code on fail.
 */
static inline int chardev_terminate(const struct chardev *dev)
{
    return dev->api->terminate(dev);
}

/**
 * Read data from a character device, nonblocking function.
 * Is normal for this function to return less character than the amount asked.
 *
 * @param dev: character device.
 * @param data: destination buffer for data read.
 * @param len: number of bytes to read.
 * @return number of bytes read or a negative errno code on fail.
 */
static inline ssize_t chardev_read(const struct chardev *dev, void *data,
                                   const size_t len)
{
    return dev->api->read(dev, data, len);
}

/**
 * Write data to character device.
 *
 * @param dev: character device.
 * @param data: data to write.
 * @param len: number of bytes to write.
 * @return number of bytes written or negative errno code on fail.
 */
static inline ssize_t chardev_write(const struct chardev *dev, const void *data,
                                    const size_t len)
{
    return dev->api->write(dev, data, len);
}

/**
 * Perform an I/O control operation on a character device.
 *
 * @param dev: character device.
 * @param cmd: ioctl command.
 * @param arg: ioctl command arguments.
 * @return 0 on success, negative errno code on fail.
 */
static inline int chardev_ioctl(const struct chardev *dev, const int cmd,
                                const void *arg)
{
    return dev->api->ioctl(dev, cmd, arg);
}


#ifdef __cplusplus
}
#endif

#endif /* CHARDEV_H */

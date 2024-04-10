/***************************************************************************
 *   Copyright (C) 2024 by Federico Amedeo Izzo IU2NUO,                    *
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

#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>

/**
 * Enumeration type for I2C bus speeds
 */
enum I2CSpeed
{
    I2C_SPEED_LOW = 0,  ///< Generic, implementation-specific, low speed
    I2C_SPEED_100kHz,   ///< Standard I2C speed
    I2C_SPEED_400kHz,   ///< Fast I2C speed
    I2C_SPEED_1MHz      ///< Fast Plus I2C speed
};

struct i2cDevice;

/**
 * I2C driver API.
 */
struct i2cApi
{
    /**
     * Perform a read operation on an I2C device.
     *
     * @param dev: pointer to the device managing the bus.
     * @param addr: 7-bit address of the remote peripheral.
     * @param data: pointer to the destination buffer for data read.
     * @param len: number of bytes to read.
     * @param stop: set to true to send a stop condition at the end of the transfer.
     * @return zero on success, a negative error code otherwise.
     */
    int (*read)(const struct i2cDevice *dev, const uint8_t addr, void *data,
                const size_t len, const bool stop);

    /**
     * Perform a write operation on an I2C device.
     *
     * @param dev: pointer to the device managing the bus.
     * @param addr: 7-bit address of the remote peripheral.
     * @param data: data to be written.
     * @param len: number of bytes to read.
     * @param stop: set to true to send a stop condition at the end of the transfer.
     * @return zero on success, a negative error code otherwise.
     */
    int (*write)(const struct i2cDevice *dev, const uint8_t addr,
                 const void *data, const size_t len, const bool stop);
};

/**
 * I2C peripheral descriptor.
 */
struct i2cDevice
{
    const struct i2cApi   *driver;     ///< Driver API
    const void            *periph;     ///< Underlying hardware peripheral
    const pthread_mutex_t *mutex;      ///< Mutex for exclusive access
};


/**
 * Acquire exclusive ownership on an I2C peripheral.
 *
 * @param dev: pointer to peripheral descriptor.
 * @return zero on success, a negative error code on failure.
 */
static inline int i2c_acquire(const struct i2cDevice *dev)
{
    if(dev->mutex == NULL)
        return 0;

    return pthread_mutex_lock((pthread_mutex_t *) dev->mutex);
}

/**
 * Attempt to acquire exclusive ownership on an I2C peripheral.
 *
 * @param dev: pointer to peripheral descriptor.
 * @return zero on success, a negative error code on failure.
 */
static inline int i2c_tryAcquire(const struct i2cDevice *dev)
{
    if(dev->mutex == NULL)
        return 0;

    return pthread_mutex_trylock((pthread_mutex_t *) dev->mutex);
}

/**
 * Release exclusive ownership on an I2C peripheral.
 *
 * @param dev: pointer to peripheral descriptor.
 * @return zero on success, a negative error code on failure.
 */
static inline int i2c_release(const struct i2cDevice *dev)
{
    if(dev->mutex == NULL)
        return 0;

    return pthread_mutex_unlock((pthread_mutex_t *) dev->mutex);
}

/**
 * Read data from an I2C device connected on the bus managed by the peripheral.
 *
 * @param dev: pointer to peripheral descriptor.
 * @param addr: 7-bit device address.
 * @param data: pointer to a buffer where to store the received data.
 * @param len: number of bytes to read.
 * @param stop: if true, an I2C stop condition is generated at the end of the
 * transfer.
 * @return zero on success, a negative error code on failure.
 */
static inline int i2c_read(const struct i2cDevice *dev, const uint8_t addr,
                           void *data, const size_t len, const bool stop)
{
    return dev->driver->read(dev, addr, data, len, stop);
}

/**
 * Send data to an I2C device connected on the bus managed by the peripheral.
 *
 * @param dev: pointer to peripheral descriptor.
 * @param addr: 7-bit device address.
 * @param data: pointer to the data to be sent.
 * @param len: number of bytes to send.
 * @param stop: if true, an I2C stop condition is generated at the end of the
 * transfer.
 * @return zero on success, a negative error code on failure.
 */
static inline int i2c_write(const struct i2cDevice *dev, const uint8_t addr,
                            const void *data, const size_t len, const bool stop)
{
    return dev->driver->write(dev, addr, data, len, stop);
}

#endif /* I2C_H */

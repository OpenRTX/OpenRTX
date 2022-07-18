/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Silvano Seva IU2KWO                      *
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

#ifndef USB_VCOM_H
#define USB_VCOM_H

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Size of the reception buffer for incoming data from the USB host, in bytes.
 * NOTE: value is equal to the size of one USB bulk transfer, do not change
 * this parameter.
 */
#define RX_RING_BUF_SIZE 1024

/**
 * Initialise USB virtual com port. Parameters: 115200 baud, 8N1.
 * @return zero on success, negative value on failure.
 */
int vcom_init();

/**
* Write a block of data. This function blocks until all data have been sent.
* \param buffer buffer where take data to write.
* \param size buffer size
* \return number of bytes written or a negative number on failure.
*/
ssize_t vcom_writeBlock(const void *buf, size_t len);

/**
* Read a block of data, nonblocking function.
* \param buffer buffer where read data will be stored.
* \param size buffer size.
* \return number of bytes read or a negative number on failure. Note that
* it is normal for this function to return less character than the amount
* asked.
*/
ssize_t vcom_readBlock(void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* USB_VCOM_H */

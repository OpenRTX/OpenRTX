/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
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

#ifndef USB_H
#define USB_H

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise the USB driver.
 */
void usb_init();

/**
 * Shut down the USB driver.
 */
void usb_terminate();

/**
* Write a block of data. This function blocks until all data have been sent.
* \param buffer buffer where take data to write.
* \param size buffer size
* \return number of bytes written or a negative number on failure.
*/
ssize_t usb_vcom_writeBlock(const void *buf, size_t len);

/**
* Read a block of data, nonblocking function.
* \param buffer buffer where read data will be stored.
* \param size buffer size.
* \return number of bytes read or a negative number on failure. Note that
* it is normal for this function to return less character than the amount
* asked.
*/
ssize_t usb_vcom_readBlock(void *buf, size_t len);


#ifdef __cplusplus
}
#endif

#endif /* USB_H */

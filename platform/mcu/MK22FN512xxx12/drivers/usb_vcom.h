/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef USB_VCOM_H
#define USB_VCOM_H

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise USB virtual com port. Parameters: 115200 baud, 8N1.
 * @return zero on success, negative value on failure.
 */
void vcom_init();

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

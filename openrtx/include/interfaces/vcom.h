/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VCOM_H
#define VCOM_H

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Virtual serial port to a host computer, such as a USB CDC-ACM port: a
 * byte stream for the console, data logs and host protocols. Each MCU or
 * target with such a port implements this interface.
 */

/**
 * Initialise the virtual serial port.
 *
 * @return 0 on success, a negative error code on failure.
 */
int vcom_init();

/**
 * Write a block of data, waiting until it has been handed to the host.
 *
 * @param buf: data to write.
 * @param len: number of bytes to write.
 * @return the number of bytes written, or a negative error code when the
 * data could not be sent, for example because no host is reading.
 */
ssize_t vcom_writeBlock(const void *buf, size_t len);

/**
 * Read the data received from the host so far, without waiting.
 *
 * @param buf: buffer for the data.
 * @param len: size of the buffer, in bytes.
 * @return the number of bytes read, possibly fewer than asked or zero, or
 * a negative error code on failure.
 */
ssize_t vcom_readBlock(void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* VCOM_H */

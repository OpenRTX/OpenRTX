/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef USART6_H
#define USART6_H

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise USART1 peripheral with a given baud rate. Serial communication is
 * configured for 8 data bits, no parity, one stop bit.
 *
 * @param baudrate: serial port baud rate, in bits per second.
 */
void usart6_init(unsigned int baudrate);

/**
 * Shut down USART1 peripheral.
 */
void usart6_terminate();

/**
 * Read a block of data.
 *
 * \param buffer buffer where read data will be stored.
 * \param size buffer size.
 * \param where where to read from.
 * \return number of bytes read or a negative number on failure. Note that
 * it is normal for this function to return less character than the amount
 * asked.
 */
ssize_t usart6_readBlock(void *buffer, size_t size, off_t where);

/**
 * Write a block of data.
 *
 * \param buffer buffer where take data to write.
 * \param size buffer size.
 * \param where where to write to.
 * \return number of bytes written or a negative number on failure.
 */
ssize_t usart6_writeBlock(void *buffer, size_t size, off_t where);

/**
 * Write a string.
 * Can be used to write debug information before the kernel is started or in
 * case of serious errors, right before rebooting.
 * Can ONLY be called when the kernel is not yet started, paused or within
 * an interrupt. This default implementation ignores writes.
 *
 * \param str the string to write. The string must be NUL terminated.
 */
void usart6_IRQwrite(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* USART6_H */
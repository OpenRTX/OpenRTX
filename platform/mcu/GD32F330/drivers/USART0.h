/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IUNUO,                     *
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

#ifndef USART1_H
#define USART1_H

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
void usart0_init(unsigned int baudrate);

/**
 * Shut down USART1 peripheral.
 */
void usart0_terminate();

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
ssize_t usart0_readBlock(void *buffer, size_t size, off_t where);

/**
 * Write a block of data.
 *
 * \param buffer buffer where take data to write.
 * \param size buffer size.
 * \param where where to write to.
 * \return number of bytes written or a negative number on failure.
 */
ssize_t usart0_writeBlock(void *buffer, size_t size, off_t where);

/**
 * Write a string.
 * Can be used to write debug information before the kernel is started or in
 * case of serious errors, right before rebooting.
 * Can ONLY be called when the kernel is not yet started, paused or within
 * an interrupt. This default implementation ignores writes.
 *
 * \param str the string to write. The string must be NUL terminated.
 */
void usart0_IRQwrite(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* USART1_H */

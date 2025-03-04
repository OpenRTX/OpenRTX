/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   Adapted from STM32 USART driver for miosix kernel written by Federico *
 *   Terraneo.                                                             *
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
 *                                                                         *
 *   (2025) Added and modified by KD0OSS for use in Module17/OpenRTX       *
 ***************************************************************************/

#ifndef USART3_NOD17_H
#define USART3_NOD17_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise USART3_NOD17 peripheral with a given baud rate. Serial communication is
 * configured for 8 data bits, no parity, one stop bit.
 *
 * @param baudrate: serial port baud rate, in bits per second.
 */
void usart3_mod17_init(unsigned int baudrate);

/**
 * Shut down USART3_NOD17 peripheral.
 */
void usart3_mod17_terminate();

/**
 * Read a block of data.
 *
 * \param buffer buffer where read data will be stored.
 * \param size buffer size.
 * \return number of bytes read or a negative number on failure. Note that
 * it is normal for this function to return less character than the amount
 * asked.
 */
ssize_t usart3_mod17_readBlock(void *buffer, size_t size);

/**
 * Write a block of data.
 *
 * \param buffer buffer where take data to write.
 * \param size buffer size.
 * \return number of bytes written or a negative number on failure.
 */
ssize_t usart3_mod17_writeBlock(void *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* USART3_NOD17_H */

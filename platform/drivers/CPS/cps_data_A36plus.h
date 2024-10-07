/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                Andrej Antunovikj K8TUN                  *
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

#ifndef NVMDATA_A36PLUS_H
#define NVMDATA_A36PLUS_H

#include <stdint.h>

/**
 * \internal Data structures matching the one used by original A36Plus firmware to
 * manage channel data inside nonvolatile flash memory.
 */
typedef struct
{
   uint32_t rx_frequency;
   uint32_t tx_frequency;
   uint16_t ctcss_receive; // divide by 10 to get Hz
   uint16_t ctcss_transmit; // divide by 10 to get Hz
   uint8_t unknown[2];
   uint8_t power; // high 0x00, low 0x01, medium 0x02
   uint8_t bandwidth : 2, // 0x00 = 12.5 kHz, 0x01 = 25 kHz
           unknown2 : 6; // encrypt, bcl, scan include in the original firmware
   uint8_t unknown3[4]; // always 0xff? maybe the name can be 16 chars
   char name[12]; // channel name
}
__attribute__((packed)) a36plusChannel_t;
// 33 bytes total, for some reason the null terminator is not included in the 16 byte name

#endif /* NVMDATA_A36PLUS_H */

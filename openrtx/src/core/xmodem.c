/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <crc.h>
#include <xmodem.h>
#include <string.h>
#include <stdio.h>
#include <usb_vcom.h>

#define SOH     (0x01)  /* start of 128-byte data packet */
#define STX     (0x02)  /* start of 1024-byte data packet */
#define EOT     (0x04)  /* End Of Transmission */
#define ACK     (0x06)  /* ACKnowledge, receive OK */
#define NAK     (0x15)  /* Negative ACKnowledge, receiver ERROR, retry */
#define CAN     (0x18)  /* two CAN in succession will abort transfer */
#define CRC     (0x43)  /* 'C' == 0x43, request 16-bit CRC, use in place of first NAK for CRC mode */
#define ABT1    (0x41)  /* 'A' == 0x41, assume try abort by user typing */
#define ABT2    (0x61)  /* 'a' == 0x61, assume try abort by user typing */



void xmodem_sendPacket(const void *data, size_t size, uint8_t blockNum)
{
    // Bad payload size, null block number or null data pointer: do not send
    if(((size != 128) && (size != 1024)) || (blockNum == 0) || (data == NULL))
    {
        return;
    }

    uint8_t buf[3] = {SOH, 0x00, 0x00};

    // Override header to STX for 1kB packets
    if(size > 128)
    {
        buf[0] = STX;
    }

    // Sequence number
    buf[1] = blockNum;
    buf[2] = blockNum ^ 0xFF;

    uint16_t crc = crc_ccitt(data, size);

    // Send header, then data and finally CRC
    vcom_writeBlock(buf, 3);
    vcom_writeBlock(data, size);

    buf[0] = crc >> 8;
    buf[1] = crc & 0xFF;
    vcom_writeBlock(buf, 2);
}

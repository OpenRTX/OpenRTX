/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
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

#include <stdio.h>
#include <string.h>
#include <xmodem.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include "W25Qx.h"
#include "usb_vcom.h"

#define SOH     (0x01)  /* start of 128-byte data packet */
#define STX     (0x02)  /* start of 1024-byte data packet */
#define EOT     (0x04)  /* End Of Transmission */
#define ACK     (0x06)  /* ACKnowledge, receive OK */
#define NAK     (0x15)  /* Negative ACKnowledge, receiver ERROR, retry */
#define CAN     (0x18)  /* two CAN in succession will abort transfer */
#define CRC     (0x43)  /* 'C' == 0x43, request 16-bit CRC, use in place of first NAK for CRC mode */
#define ABT1    (0x41)  /* 'A' == 0x41, assume try abort by user typing */
#define ABT2    (0x61)  /* 'a' == 0x61, assume try abort by user typing */

static const size_t FLASH_SIZE = 16*1024*1024;  /* 16MB */
uint8_t blockData[1024];

int main()
{
    platform_init();
    W25Qx_init();
    W25Qx_wakeup();

    uint8_t cmd = 0;
    while(cmd != 'C')
    {
        platform_ledOn(GREEN);
        sleepFor(0,50);
        platform_ledOff(GREEN);
        sleepFor(0,50);
        vcom_readBlock(&cmd, 1);
    }


    uint8_t block = 1;
    for(size_t addr = 0; addr < FLASH_SIZE; )
    {
        W25Qx_readData(addr, blockData, 1024);

        bool ok = false;
        do
        {
            xmodem_sendPacket(blockData, 1024, block);

            cmd = 0;
            while((cmd != ACK) && (cmd != NAK))
            {
                platform_ledOn(RED);
                sleepFor(0,50);
                platform_ledOff(RED);
                sleepFor(0,50);
                vcom_readBlock(&cmd, 1);

                if(cmd == ACK) ok = true;
            }
        }
        while(ok == false);

        block++;
        if(block == 255) block = 1;
        addr += 1024;
    }

    cmd = EOT;
    vcom_writeBlock(&cmd, 1);
    while(cmd != ACK)
    {
        vcom_readBlock(&cmd, 1);
    }

    while(1)
    {
        platform_ledOn(GREEN);
        sleepFor(1,0);
        platform_ledOff(GREEN);
        sleepFor(1,0);
    }

    return 0;
}

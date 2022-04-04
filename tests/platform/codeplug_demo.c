/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
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
#include <stdint.h>
#include <sys/types.h>
#include <interfaces/nvmem.h>
#include <cps.h>

int main()
{
    nvm_init();

    getchar();
    printf("Codeplug Demo!\r\n\r\n");
    printf("Channels:\r\n");
    for(int pos=0,result=0; result != -1; pos++)
    {
        channel_t ch;
        result = cps_readChannel(&ch, pos);
        if(result != -1)
        {
            printf("Channel n.%d:\r\n", pos+1);
            printf("  %s\r\n  TX: %ld\r\n  RX: %ld\r\n  Mode: %s\r\n  Bandwidth: %s\r\n",
                   ch.name,
                   ch.tx_frequency,
                   ch.rx_frequency,
                   (ch.mode == 1) ? "DMR" : "FM",
                   (ch.bandwidth == BW_12_5) ? "12.5kHz" : ((ch.bandwidth == BW_20)
                                                              ? "20kHz" : "25kHz"));
        }
        puts("\r");
    }
    printf("Zones:\r\n");
    for(int pos=0,result=0; result != -1; pos++)
    {
        zone_t bank;
        result = cps_readZoneData(&bank, pos);
        if(result != -1)
        {
            printf("Zone n.%d:\r\n", pos+1);
            printf("  %s\r\n", bank.name);
            for(int x=0; x < 64; x++)
            {
                if(bank.member[x] != 0)
                {
                    printf("  - Index: %d, Channel %d\r\n", x, bank.member[x]);
                }
            }
        }
        puts("\r");
    }
    printf("Contacts:\r\n");
    for(int pos=0,result=0; result != -1; pos++)
    {
        contact_t contact;
        result = cps_readContact(&contact, pos);
        if(result != -1)
        {
            printf("Contact n.%d:\r\n", pos+1);
            printf("  %s\r\n", contact.name);
            printf("  - DMR ID:%lu\r\n", contact.id);
            printf("  - Type:%d\r\n", contact.type);
            printf("  - Receive Tone:%s\r\n",  contact.receive_tone ? "True" : "False");
        }
        puts("\r");
    }
    return 0;
}

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

    uint16_t pos = 0;

    while(1)
    {
        getchar();

        channel_t ch;
        cps_readChannel(&ch, pos);
        printf("Contact entry %d:\r\n", pos+1);
        printf("  %s\r\n  TX: %ld\r\n  RX: %ld\r\n  Mode: %s\r\n  Bandwidth: %s\r\n",
               ch.name,
               ch.tx_frequency,
               ch.rx_frequency,
               (ch.mode == 1) ? "DMR" : "FM",
               (ch.bandwidth == BW_12_5) ? "12.5kHz" : ((ch.bandwidth == BW_20)
                                                          ? "20kHz" : "25kHz"));
        puts("\r");
        pos += 1;
    }

    return 0;
}

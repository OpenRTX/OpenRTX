/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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
#include <interfaces/platform.h>

int main()
{
    platform_init();

    const hwInfo_t *info = platform_getHwInfo();

    while(1)
    {
        getchar();
        puts("** Hardware information **\r\n\r");
        printf("- Hardware name: %s\r\n", info->name);
        printf("- Band support: VHF %s, UHF %s\r\n", info->vhf_band ? "yes" : "no",
                                                   info->uhf_band ? "yes" : "no");
        printf("- VHF band range: %d - %d MHz\r\n", info->vhf_minFreq, info->vhf_maxFreq);
        printf("- UHF band range: %d - %d MHz\r\n", info->uhf_minFreq, info->uhf_maxFreq);
        printf("- Display type: %d\r\n\r\n", info->lcd_type);
    }

    return 0;
}

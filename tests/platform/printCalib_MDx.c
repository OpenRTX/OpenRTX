/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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
#include <sys/types.h>
#include <interfaces/nvmem.h>
#include <calibInfo_MDx.h>
#include <interfaces/delays.h>

void printCalData(md3x0Calib_t cal) {
    uint8_t i;

    printf("freqAdjustMid: %03d\r\n",   cal.freqAdjustMid);

    printf("\r\n\r\nrxFreq: ");         for(i = 0; i < 9; i++) printf("%ld ", cal.rxFreq[i]);
    printf("\r\ntxFreq: ");             for(i = 0; i < 9; i++) printf("%ld ", cal.txFreq[i]);
    printf("\r\ntxHighPower: ");        for(i = 0; i < 9; i++) printf("%03d ", cal.txHighPower[i]);
    printf("\r\ntxLowPower: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.txLowPower[i]);
    printf("\r\nrxSensitivity: ");      for(i = 0; i < 9; i++) printf("%03d ", cal.rxSensitivity[i]);
    printf("\r\nsendIrange: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.sendIrange[i]);
    printf("\r\nsendQrange: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.sendQrange[i]);
    printf("\r\nanalogSendIrange: ");   for(i = 0; i < 9; i++) printf("%03d ", cal.analogSendIrange[i]);
    printf("\r\nanalogSendQrange: ");   for(i = 0; i < 9; i++) printf("%03d ", cal.analogSendQrange[i]);
}

void printMD3x0calibration()
{
    md3x0Calib_t cal;
    nvm_readCalibData(&cal);

    printCalData(cal);
}

void printMDUV3x0calibration()
{
    mduv3x0Calib_t cal;
    nvm_readCalibData(&cal);

    puts("\r\nUHF band:\r");
    printCalData(cal.uhfCal);

    puts("\r\nVHF band:\r");
    printCalData(cal.vhfCal);
}

int main()
{
    nvm_init();

    while(1)
    {
        delayMs(5000);

        #ifdef PLATFORM_MD3x0
        printMD3x0calibration();
        #else
        printMDUV3x0calibration();
        #endif
    }

    return 0;
}

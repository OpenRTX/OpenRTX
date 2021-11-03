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

#include <calibInfo_MDx.h>
#include <interfaces/nvmem.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

void printMD3x0calibration()
{
    uint8_t i;
    md3x0Calib_t cal;
    nvm_readCalibData(&cal);

    printf("vox1: %03d\r\n", cal.vox1);
    printf("vox10: %03d\r\n", cal.vox10);
    printf("rxLowVoltage: %03d\r\n", cal.rxLowVoltage);
    printf("rxFullVoltage: %03d\r\n", cal.rxFullVoltage);
    printf("rssi1: %03d\r\n", cal.rssi1);
    printf("rssi4: %03d\r\n", cal.rssi4);
    printf("analogMic: %03d\r\n", cal.analogMic);
    printf("digitalMic: %03d\r\n", cal.digitalMic);
    printf("freqAdjustHigh: %03d\r\n", cal.freqAdjustHigh);
    printf("freqAdjustMid: %03d\r\n", cal.freqAdjustMid);
    printf("freqAdjustLow: %03d", cal.freqAdjustLow);

    printf("\r\n\r\nrxFreq: ");
    for (i = 0; i < 9; i++) printf("%ld ", cal.rxFreq[i]);
    printf("\r\ntxFreq: ");
    for (i = 0; i < 9; i++) printf("%ld ", cal.txFreq[i]);
    printf("\r\ntxHighPower: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.txHighPower[i]);
    printf("\r\ntxLowPower: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.txLowPower[i]);
    printf("\r\nrxSensitivity: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.rxSensitivity[i]);
    printf("\r\nopenSql9: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.openSql9[i]);
    printf("\r\ncloseSql9: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.closeSql9[i]);
    printf("\r\nopenSql1: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.openSql1[i]);
    printf("\r\ncloseSql1: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.closeSql1[i]);
    printf("\r\nmaxVolume: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.maxVolume[i]);
    printf("\r\nctcss67Hz: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.ctcss67Hz[i]);
    printf("\r\nctcss151Hz: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.ctcss151Hz[i]);
    printf("\r\nctcss254Hz: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.ctcss254Hz[i]);
    printf("\r\ndcsMod2: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.dcsMod2[i]);
    printf("\r\ndcsMod1: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.dcsMod1[i]);
    printf("\r\nmod1Partial: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.mod1Partial[i]);
    printf("\r\nanalogVoiceAdjust: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.analogVoiceAdjust[i]);
    printf("\r\nlockVoltagePartial: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.lockVoltagePartial[i]);
    printf("\r\nsendIpartial: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.sendIpartial[i]);
    printf("\r\nsendQpartial: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.sendQpartial[i]);
    printf("\r\nsendIrange: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.sendIrange[i]);
    printf("\r\nsendQrange: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.sendQrange[i]);
    printf("\r\nrxIpartial: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.rxIpartial[i]);
    printf("\r\nrxQpartial: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.rxQpartial[i]);
    printf("\r\nanalogSendIrange: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.analogSendIrange[i]);
    printf("\r\nanalogSendQrange: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.analogSendQrange[i]);
}

void printMDUV3x0calibration()
{
    uint8_t i;
    mduv3x0Calib_t cal;
    nvm_readCalibData(&cal);

    printf("vox1: %03d\r\n", cal.vox1);
    printf("vox10: %03d\r\n", cal.vox10);
    printf("rxLowVoltage: %03d\r\n", cal.rxLowVoltage);
    printf("rxFullVoltage: %03d\r\n", cal.rxFullVoltage);
    printf("rssi1: %03d\r\n", cal.rssi1);
    printf("rssi4: %03d\r\n", cal.rssi4);

    puts("\r\nUHF band:\r");
    printf("freqAdjustMid: %03d\r\n", cal.uhfCal.freqAdjustMid);
    printf("\r\nrxFreq: ");
    for (i = 0; i < 9; i++) printf("%ld ", cal.uhfCal.rxFreq[i]);
    printf("\r\ntxFreq: ");
    for (i = 0; i < 9; i++) printf("%ld ", cal.uhfCal.txFreq[i]);
    printf("\r\ntxHighPower: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.txHighPower[i]);
    printf("\r\ntxMidPower: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.txMidPower[i]);
    printf("\r\ntxLowPower: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.txLowPower[i]);
    printf("\r\nrxSensitivity: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.rxSensitivity[i]);
    printf("\r\nopenSql9: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.openSql9[i]);
    printf("\r\ncloseSql9: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.closeSql9[i]);
    printf("\r\nopenSql1: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.openSql1[i]);
    printf("\r\ncloseSql1: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.closeSql1[i]);
    printf("\r\nctcss67Hz: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.ctcss67Hz[i]);
    printf("\r\nctcss151Hz: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.ctcss151Hz[i]);
    printf("\r\nctcss254Hz: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.ctcss254Hz[i]);
    printf("\r\ndcsMod1: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.dcsMod1[i]);
    printf("\r\nsendIrange: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.sendIrange[i]);
    printf("\r\nsendQrange: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.sendQrange[i]);
    printf("\r\nanalogSendIrange: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.analogSendIrange[i]);
    printf("\r\nanalogSendQrange: ");
    for (i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.analogSendQrange[i]);

    puts("\r\nVHF band:\r");
    printf("freqAdjustMid: %03d\r\n", cal.vhfCal.freqAdjustMid);
    printf("\r\nrxFreq: ");
    for (i = 0; i < 5; i++) printf("%ld ", cal.vhfCal.rxFreq[i]);
    printf("\r\ntxFreq: ");
    for (i = 0; i < 5; i++) printf("%ld ", cal.vhfCal.txFreq[i]);
    printf("\r\ntxHighPower: ");
    for (i = 0; i < 5; i++) printf("%03d ", cal.vhfCal.txHighPower[i]);
    printf("\r\ntxMidPower: ");
    for (i = 0; i < 5; i++) printf("%03d ", cal.vhfCal.txMidPower[i]);
    printf("\r\ntxLowPower: ");
    for (i = 0; i < 5; i++) printf("%03d ", cal.vhfCal.txLowPower[i]);
    printf("\r\nrxSensitivity: ");
    for (i = 0; i < 5; i++) printf("%03d ", cal.vhfCal.rxSensitivity[i]);
    printf("\r\nopenSql9: ");
    for (i = 0; i < 5; i++) printf("%03d ", cal.vhfCal.openSql9[i]);
    printf("\r\ncloseSql9: ");
    for (i = 0; i < 5; i++) printf("%03d ", cal.vhfCal.closeSql9[i]);
    printf("\r\nopenSql1: ");
    for (i = 0; i < 5; i++) printf("%03d ", cal.vhfCal.openSql1[i]);
    printf("\r\ncloseSql1: ");
    for (i = 0; i < 5; i++) printf("%03d ", cal.vhfCal.closeSql1[i]);
    printf("\r\nctcss67Hz: ");
    for (i = 0; i < 5; i++) printf("%03d ", cal.vhfCal.ctcss67Hz[i]);
    printf("\r\nctcss151Hz: ");
    for (i = 0; i < 5; i++) printf("%03d ", cal.vhfCal.ctcss151Hz[i]);
    printf("\r\nctcss254Hz: ");
    for (i = 0; i < 5; i++) printf("%03d ", cal.vhfCal.ctcss254Hz[i]);
    printf("\r\ndcsMod1: ");
    for (i = 0; i < 5; i++) printf("%03d ", cal.vhfCal.dcsMod1[i]);
    printf("\r\nsendIrange: ");
    for (i = 0; i < 5; i++) printf("%03d ", cal.vhfCal.sendIrange[i]);
    printf("\r\nsendQrange: ");
    for (i = 0; i < 5; i++) printf("%03d ", cal.vhfCal.sendQrange[i]);
    printf("\r\nanalogSendIrange: ");
    for (i = 0; i < 5; i++) printf("%03d ", cal.vhfCal.analogSendIrange[i]);
    printf("\r\nanalogSendQrange: ");
    for (i = 0; i < 5; i++) printf("%03d ", cal.vhfCal.analogSendQrange[i]);
}

int main()
{
    nvm_init();

    while (1)
    {
        getchar();

#ifdef PLATFORM_MD3x0
        printMD3x0calibration();
#else
        printMDUV3x0calibration();
#endif
    }

    return 0;
}

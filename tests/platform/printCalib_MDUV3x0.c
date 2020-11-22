
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include "nvmem.h"
#include "nvmTypes_MDUV3x0.h"
#include "extFlash_MDx.h"

int main()
{
    nvm_init();

    while(1)
    {
        getchar();

        uint8_t i;
        mduv3x0Calib_t cal;
        nvm_readCalibData(&cal);

        printf("vox1: %03d\r\n",            cal.vox1);
        printf("vox10: %03d\r\n",           cal.vox10);
        printf("rxLowVoltage: %03d\r\n",    cal.rxLowVoltage);
        printf("rxFullVoltage: %03d\r\n",   cal.rxFullVoltage);
        printf("rssi1: %03d\r\n",           cal.rssi1);
        printf("rssi4: %03d\r\n",           cal.rssi4);

        puts("\r\nUHF band:\r");
        printf("freqAdjustMid: %03d\r\n",   cal.uhfCal.freqAdjustMid);
        printf("\r\nrxFreq: ");             for(i = 0; i < 9; i++) printf("%d ", cal.uhfCal.rxFreq[i]);
        printf("\r\ntxFreq: ");             for(i = 0; i < 9; i++) printf("%d ", cal.uhfCal.txFreq[i]);
        printf("\r\ntxHighPower: ");        for(i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.txHighPower[i]);
        printf("\r\ntxMidPower: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.txMidPower[i]);
        printf("\r\ntxLowPower: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.txLowPower[i]);
        printf("\r\nrxSensitivity: ");      for(i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.rxSensitivity[i]);
        printf("\r\nopenSql9: ");           for(i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.openSql9[i]);
        printf("\r\ncloseSql9: ");          for(i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.closeSql9[i]);
        printf("\r\nopenSql1: ");           for(i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.openSql1[i]);
        printf("\r\ncloseSql1: ");          for(i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.closeSql1[i]);
        printf("\r\nctcss67Hz: ");          for(i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.ctcss67Hz[i]);
        printf("\r\nctcss151Hz: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.ctcss151Hz[i]);
        printf("\r\nctcss254Hz: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.ctcss254Hz[i]);
        printf("\r\ndcsMod1: ");            for(i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.dcsMod1[i]);
        printf("\r\nsendIrange: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.sendIrange[i]);
        printf("\r\nsendQrange: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.sendQrange[i]);
        printf("\r\nanalogSendIrange: ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.analogSendIrange[i]);
        printf("\r\nanalogSendQrange: ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhfCal.analogSendQrange[i]);

        puts("\r\nVHF band:\r");
        printf("freqAdjustMid: %03d\r\n",   cal.vhfCal.freqAdjustMid);
        printf("\r\nrxFreq: ");             for(i = 0; i < 9; i++) printf("%d ", cal.vhfCal.rxFreq[i]);
        printf("\r\ntxFreq: ");             for(i = 0; i < 9; i++) printf("%d ", cal.vhfCal.txFreq[i]);
        printf("\r\ntxHighPower: ");        for(i = 0; i < 9; i++) printf("%03d ", cal.vhfCal.txHighPower[i]);
        printf("\r\ntxMidPower: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.vhfCal.txMidPower[i]);
        printf("\r\ntxLowPower: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.vhfCal.txLowPower[i]);
        printf("\r\nrxSensitivity: ");      for(i = 0; i < 9; i++) printf("%03d ", cal.vhfCal.rxSensitivity[i]);
        printf("\r\nopenSql9: ");           for(i = 0; i < 9; i++) printf("%03d ", cal.vhfCal.openSql9[i]);
        printf("\r\ncloseSql9: ");          for(i = 0; i < 9; i++) printf("%03d ", cal.vhfCal.closeSql9[i]);
        printf("\r\nopenSql1: ");           for(i = 0; i < 9; i++) printf("%03d ", cal.vhfCal.openSql1[i]);
        printf("\r\ncloseSql1: ");          for(i = 0; i < 9; i++) printf("%03d ", cal.vhfCal.closeSql1[i]);
        printf("\r\nctcss67Hz: ");          for(i = 0; i < 9; i++) printf("%03d ", cal.vhfCal.ctcss67Hz[i]);
        printf("\r\nctcss151Hz: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.vhfCal.ctcss151Hz[i]);
        printf("\r\nctcss254Hz: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.vhfCal.ctcss254Hz[i]);
        printf("\r\ndcsMod1: ");            for(i = 0; i < 9; i++) printf("%03d ", cal.vhfCal.dcsMod1[i]);
        printf("\r\nsendIrange: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.vhfCal.sendIrange[i]);
        printf("\r\nsendQrange: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.vhfCal.sendQrange[i]);
        printf("\r\nanalogSendIrange: ");   for(i = 0; i < 9; i++) printf("%03d ", cal.vhfCal.analogSendIrange[i]);
        printf("\r\nanalogSendQrange: ");   for(i = 0; i < 9; i++) printf("%03d ", cal.vhfCal.analogSendQrange[i]);
    }

    return 0;
}

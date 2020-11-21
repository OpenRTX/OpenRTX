
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include "nvmem.h"
#include "nvmTypes_MD3x0.h"
#include "extFlash_MDx.h"

int main()
{
    nvm_init();

    while(1)
    {
        getchar();

        uint8_t i;
        md3x0Calib_t cal;
        nvm_readCalibData(&cal);

        printf("vox1: %03d\r\n",            cal.vox1);
        printf("vox10: %03d\r\n",           cal.vox10);
        printf("rxLowVoltage: %03d\r\n",    cal.rxLowVoltage);
        printf("rxFullVoltage: %03d\r\n",   cal.rxFullVoltage);
        printf("rssi1: %03d\r\n",           cal.rssi1);
        printf("rssi4: %03d\r\n",           cal.rssi4);
        printf("analogMic: %03d\r\n",       cal.analogMic);
        printf("digitalMic: %03d\r\n",      cal.digitalMic);
        printf("freqAdjustHigh: %03d\r\n",  cal.freqAdjustHigh);
        printf("freqAdjustMid: %03d\r\n",   cal.freqAdjustMid);
        printf("freqAdjustLow: %03d",       cal.freqAdjustLow);

        printf("\r\n\r\nrxFreq: ");         for(i = 0; i < 9; i++) printf("%d ", cal.rxFreq[i]);
        printf("\r\ntxFreq: ");             for(i = 0; i < 9; i++) printf("%d ", cal.txFreq[i]);
        printf("\r\ntxHighPower: ");        for(i = 0; i < 9; i++) printf("%03d ", cal.txHighPower[i]);
        printf("\r\ntxLowPower: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.txLowPower[i]);
        printf("\r\nrxSensitivity: ");      for(i = 0; i < 9; i++) printf("%03d ", cal.rxSensitivity[i]);
        printf("\r\nopenSql9: ");           for(i = 0; i < 9; i++) printf("%03d ", cal.openSql9[i]);
        printf("\r\ncloseSql9: ");          for(i = 0; i < 9; i++) printf("%03d ", cal.closeSql9[i]);
        printf("\r\nopenSql1: ");           for(i = 0; i < 9; i++) printf("%03d ", cal.openSql1[i]);
        printf("\r\ncloseSql1: ");          for(i = 0; i < 9; i++) printf("%03d ", cal.closeSql1[i]);
        printf("\r\nmaxVolume: ");          for(i = 0; i < 9; i++) printf("%03d ", cal.maxVolume[i]);
        printf("\r\nctcss67Hz: ");          for(i = 0; i < 9; i++) printf("%03d ", cal.ctcss67Hz[i]);
        printf("\r\nctcss151Hz: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.ctcss151Hz[i]);
        printf("\r\nctcss254Hz: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.ctcss254Hz[i]);
        printf("\r\ndcsMod2: ");            for(i = 0; i < 9; i++) printf("%03d ", cal.dcsMod2[i]);
        printf("\r\ndcsMod1: ");            for(i = 0; i < 9; i++) printf("%03d ", cal.dcsMod1[i]);
        printf("\r\nmod1Partial: ");        for(i = 0; i < 9; i++) printf("%03d ", cal.mod1Partial[i]);
        printf("\r\nanalogVoiceAdjust: ");  for(i = 0; i < 9; i++) printf("%03d ", cal.analogVoiceAdjust[i]);
        printf("\r\nlockVoltagePartial: "); for(i = 0; i < 9; i++) printf("%03d ", cal.lockVoltagePartial[i]);
        printf("\r\nsendIpartial: ");       for(i = 0; i < 9; i++) printf("%03d ", cal.sendIpartial[i]);
        printf("\r\nsendQpartial: ");       for(i = 0; i < 9; i++) printf("%03d ", cal.sendQpartial[i]);
        printf("\r\nsendIrange: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.sendIrange[i]);
        printf("\r\nsendQrange: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.sendQrange[i]);
        printf("\r\nrxIpartial: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.rxIpartial[i]);
        printf("\r\nrxQpartial: ");         for(i = 0; i < 9; i++) printf("%03d ", cal.rxQpartial[i]);
        printf("\r\nanalogSendIrange: ");   for(i = 0; i < 9; i++) printf("%03d ", cal.analogSendIrange[i]);
        printf("\r\nanalogSendQrange: ");   for(i = 0; i < 9; i++) printf("%03d ", cal.analogSendQrange[i]);
    }

    return 0;
}

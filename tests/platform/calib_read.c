/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include "interfaces/nvmem.h"
#include "interfaces/platform.h"
#include "interfaces/delays.h"
#include "calibration/calibInfo_GDx.h"
#include "core/nvmem_access.h"
#include "core/utils.h"

struct md3x0FullCalib {
   // 0x0000
   uint8_t vox1;
   uint8_t vox10;
   uint8_t rxLowVoltage;
   uint8_t rxFullVoltage;
   uint8_t rssi1;
   uint8_t rssi4;
   uint8_t analogMic;
   uint8_t digitalMic;
   uint8_t freqAdjustHigh;
   uint8_t freqAdjustMid;
   uint8_t freqAdjustLow;
   uint8_t __gap1[5];

   // 0x0010
   uint8_t txHighPower[9];
   uint8_t __gap2[7];

   // 0x0020
   uint8_t txLowPower[9];
   uint8_t __gap3[7];

   // 0x0030
   uint8_t rxSensitivity[9];
   uint8_t __gap4[7];

   // 0x0040
   uint8_t openSql9[9];
   uint8_t __gap5[7];

   // 0x0050
   uint8_t closeSql9[9];
   uint8_t __gap6[7];

   // 0x0060
   uint8_t openSql1[9];
   uint8_t __gap7[7];

   // 0x0070
   uint8_t closeSql1[9];
   uint8_t __gap8[7];

   // 0x0080
   uint8_t maxVolume[9];
   uint8_t __gap9[7];

   // 0x0090
   uint8_t ctcss67Hz[9];
   uint8_t __gap10[7];

   // 0x00a0
   uint8_t ctcss151Hz[9];
   uint8_t __gap11[7];

   // 0x00b0
   uint8_t ctcss254Hz[9];
   uint8_t __gap12[7];

   // 0x00c0
   uint8_t dcsMod2[9];
   uint8_t __gap13[7];

   // 0x00d0
   uint8_t dcsMod1[9];
   uint8_t __gap14[7];

   // 0x00e0
   uint8_t mod1Partial[9];
   uint8_t __gap15[7];

   // 0x00f0
   uint8_t analogVoiceAdjust[9];
   uint8_t __gap16[7];

   // 0x0100
   uint8_t lockVoltagePartial[9];
   uint8_t __gap17[7];

   // 0x0110
   uint8_t sendIpartial[9];
   uint8_t __gap18[7];

   // 0x0120
   uint8_t sendQpartial[9];
   uint8_t __gap19[7];

   // 0x0130
   uint8_t sendIrange[9];
   uint8_t __gap20[7];

   // 0x0140
   uint8_t sendQrange[9];
   uint8_t __gap21[7];

   // 0x0150
   uint8_t rxIpartial[9];
   uint8_t __gap22[7];

   // 0x0160
   uint8_t rxQpartial[9];
   uint8_t __gap23[7];

   // 0x0170
   uint8_t analogSendIrange[9];
   uint8_t __gap24[7];

   // 0x0180
   uint8_t analogSendQrange[9];
   uint8_t __gap25[7];

   // 0x0190
   uint8_t __gap26[32];

   // 0x01b0
   uint32_t freq[18];
   uint8_t __gap27[7];
};

struct mduv3x0FullCalib {
    // 0x0000
    uint8_t vox1;
    uint8_t vox10;
    uint8_t rxLowVoltage;
    uint8_t rxFullVoltage;
    uint8_t rssi1;
    uint8_t rssi4;
    uint8_t analogMic;
    uint8_t digitalMic;
    uint8_t uhf_freqAdjustHigh;
    uint8_t uhf_freqAdjustMid;
    uint8_t uhf_freqAdjustLow;
    uint8_t vhf_freqAdjustHigh;
    uint8_t vhf_freqAdjustMid;
    uint8_t vhf_freqAdjustLow;
    uint8_t __gap1[2];

    // 0x0010
    uint8_t uhf_txHighPower[9];
    uint8_t vhf_txHighPower[5];
    uint8_t __gap2[2];

    // 0x0020
    uint8_t uhf_txLowPower[9];
    uint8_t vhf_txLowPower[5];
    uint8_t __gap3[2];

    // 0x0030
    uint8_t uhf_rxSensitivity[9];
    uint8_t vhf_rxSensitivity[5];
    uint8_t __gap4[2];

    // 0x0040
    uint8_t uhf_openSql9[9];
    uint8_t __gap5[7];

    // 0x0050
    uint8_t uhf_closeSql9[9];
    uint8_t __gap6[7];

    // 0x0070
    uint8_t uhf_closeSql1[9];
    uint8_t __gap7[7];

    // 0x0060
    uint8_t uhf_openSql1[9];
    uint8_t __gap8[7];

    // 0x0090
    uint8_t uhf_ctcss67Hz[9];
    uint8_t __gap9[2];
    uint8_t vhf_ctcss67Hz[5];

    // 0x00a0
    uint8_t uhf_ctcss151Hz[9];
    uint8_t __gap10[2];
    uint8_t vhf_ctcss151Hz[5];

    // 0x00b0
    uint8_t uhf_ctcss254Hz[9];
    uint8_t __gap11[2];
    uint8_t vhf_ctcss254Hz[5];

    // 0x00d0
    uint8_t uhf_dcsMod1[9];
    uint8_t __gap12[2];
    uint8_t vhf_dcsMod1[5];

    // 0x00e0
    uint8_t vhf_openSql9[5];
    uint8_t vhf_closeSql9[5];
    uint8_t vhf_closeSql1[5];
    uint8_t vhf_openSql1[5];
    uint8_t __gap13[12];

    // 0x0100
    uint32_t vhf_freq[10];
    uint8_t __gap14[7];

    // 0x0130
    uint8_t uhf_sendIrange[9];
    uint8_t vhf_sendIrange[5];
    uint8_t __gap15[2];

    // 0x0140
    uint8_t uhf_sendQrange[9];
    uint8_t vhf_sendQrange[5];
    uint8_t __gap16[2];

    // 0x0150
    uint8_t __gap17[32];

    // 0x0170
    uint8_t uhf_analogSendIrange[9];
    uint8_t vhf_analogSendIrange[5];
    uint8_t __gap18[2];

    // 0x0180
    uint8_t uhf_analogSendQrange[9];
    uint8_t vhf_analogSendQrange[5];
    uint8_t __gap19[2];

    // 0x0190
    uint8_t uhf_txMidPower[9];
    uint8_t vhf_txMidPower[5];
    uint8_t __gap20[2];

    // 0x01a0
    uint8_t __gap21[16];

    // 0x01b0
    uint32_t uhf_freq[18];
    uint8_t __gap22[7];
};

void printCalibration_gdx()
{
    uint8_t i;
    gdxCalibration_t cal;
    nvm_readCalibData(&cal);

    puts("\r\nUHF band:\r");
    printf("modBias                %03d",    cal.data[1].modBias);
    printf("\r\nmod2Offset         %03d",    cal.data[1].mod2Offset);
    printf("\r\ntxHighPower:       ");       for(i = 0; i < 16; i++) printf("%03d ", cal.data[1].txHighPower[i]);
    printf("\r\ntxLowPower:        ");       for(i = 0; i < 16; i++) printf("%03d ", cal.data[1].txLowPower[i]);
    printf("\r\nanalogSqlThresh:   ");       for(i = 0; i < 8; i++)  printf("%03d ", cal.data[1].analogSqlThresh[i]);
    printf("\r\nnoise1_HighTsh_Wb  %03d",    cal.data[1].noise1_HighTsh_Wb);
    printf("\r\nnoise1_LowTsh_Wb   %03d",    cal.data[1].noise1_LowTsh_Wb);
    printf("\r\nnoise2_HighTsh_Wb  %03d",    cal.data[1].noise2_HighTsh_Wb);
    printf("\r\nnoise2_LowTsh_Wb   %03d",    cal.data[1].noise2_LowTsh_Wb);
    printf("\r\nrssi_HighTsh_Wb    %03d",    cal.data[1].rssi_HighTsh_Wb);
    printf("\r\nrssi_LowTsh_Wb     %03d",    cal.data[1].rssi_LowTsh_Wb);
    printf("\r\nnoise1_HighTsh_Nb  %03d",    cal.data[1].noise1_HighTsh_Nb);
    printf("\r\nnoise1_LowTsh_Nb   %03d",    cal.data[1].noise1_LowTsh_Nb);
    printf("\r\nnoise2_HighTsh_Nb  %03d",    cal.data[1].noise2_HighTsh_Nb);
    printf("\r\nnoise2_LowTsh_Nb   %03d",    cal.data[1].noise2_LowTsh_Nb);
    printf("\r\nrssi_HighTsh_Nb    %03d",    cal.data[1].rssi_HighTsh_Nb);
    printf("\r\nrssi_LowTsh_Nb     %03d",    cal.data[1].rssi_LowTsh_Nb);
    printf("\r\nRSSILowerThreshold %03d",    cal.data[1].RSSILowerThreshold);
    printf("\r\nRSSIUpperThreshold %03d",    cal.data[1].RSSIUpperThreshold);
    printf("\r\nmod1Amplitude:     ");       for(i = 0; i < 8; i++) printf("%03d ", cal.data[1].mod1Amplitude[i]);
    printf("\r\ntxDev_DTMF         %03d",    cal.data[1].txDev_DTMF);
    printf("\r\ntxDev_tone         %03d",    cal.data[1].txDev_tone);
    printf("\r\ntxDev_CTCSS_wb     %03d",    cal.data[1].txDev_CTCSS_wb);
    printf("\r\ntxDev_CTCSS_nb     %03d",    cal.data[1].txDev_CTCSS_nb);
    printf("\r\ntxDev_DCS_wb       %03d",    cal.data[1].txDev_DCS_wb);
    printf("\r\ntxDev_DCS_nb       %03d",    cal.data[1].txDev_DCS_nb);
    printf("\r\nPA_drv             %03d",    cal.data[1].PA_drv);
    printf("\r\nPGA_gain           %03d",    cal.data[1].PGA_gain);
    printf("\r\nanalogMicGain      %03d",    cal.data[1].analogMicGain);
    printf("\r\nrxAGCgain          %03d",    cal.data[1].rxAGCgain);
    printf("\r\nmixGainWideband    %03d",    cal.data[1].mixGainWideband);
    printf("\r\nmixGainNarrowband  %03d",    cal.data[1].mixGainNarrowband);
    printf("\r\nmixGainWideband    %03d",    cal.data[1].mixGainWideband);
    printf("\r\nmixGainNarrowband  %03d",    cal.data[1].mixGainNarrowband);
    printf("\r\nuhfPwrCalPoints:   ");       for(i = 0; i < 16; i++) printf("%ld ", cal.uhfPwrCalPoints[i]);
    printf("\r\nuhfCalPoints:      ");       for(i = 0; i < 8; i++)  printf("%ld ", cal.uhfCalPoints[i]);

    puts("\r\nVHF band:\r");
    printf("modBias                %03d",    cal.data[0].modBias);
    printf("\r\nmod2Offset         %03d",    cal.data[0].mod2Offset);
    printf("\r\ntxHighPower:       ");       for(i = 0; i < 8; i++) printf("%03d ", cal.data[0].txHighPower[i]);
    printf("\r\ntxLowPower:        ");       for(i = 0; i < 8; i++) printf("%03d ", cal.data[0].txLowPower[i]);
    printf("\r\nanalogSqlThresh:   ");       for(i = 0; i < 8; i++) printf("%03d ", cal.data[0].analogSqlThresh[i]);
    printf("\r\nnoise1_HighTsh_Wb  %03d",    cal.data[0].noise1_HighTsh_Wb);
    printf("\r\nnoise1_LowTsh_Wb   %03d",    cal.data[0].noise1_LowTsh_Wb);
    printf("\r\nnoise2_HighTsh_Wb  %03d",    cal.data[0].noise2_HighTsh_Wb);
    printf("\r\nnoise2_LowTsh_Wb   %03d",    cal.data[0].noise2_LowTsh_Wb);
    printf("\r\nrssi_HighTsh_Wb    %03d",    cal.data[0].rssi_HighTsh_Wb);
    printf("\r\nrssi_LowTsh_Wb     %03d",    cal.data[0].rssi_LowTsh_Wb);
    printf("\r\nnoise1_HighTsh_Nb  %03d",    cal.data[0].noise1_HighTsh_Nb);
    printf("\r\nnoise1_LowTsh_Nb   %03d",    cal.data[0].noise1_LowTsh_Nb);
    printf("\r\nnoise2_HighTsh_Nb  %03d",    cal.data[0].noise2_HighTsh_Nb);
    printf("\r\nnoise2_LowTsh_Nb   %03d",    cal.data[0].noise2_LowTsh_Nb);
    printf("\r\nrssi_HighTsh_Nb    %03d",    cal.data[0].rssi_HighTsh_Nb);
    printf("\r\nrssi_LowTsh_Nb     %03d",    cal.data[0].rssi_LowTsh_Nb);
    printf("\r\nRSSILowerThreshold %03d",    cal.data[0].RSSILowerThreshold);
    printf("\r\nRSSIUpperThreshold %03d",    cal.data[0].RSSIUpperThreshold);
    printf("\r\nmod1Amplitude:     ");       for(i = 0; i < 8; i++) printf("%03d ", cal.data[0].mod1Amplitude[i]);
    printf("\r\ntxDev_DTMF         %03d",    cal.data[0].txDev_DTMF);
    printf("\r\ntxDev_tone         %03d",    cal.data[0].txDev_tone);
    printf("\r\ntxDev_CTCSS_wb     %03d",    cal.data[0].txDev_CTCSS_wb);
    printf("\r\ntxDev_CTCSS_nb     %03d",    cal.data[0].txDev_CTCSS_nb);
    printf("\r\ntxDev_DCS_wb       %03d",    cal.data[0].txDev_DCS_wb);
    printf("\r\ntxDev_DCS_nb       %03d",    cal.data[0].txDev_DCS_nb);
    printf("\r\nPA_drv             %03d",    cal.data[0].PA_drv);
    printf("\r\nPGA_gain           %03d",    cal.data[0].PGA_gain);
    printf("\r\nanalogMicGain      %03d",    cal.data[0].analogMicGain);
    printf("\r\nrxAGCgain          %03d",    cal.data[0].rxAGCgain);
    printf("\r\nmixGainWideband    %03d",    cal.data[0].mixGainWideband);
    printf("\r\nmixGainNarrowband  %03d",    cal.data[0].mixGainNarrowband);
    printf("\r\nmixGainWideband    %03d",    cal.data[0].mixGainWideband);
    printf("\r\nmixGainNarrowband  %03d",    cal.data[0].mixGainNarrowband);
    printf("\r\nvhfCalPoints:      ");       for(i = 0; i < 8; i++) printf("%ld ", cal.vhfCalPoints[i]);
}

void printCalibration_md3x0()
{
    uint8_t i;
    struct md3x0FullCalib cal;
    uint8_t *rawData = (uint8_t *) &cal;

    nvm_read(1, -1, 0x0000, rawData, 256);          // W25Qx security register 0x0000
    nvm_read(2, -1, 0x0000, rawData + 256, 256);    // W25Qx security register 0x0100

    printf("vox1:           %03d\r\n",  cal.vox1);
    printf("vox10:          %03d\r\n",  cal.vox10);
    printf("rxLowVoltage:   %03d\r\n",  cal.rxLowVoltage);
    printf("rxFullVoltage:  %03d\r\n",  cal.rxFullVoltage);
    printf("rssi1:          %03d\r\n",  cal.rssi1);
    printf("rssi4:          %03d\r\n",  cal.rssi4);
    printf("analogMic:      %03d\r\n",  cal.analogMic);
    printf("digitalMic:     %03d\r\n",  cal.digitalMic);
    printf("freqAdjustHigh: %03d\r\n",  cal.freqAdjustHigh);
    printf("freqAdjustMid:  %03d\r\n",  cal.freqAdjustMid);
    printf("freqAdjustLow:  %03d\r\n",  cal.freqAdjustLow);

    printf("\r\ntxHighPower:        "); for(i = 0; i < 9; i++) printf("%03d ", cal.txHighPower[i]);
    printf("\r\ntxLowPower:         "); for(i = 0; i < 9; i++) printf("%03d ", cal.txLowPower[i]);
    printf("\r\nrxSensitivity:      "); for(i = 0; i < 9; i++) printf("%03d ", cal.rxSensitivity[i]);
    printf("\r\nopenSql9:           "); for(i = 0; i < 9; i++) printf("%03d ", cal.openSql9[i]);
    printf("\r\ncloseSql9:          "); for(i = 0; i < 9; i++) printf("%03d ", cal.closeSql9[i]);
    printf("\r\nopenSql1:           "); for(i = 0; i < 9; i++) printf("%03d ", cal.openSql1[i]);
    printf("\r\ncloseSql1:          "); for(i = 0; i < 9; i++) printf("%03d ", cal.closeSql1[i]);
    printf("\r\nmaxVolume:          "); for(i = 0; i < 9; i++) printf("%03d ", cal.maxVolume[i]);
    printf("\r\nctcss67Hz:          "); for(i = 0; i < 9; i++) printf("%03d ", cal.ctcss67Hz[i]);
    printf("\r\nctcss151Hz:         "); for(i = 0; i < 9; i++) printf("%03d ", cal.ctcss151Hz[i]);
    printf("\r\nctcss254Hz:         "); for(i = 0; i < 9; i++) printf("%03d ", cal.ctcss254Hz[i]);
    printf("\r\ndcsMod2:            "); for(i = 0; i < 9; i++) printf("%03d ", cal.dcsMod2[i]);
    printf("\r\ndcsMod1:            "); for(i = 0; i < 9; i++) printf("%03d ", cal.dcsMod1[i]);
    printf("\r\nmod1Partial:        "); for(i = 0; i < 9; i++) printf("%03d ", cal.mod1Partial[i]);
    printf("\r\nanalogVoiceAdjust:  "); for(i = 0; i < 9; i++) printf("%03d ", cal.analogVoiceAdjust[i]);
    printf("\r\nlockVoltagePartial: "); for(i = 0; i < 9; i++) printf("%03d ", cal.lockVoltagePartial[i]);
    printf("\r\nsendIpartial:       "); for(i = 0; i < 9; i++) printf("%03d ", cal.sendIpartial[i]);
    printf("\r\nsendQpartial:       "); for(i = 0; i < 9; i++) printf("%03d ", cal.sendQpartial[i]);
    printf("\r\nsendIrange:         "); for(i = 0; i < 9; i++) printf("%03d ", cal.sendIrange[i]);
    printf("\r\nsendQrange:         "); for(i = 0; i < 9; i++) printf("%03d ", cal.sendQrange[i]);
    printf("\r\nrxIpartial:         "); for(i = 0; i < 9; i++) printf("%03d ", cal.rxIpartial[i]);
    printf("\r\nrxQpartial:         "); for(i = 0; i < 9; i++) printf("%03d ", cal.rxQpartial[i]);
    printf("\r\nanalogSendIrange:   "); for(i = 0; i < 9; i++) printf("%03d ", cal.analogSendIrange[i]);
    printf("\r\nanalogSendQrange:   "); for(i = 0; i < 9; i++) printf("%03d ", cal.analogSendQrange[i]);

    printf("\r\n\r\nrxFreq: ");         for(i = 0; i < 9; i++) printf("%ld ", bcdToBin(cal.freq[2*i])*10);
    printf("\r\ntxFreq:     ");         for(i = 0; i < 9; i++) printf("%ld ", bcdToBin(cal.freq[2*i+1])*10);
}

void printCalibration_mduv3x0()
{
    uint8_t i;
    struct mduv3x0FullCalib cal;
    uint8_t *rawData = (uint8_t *) &cal;

    nvm_read(1, -1, 0x0000, rawData, 256);          // W25Qx security register 0x0000
    nvm_read(2, -1, 0x0000, rawData + 256, 256);    // W25Qx security register 0x0100

    printf("vox1:           %03d\r\n",  cal.vox1);
    printf("vox10:          %03d\r\n",  cal.vox10);
    printf("rxLowVoltage:   %03d\r\n",  cal.rxLowVoltage);
    printf("rxFullVoltage:  %03d\r\n",  cal.rxFullVoltage);
    printf("rssi1:          %03d\r\n",  cal.rssi1);
    printf("rssi4:          %03d\r\n",  cal.rssi4);
    printf("analogMic:      %03d\r\n",  cal.analogMic);
    printf("digitalMic:     %03d\r\n",  cal.digitalMic);

    puts("\r\nUHF band:\r");
    printf("freqAdjustMid:        %03d\r\n", cal.uhf_freqAdjustMid);
    printf("\r\ntxHighPower:      ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhf_txHighPower[i]);
    printf("\r\ntxMidPower:       ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhf_txMidPower[i]);
    printf("\r\ntxLowPower:       ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhf_txLowPower[i]);
    printf("\r\nrxSensitivity:    ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhf_rxSensitivity[i]);
    printf("\r\nopenSql9:         ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhf_openSql9[i]);
    printf("\r\ncloseSql9:        ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhf_closeSql9[i]);
    printf("\r\nopenSql1:         ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhf_openSql1[i]);
    printf("\r\ncloseSql1:        ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhf_closeSql1[i]);
    printf("\r\nctcss67Hz:        ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhf_ctcss67Hz[i]);
    printf("\r\nctcss151Hz:       ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhf_ctcss151Hz[i]);
    printf("\r\nctcss254Hz:       ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhf_ctcss254Hz[i]);
    printf("\r\ndcsMod1:          ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhf_dcsMod1[i]);
    printf("\r\nsendIrange:       ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhf_sendIrange[i]);
    printf("\r\nsendQrange:       ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhf_sendQrange[i]);
    printf("\r\nanalogSendIrange: ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhf_analogSendIrange[i]);
    printf("\r\nanalogSendQrange: ");   for(i = 0; i < 9; i++) printf("%03d ", cal.uhf_analogSendQrange[i]);
    printf("\r\nrxFreq:           ");   for(i = 0; i < 9; i++) printf("%ld ",  bcdToBin(cal.uhf_freq[2*i])*10);
    printf("\r\ntxFreq:           ");   for(i = 0; i < 9; i++) printf("%ld ",  bcdToBin(cal.uhf_freq[2*i+1])*10);

    puts("\r\nVHF band:\r");
    printf("freqAdjustMid:        %03d\r\n", cal.vhf_freqAdjustMid);
    printf("\r\ntxHighPower:      ");   for(i = 0; i < 5; i++) printf("%03d ", cal.vhf_txHighPower[i]);
    printf("\r\ntxMidPower:       ");   for(i = 0; i < 5; i++) printf("%03d ", cal.vhf_txMidPower[i]);
    printf("\r\ntxLowPower:       ");   for(i = 0; i < 5; i++) printf("%03d ", cal.vhf_txLowPower[i]);
    printf("\r\nrxSensitivity:    ");   for(i = 0; i < 5; i++) printf("%03d ", cal.vhf_rxSensitivity[i]);
    printf("\r\nopenSql9:         ");   for(i = 0; i < 5; i++) printf("%03d ", cal.vhf_openSql9[i]);
    printf("\r\ncloseSql9:        ");   for(i = 0; i < 5; i++) printf("%03d ", cal.vhf_closeSql9[i]);
    printf("\r\nopenSql1:         ");   for(i = 0; i < 5; i++) printf("%03d ", cal.vhf_openSql1[i]);
    printf("\r\ncloseSql1:        ");   for(i = 0; i < 5; i++) printf("%03d ", cal.vhf_closeSql1[i]);
    printf("\r\nctcss67Hz:        ");   for(i = 0; i < 5; i++) printf("%03d ", cal.vhf_ctcss67Hz[i]);
    printf("\r\nctcss151Hz:       ");   for(i = 0; i < 5; i++) printf("%03d ", cal.vhf_ctcss151Hz[i]);
    printf("\r\nctcss254Hz:       ");   for(i = 0; i < 5; i++) printf("%03d ", cal.vhf_ctcss254Hz[i]);
    printf("\r\ndcsMod1:          ");   for(i = 0; i < 5; i++) printf("%03d ", cal.vhf_dcsMod1[i]);
    printf("\r\nsendIrange:       ");   for(i = 0; i < 5; i++) printf("%03d ", cal.vhf_sendIrange[i]);
    printf("\r\nsendQrange:       ");   for(i = 0; i < 5; i++) printf("%03d ", cal.vhf_sendQrange[i]);
    printf("\r\nanalogSendIrange: ");   for(i = 0; i < 5; i++) printf("%03d ", cal.vhf_analogSendIrange[i]);
    printf("\r\nanalogSendQrange: ");   for(i = 0; i < 5; i++) printf("%03d ", cal.vhf_analogSendQrange[i]);
    printf("\r\nrxFreq:           ");   for(i = 0; i < 5; i++) printf("%ld ",  bcdToBin(cal.vhf_freq[2*i])*10);
    printf("\r\ntxFreq:           ");   for(i = 0; i < 5; i++) printf("%ld ",  bcdToBin(cal.vhf_freq[2*i+1])*10);
}

static void waitForPtt()
{
    // Wait until PTT is pressed
    while(platform_getPttStatus() == false) {
        platform_ledOn(GREEN);
        sleepFor(0, 500);
        platform_ledOff(GREEN);
        sleepFor(0, 500);
    }

    // Resume execution on PTT release
    platform_ledOn(RED);
    while(platform_getPttStatus() == true) ;
    platform_ledOff(RED);
}

int main()
{
    platform_init();
    nvm_init();

    while(1) {
        waitForPtt();

#if defined(PLATFORM_MD3x0)
        printCalibration_md3x0();
#elif defined(PLATFORM_MDUV3x0)
        printCalibration_mduv3x0();
#elif defined(PLATFORM_GD77) || defined(PLATFORM_DM1801)
        printCalibration_gdx();
#else
#error "Device not supported"
#endif
    }

    return 0;
}

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

#include <calibInfo_GDx.h>
#include <nvmem.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

void printCalibration()
{
    uint8_t i;
    gdxCalibration_t cal;
    nvm_readCalibData(&cal);

    puts("\r\nUHF band:\r");
    printf("mod1Bias %03d", cal.uhfCal.mod1Bias);
    printf("\r\nmod2Offset %03d", cal.uhfCal.mod2Offset);
    printf("\r\ntxHighPower: ");
    for (i = 0; i < 16; i++) printf("%03d ", cal.uhfCal.txHighPower[i]);
    printf("\r\ntxLowPower: ");
    for (i = 0; i < 16; i++) printf("%03d ", cal.uhfCal.txLowPower[i]);
    printf("\r\nanalogSqlThresh: ");
    for (i = 0; i < 8; i++) printf("%03d ", cal.uhfCal.analogSqlThresh[i]);
    printf("\r\nnoise1_HighTsh_Wb %03d", cal.uhfCal.noise1_HighTsh_Wb);
    printf("\r\nnoise1_LowTsh_Wb  %03d", cal.uhfCal.noise1_LowTsh_Wb);
    printf("\r\nnoise2_HighTsh_Wb %03d", cal.uhfCal.noise2_HighTsh_Wb);
    printf("\r\nnoise2_LowTsh_Wb %03d", cal.uhfCal.noise2_LowTsh_Wb);
    printf("\r\nrssi_HighTsh_Wb %03d", cal.uhfCal.rssi_HighTsh_Wb);
    printf("\r\nrssi_LowTsh_Wb  %03d", cal.uhfCal.rssi_LowTsh_Wb);
    printf("\r\nnoise1_HighTsh_Nb %03d", cal.uhfCal.noise1_HighTsh_Nb);
    printf("\r\nnoise1_LowTsh_Nb %03d", cal.uhfCal.noise1_LowTsh_Nb);
    printf("\r\nnoise2_HighTsh_Nb %03d", cal.uhfCal.noise2_HighTsh_Nb);
    printf("\r\nnoise2_LowTsh_Nb %03d", cal.uhfCal.noise2_LowTsh_Nb);
    printf("\r\nrssi_HighTsh_Nb %03d", cal.uhfCal.rssi_HighTsh_Nb);
    printf("\r\nrssi_LowTsh_Nb %03d", cal.uhfCal.rssi_LowTsh_Nb);
    printf("\r\nRSSILowerThreshold %03d", cal.uhfCal.RSSILowerThreshold);
    printf("\r\nRSSIUpperThreshold %03d", cal.uhfCal.RSSIUpperThreshold);
    printf("\r\nmod1Amplitude: ");
    for (i = 0; i < 8; i++) printf("%03d ", cal.uhfCal.mod1Amplitude[i]);
    printf("\r\ndacDataRange %03d", cal.uhfCal.dacDataRange);
    printf("\r\ntxDev_DTMF %03d", cal.uhfCal.txDev_DTMF);
    printf("\r\ntxDev_tone %03d", cal.uhfCal.txDev_tone);
    printf("\r\ntxDev_CTCSS_wb %03d", cal.uhfCal.txDev_CTCSS_wb);
    printf("\r\ntxDev_CTCSS_nb %03d", cal.uhfCal.txDev_CTCSS_nb);
    printf("\r\ntxDev_DCS_wb %03d", cal.uhfCal.txDev_DCS_wb);
    printf("\r\ntxDev_DCS_nb %03d", cal.uhfCal.txDev_DCS_nb);
    printf("\r\nPA_drv %03d", cal.uhfCal.PA_drv);
    printf("\r\nPGA_gain %03d", cal.uhfCal.PGA_gain);
    printf("\r\nanalogMicGain %03d", cal.uhfCal.analogMicGain);
    printf("\r\nrxAGCgain %03d", cal.uhfCal.rxAGCgain);
    printf("\r\nmixGainWideband %03d", cal.uhfCal.mixGainWideband);
    printf("\r\nmixGainNarrowband %03d", cal.uhfCal.mixGainNarrowband);
    printf("\r\nrxAudioGainWideband %03d", cal.uhfCal.rxAudioGainWideband);
    printf("\r\nrxAudioGainNarrowband %03d", cal.uhfCal.rxAudioGainNarrowband);
    printf("\r\nuhfPwrCalPoints: ");
    for (i = 0; i < 16; i++) printf("%ld ", cal.uhfPwrCalPoints[i]);
    printf("\r\nuhfMod1CalPoints: ");
    for (i = 0; i < 8; i++) printf("%ld ", cal.uhfMod1CalPoints[i]);

    puts("\r\nVHF band:\r");
    printf("mod1Bias %03d", cal.vhfCal.mod1Bias);
    printf("\r\nmod2Offset %03d", cal.vhfCal.mod2Offset);
    printf("\r\ntxHighPower: ");
    for (i = 0; i < 8; i++) printf("%03d ", cal.vhfCal.txHighPower[i]);
    printf("\r\ntxLowPower: ");
    for (i = 0; i < 8; i++) printf("%03d ", cal.vhfCal.txLowPower[i]);
    printf("\r\nanalogSqlThresh: ");
    for (i = 0; i < 8; i++) printf("%03d ", cal.vhfCal.analogSqlThresh[i]);
    printf("\r\nnoise1_HighTsh_Wb %03d", cal.vhfCal.noise1_HighTsh_Wb);
    printf("\r\nnoise1_LowTsh_Wb  %03d", cal.vhfCal.noise1_LowTsh_Wb);
    printf("\r\nnoise2_HighTsh_Wb %03d", cal.vhfCal.noise2_HighTsh_Wb);
    printf("\r\nnoise2_LowTsh_Wb %03d", cal.vhfCal.noise2_LowTsh_Wb);
    printf("\r\nrssi_HighTsh_Wb %03d", cal.vhfCal.rssi_HighTsh_Wb);
    printf("\r\nrssi_LowTsh_Wb  %03d", cal.vhfCal.rssi_LowTsh_Wb);
    printf("\r\nnoise1_HighTsh_Nb %03d", cal.vhfCal.noise1_HighTsh_Nb);
    printf("\r\nnoise1_LowTsh_Nb %03d", cal.vhfCal.noise1_LowTsh_Nb);
    printf("\r\nnoise2_HighTsh_Nb %03d", cal.vhfCal.noise2_HighTsh_Nb);
    printf("\r\nnoise2_LowTsh_Nb %03d", cal.vhfCal.noise2_LowTsh_Nb);
    printf("\r\nrssi_HighTsh_Nb %03d", cal.vhfCal.rssi_HighTsh_Nb);
    printf("\r\nrssi_LowTsh_Nb %03d", cal.vhfCal.rssi_LowTsh_Nb);
    printf("\r\nRSSILowerThreshold %03d", cal.vhfCal.RSSILowerThreshold);
    printf("\r\nRSSIUpperThreshold %03d", cal.vhfCal.RSSIUpperThreshold);
    printf("\r\nmod1Amplitude: ");
    for (i = 0; i < 8; i++) printf("%03d ", cal.vhfCal.mod1Amplitude[i]);
    printf("\r\ndacDataRange %03d", cal.vhfCal.dacDataRange);
    printf("\r\ntxDev_DTMF %03d", cal.vhfCal.txDev_DTMF);
    printf("\r\ntxDev_tone %03d", cal.vhfCal.txDev_tone);
    printf("\r\ntxDev_CTCSS_wb %03d", cal.vhfCal.txDev_CTCSS_wb);
    printf("\r\ntxDev_CTCSS_nb %03d", cal.vhfCal.txDev_CTCSS_nb);
    printf("\r\ntxDev_DCS_wb %03d", cal.vhfCal.txDev_DCS_wb);
    printf("\r\ntxDev_DCS_nb %03d", cal.vhfCal.txDev_DCS_nb);
    printf("\r\nPA_drv %03d", cal.vhfCal.PA_drv);
    printf("\r\nPGA_gain %03d", cal.vhfCal.PGA_gain);
    printf("\r\nanalogMicGain %03d", cal.vhfCal.analogMicGain);
    printf("\r\nrxAGCgain %03d", cal.vhfCal.rxAGCgain);
    printf("\r\nmixGainWideband %03d", cal.vhfCal.mixGainWideband);
    printf("\r\nmixGainNarrowband %03d", cal.vhfCal.mixGainNarrowband);
    printf("\r\nrxAudioGainWideband %03d", cal.vhfCal.rxAudioGainWideband);
    printf("\r\nrxAudioGainNarrowband %03d", cal.vhfCal.rxAudioGainNarrowband);
    printf("\r\nvhfCalPoints: ");
    for (i = 0; i < 8; i++) printf("%ld ", cal.vhfCalPoints[i]);
}

int main()
{
    nvm_init();

    while (1)
    {
        getchar();
        printCalibration();
    }

    return 0;
}

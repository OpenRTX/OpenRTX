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
#include <calibInfo_GDx.h>
#include <interfaces/delays.h>


void printBandCalData(bandCalData_t *cal)
{
    uint8_t i;

    printf("\r\n_DigitalRxGainNarrowband %03d", cal->_DigitalRxGainNarrowband);
    printf("\r\n_DigitalTxGainNarrowband %03d", cal->_DigitalTxGainNarrowband);
    printf("\r\n_DigitalRxGainWideband %03d",   cal->_DigitalRxGainWideband);
    printf("\r\n_DigitalTxGainWideband %03d",   cal->_DigitalTxGainWideband);
    printf("\r\nmodBias %03d",                  cal->modBias);
    printf("\r\nmod2Offset %03d",               cal->mod2Offset);
    printf("\r\ntxLowPower: ");                 for(i = 0; i < 16; i++) printf("%03d ", cal->txLowPower[i]);
    printf("\r\ntxHighPower: ");                for(i = 0; i < 16; i++) printf("%03d ", cal->txHighPower[i]);
    printf("\r\nanalogSqlThresh: ");            for(i = 0; i < 8; i++)  printf("%03d ", cal->analogSqlThresh[i]);

    printf("\r\nnoise1_HighTsh_Wb %03d",        cal->noise1_HighTsh_Wb);
    printf("\r\nnoise1_LowTsh_Wb  %03d",        cal->noise1_LowTsh_Wb);
    printf("\r\nnoise2_HighTsh_Wb %03d",        cal->noise2_HighTsh_Wb);
    printf("\r\nnoise2_LowTsh_Wb %03d",         cal->noise2_LowTsh_Wb);
    printf("\r\nrssi_HighTsh_Wb %03d",          cal->rssi_HighTsh_Wb);
    printf("\r\nrssi_LowTsh_Wb  %03d",          cal->rssi_LowTsh_Wb);

    printf("\r\nnoise1_HighTsh_Nb %03d",        cal->noise1_HighTsh_Nb);
    printf("\r\nnoise1_LowTsh_Nb %03d",         cal->noise1_LowTsh_Nb);
    printf("\r\nnoise2_HighTsh_Nb %03d",        cal->noise2_HighTsh_Nb);
    printf("\r\nnoise2_LowTsh_Nb %03d",         cal->noise2_LowTsh_Nb);
    printf("\r\nrssi_HighTsh_Nb %03d",          cal->rssi_HighTsh_Nb);
    printf("\r\nrssi_LowTsh_Nb %03d",           cal->rssi_LowTsh_Nb);

    printf("\r\nRSSILowerThreshold %03d",       cal->RSSILowerThreshold);
    printf("\r\nRSSIUpperThreshold %03d",       cal->RSSIUpperThreshold);

    printf("\r\nmod1Amplitude: ");              for(i = 0; i < 8; i++)  printf("%03d ", cal->mod1Amplitude[i]);

    printf("\r\ndigAudioGain %03d",             cal->digAudioGain);

    printf("\r\ntxDev_DTMF %03d",               cal->txDev_DTMF);
    printf("\r\ntxDev_tone %03d",               cal->txDev_tone);
    printf("\r\ntxDev_CTCSS_wb %03d",           cal->txDev_CTCSS_wb);
    printf("\r\ntxDev_CTCSS_nb %03d",           cal->txDev_CTCSS_nb);
    printf("\r\ntxDev_DCS_wb %03d",             cal->txDev_DCS_wb);
    printf("\r\ntxDev_DCS_nb %03d",             cal->txDev_DCS_nb);

    printf("\r\nPA_drv %03d",                   cal->PA_drv);
    printf("\r\nPGA_gain %03d",                 cal->PGA_gain);
    printf("\r\nanalogMicGain %03d",            cal->analogMicGain);
    printf("\r\nrxAGCgain %03d",                cal->rxAGCgain);

    printf("\r\nmixGainWideband %03d",          cal->mixGainWideband);
    printf("\r\nmixGainNarrowband %03d",        cal->mixGainNarrowband);
    printf("\r\nrxDacGain %03d",                cal->rxDacGain);
    printf("\r\nrxVoiceGain %03d",              cal->rxVoiceGain);
}

void printCalibration()
{
    uint8_t i;
    gdxCalibration_t cal;
    nvm_readCalibData(&cal);

    puts("\r\nUHF band:\r");
    printBandCalData(&cal.data[1]);

    printf("\r\nuhfPwrCalPoints: ");     for(i = 0; i < 16; i++)  printf("%ld ", cal.uhfPwrCalPoints[i]);
    printf("\r\nuhfCalPoints: ");        for(i = 0; i < 8; i++)   printf("%ld ", cal.uhfCalPoints[i]);

    puts("\r\nVHF band:\r");
    printBandCalData(&cal.data[0]);
    printf("\r\nvhfCalPoints: ");        for(i = 0; i < 8; i++)   printf("%ld ", cal.vhfCalPoints[i]);
}

int main()
{
    nvm_init();

    while(1)
    {
        delayMs(5000);
        printCalibration();
    }

    return 0;
}

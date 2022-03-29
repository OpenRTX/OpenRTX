/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Amedeo Izzo IU2NUO,             *
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

#include <interfaces/nvmem.h>
#include <interfaces/delays.h>
#include <calibInfo_MDx.h>
#include <string.h>
#include <wchar.h>
#include <utils.h>
#include "W25Qx.h"

void nvm_init()
{
    W25Qx_init();
}

void nvm_terminate()
{
    W25Qx_terminate();
}

void nvm_readCalibData(void *buf)
{
    W25Qx_wakeup();
    delayUs(5);

    md3x0Calib_t *calib = ((md3x0Calib_t *) buf);

    (void) W25Qx_readSecurityRegister(0x1000, &(calib->vox1), 11);
    (void) W25Qx_readSecurityRegister(0x1010, calib->txHighPower, 9);
    (void) W25Qx_readSecurityRegister(0x1020, calib->txLowPower, 9);
    (void) W25Qx_readSecurityRegister(0x1030, calib->rxSensitivity, 9);
    (void) W25Qx_readSecurityRegister(0x1040, calib->openSql9, 9);
    (void) W25Qx_readSecurityRegister(0x1050, calib->closeSql9, 9);
    (void) W25Qx_readSecurityRegister(0x1060, calib->openSql1, 9);
    (void) W25Qx_readSecurityRegister(0x1070, calib->closeSql1, 9);
    (void) W25Qx_readSecurityRegister(0x1080, calib->maxVolume, 9);
    (void) W25Qx_readSecurityRegister(0x1090, calib->ctcss67Hz, 9);
    (void) W25Qx_readSecurityRegister(0x10a0, calib->ctcss151Hz, 9);
    (void) W25Qx_readSecurityRegister(0x10b0, calib->ctcss254Hz, 9);
    (void) W25Qx_readSecurityRegister(0x10c0, calib->dcsMod2, 9);
    (void) W25Qx_readSecurityRegister(0x10d0, calib->dcsMod1, 9);
    (void) W25Qx_readSecurityRegister(0x10e0, calib->mod1Partial, 9);
    (void) W25Qx_readSecurityRegister(0x10f0, calib->analogVoiceAdjust, 9);

    (void) W25Qx_readSecurityRegister(0x2000, calib->lockVoltagePartial, 9);
    (void) W25Qx_readSecurityRegister(0x2010, calib->sendIpartial, 9);
    (void) W25Qx_readSecurityRegister(0x2020, calib->sendQpartial, 9);
    (void) W25Qx_readSecurityRegister(0x2030, calib->sendIrange, 9);
    (void) W25Qx_readSecurityRegister(0x2040, calib->sendQrange, 9);
    (void) W25Qx_readSecurityRegister(0x2050, calib->rxIpartial, 9);
    (void) W25Qx_readSecurityRegister(0x2060, calib->rxQpartial, 9);
    (void) W25Qx_readSecurityRegister(0x2070, calib->analogSendIrange, 9);
    (void) W25Qx_readSecurityRegister(0x2080, calib->analogSendQrange, 9);

    uint32_t freqs[18];
    (void) W25Qx_readSecurityRegister(0x20b0, ((uint8_t *) &freqs), 72);
    W25Qx_sleep();

    /*
     * Ugly quirk: frequency stored in calibration data is divided by ten, so,
     * after bcd2bin conversion we have something like 40'135'000. To ajdust
     * things, frequency has to be multiplied by ten.
     */
    for(uint8_t i = 0; i < 9; i++)
    {
        calib->rxFreq[i] = ((freq_t) bcd2bin(freqs[2*i])) * 10;
        calib->txFreq[i] = ((freq_t) bcd2bin(freqs[2*i+1])) * 10;
    }
}

void nvm_loadHwInfo(hwInfo_t *info)
{
    uint16_t freqMin = 0;
    uint16_t freqMax = 0;
    uint8_t  lcdInfo = 0;

    /*
     * Hardware information data in MD3x0 devices is stored in security register
     * 0x3000.
     */
    W25Qx_wakeup();
    delayUs(5);

    (void) W25Qx_readSecurityRegister(0x3000, info->name, 8);
    (void) W25Qx_readSecurityRegister(0x3014, &freqMin, 2);
    (void) W25Qx_readSecurityRegister(0x3016, &freqMax, 2);
    (void) W25Qx_readSecurityRegister(0x301D, &lcdInfo, 1);
    W25Qx_sleep();

    /* Ensure correct null-termination of device name by removing the 0xff. */
    for(uint8_t i = 0; i < sizeof(info->name); i++)
    {
        if(info->name[i] == 0xFF) info->name[i] = '\0';
    }

    /* These devices are single-band only, either VHF or UHF. */
    freqMin = ((uint16_t) bcd2bin(freqMin))/10;
    freqMax = ((uint16_t) bcd2bin(freqMax))/10;

    if(freqMin < 200)
    {
        info->vhf_maxFreq = freqMax;
        info->vhf_minFreq = freqMin;
        info->vhf_band    = 1;
    }
    else
    {
        info->uhf_maxFreq = freqMax;
        info->uhf_minFreq = freqMin;
        info->uhf_band    = 1;
    }

    info->lcd_type = lcdInfo & 0x03;
}

/**
 * The MD380 stock CPS does not have a VFO channel slot
 * because the stock firmware does not have a VFO
 * To enable this functionality reserve a Flash portion for saving the VFO
 *
 * TODO: temporarily implemented in "nvmem_settings_MDx.c"

int nvm_readVFOChannelData(channel_t *channel)
{
    (void) channel;
    return -1;
}
*/

/*

TODO: temporarily implemented in "nvmem_settings_MDx.c"

int nvm_readSettings(settings_t *settings)
{
    (void) settings;
    return -1;
}
*/

int nvm_writeSettings(const settings_t *settings)
{
    (void) settings;
    return -1;
}

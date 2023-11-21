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

#include <wchar.h>
#include <string.h>
#include <interfaces/nvmem.h>
#include <interfaces/delays.h>
#include <calibInfo_MDx.h>
#include <utils.h>
#include "W25Qx.h"

W25Qx_DEVICE_DEFINE(W25Q128_main, W25Qx_api)
W25Qx_DEVICE_DEFINE(W25Q128_secr, W25Qx_secReg_api)

static const struct nvmArea areas[] =
{
    {
        .name       = "External flash",
        .dev        = &W25Q128_main,
        .startAddr  = 0x0000,
        .size       = 0x1000000,  // 16 MB, 128 Mbit
        .partitions = NULL
    },
    {
        .name       = "Cal. data 1",
        .dev        = &W25Q128_secr,
        .startAddr  = 0x1000,
        .size       = 0x100,      // 256 byte
        .partitions = NULL
    },
    {
        .name       = "Cal. data 2",
        .dev        = &W25Q128_secr,
        .startAddr  = 0x2000,
        .size       = 0x100,      // 256 byte
        .partitions = NULL
    }
};


void nvm_init()
{
    W25Qx_init();
}

void nvm_terminate()
{
    W25Qx_terminate();
}

size_t nvm_getMemoryAreas(const struct nvmArea **list)
{
    *list = &areas[0];

    return (sizeof(areas) / sizeof(struct nvmArea));
}

void nvm_readCalibData(void *buf)
{
    W25Qx_wakeup();
    delayUs(5);

    mduv3x0Calib_t *calib = ((mduv3x0Calib_t *) buf);

    /* Common calibration data */
    (void) W25Qx_readSecurityRegister(0x1000, (&calib->vox1), 6);

    /* UHF-band calibration data */
    (void) W25Qx_readSecurityRegister(0x1009, (&calib->uhfCal.freqAdjustMid), 1);
    (void) W25Qx_readSecurityRegister(0x1010, calib->uhfCal.txHighPower, 9);
    (void) W25Qx_readSecurityRegister(0x2090, calib->uhfCal.txMidPower, 9);
    (void) W25Qx_readSecurityRegister(0x1020, calib->uhfCal.txLowPower, 9);
    (void) W25Qx_readSecurityRegister(0x1030, calib->uhfCal.rxSensitivity, 9);
    (void) W25Qx_readSecurityRegister(0x1040, calib->uhfCal.openSql9, 9);
    (void) W25Qx_readSecurityRegister(0x1050, calib->uhfCal.closeSql9, 9);
    (void) W25Qx_readSecurityRegister(0x1070, calib->uhfCal.closeSql1, 9);
    (void) W25Qx_readSecurityRegister(0x1060, calib->uhfCal.openSql1, 9);
    (void) W25Qx_readSecurityRegister(0x1090, calib->uhfCal.ctcss67Hz, 9);
    (void) W25Qx_readSecurityRegister(0x10a0, calib->uhfCal.ctcss151Hz, 9);
    (void) W25Qx_readSecurityRegister(0x10b0, calib->uhfCal.ctcss254Hz, 9);
    (void) W25Qx_readSecurityRegister(0x10d0, calib->uhfCal.dcsMod1, 9);
    (void) W25Qx_readSecurityRegister(0x2030, calib->uhfCal.sendIrange, 9);
    (void) W25Qx_readSecurityRegister(0x2040, calib->uhfCal.sendQrange, 9);
    (void) W25Qx_readSecurityRegister(0x2070, calib->uhfCal.analogSendIrange, 9);
    (void) W25Qx_readSecurityRegister(0x2080, calib->uhfCal.analogSendQrange, 9);

    uint32_t freqs[18];
    (void) W25Qx_readSecurityRegister(0x20b0, ((uint8_t *) &freqs), 72);

    for(uint8_t i = 0; i < 9; i++)
    {
        calib->uhfCal.rxFreq[i] = ((freq_t) bcdToBin(freqs[2*i]));
        calib->uhfCal.txFreq[i] = ((freq_t) bcdToBin(freqs[2*i+1]));
    }

    /* VHF-band calibration data */
    (void) W25Qx_readSecurityRegister(0x100c, (&calib->vhfCal.freqAdjustMid), 1);
    (void) W25Qx_readSecurityRegister(0x1019, calib->vhfCal.txHighPower, 5);
    (void) W25Qx_readSecurityRegister(0x2099, calib->vhfCal.txMidPower, 5);
    (void) W25Qx_readSecurityRegister(0x1029, calib->vhfCal.txLowPower, 5);
    (void) W25Qx_readSecurityRegister(0x1039, calib->vhfCal.rxSensitivity, 5);
    (void) W25Qx_readSecurityRegister(0x109b, calib->vhfCal.ctcss67Hz, 5);
    (void) W25Qx_readSecurityRegister(0x10ab, calib->vhfCal.ctcss151Hz, 5);
    (void) W25Qx_readSecurityRegister(0x10bb, calib->vhfCal.ctcss254Hz, 5);
    (void) W25Qx_readSecurityRegister(0x10e0, calib->vhfCal.openSql9, 5);
    (void) W25Qx_readSecurityRegister(0x10e5, calib->vhfCal.closeSql9, 5);
    (void) W25Qx_readSecurityRegister(0x10ea, calib->vhfCal.closeSql1, 5);
    (void) W25Qx_readSecurityRegister(0x10ef, calib->vhfCal.openSql1, 5);
    (void) W25Qx_readSecurityRegister(0x10db, calib->vhfCal.dcsMod1, 5);
    (void) W25Qx_readSecurityRegister(0x2039, calib->vhfCal.sendIrange, 5);
    (void) W25Qx_readSecurityRegister(0x2049, calib->vhfCal.sendQrange, 5);
    (void) W25Qx_readSecurityRegister(0x2079, calib->uhfCal.analogSendIrange, 5);
    (void) W25Qx_readSecurityRegister(0x2089, calib->vhfCal.analogSendQrange, 5);

    (void) W25Qx_readSecurityRegister(0x2000, ((uint8_t *) &freqs), 40);
    W25Qx_sleep();

    for(uint8_t i = 0; i < 5; i++)
    {
        calib->vhfCal.rxFreq[i] = ((freq_t) bcdToBin(freqs[2*i]));
        calib->vhfCal.txFreq[i] = ((freq_t) bcdToBin(freqs[2*i+1]));
    }
}

void nvm_readHwInfo(hwInfo_t *info)
{
    uint16_t vhf_freqMin = 0;
    uint16_t vhf_freqMax = 0;
    uint16_t uhf_freqMin = 0;
    uint16_t uhf_freqMax = 0;
    uint8_t  lcdInfo = 0;

    /*
     * Hardware information data in MDUV3x0 devices is stored in security register
     * 0x3000.
     */
    W25Qx_wakeup();
    delayUs(5);

    (void) W25Qx_readSecurityRegister(0x3000, info->name, 8);
    (void) W25Qx_readSecurityRegister(0x3014, &uhf_freqMin, 2);
    (void) W25Qx_readSecurityRegister(0x3016, &uhf_freqMax, 2);
    (void) W25Qx_readSecurityRegister(0x3018, &vhf_freqMin, 2);
    (void) W25Qx_readSecurityRegister(0x301a, &vhf_freqMax, 2);
    (void) W25Qx_readSecurityRegister(0x301D, &lcdInfo, 1);
    W25Qx_sleep();

    /* Ensure correct null-termination of device name by removing the 0xff. */
    for(uint8_t i = 0; i < sizeof(info->name); i++)
    {
        if(info->name[i] == 0xFF) info->name[i] = '\0';
    }

    info->vhf_minFreq = ((uint16_t) bcdToBin(vhf_freqMin))/10;
    info->vhf_maxFreq = ((uint16_t) bcdToBin(vhf_freqMax))/10;
    info->uhf_minFreq = ((uint16_t) bcdToBin(uhf_freqMin))/10;
    info->uhf_maxFreq = ((uint16_t) bcdToBin(uhf_freqMax))/10;
    info->vhf_band = 1;
    info->uhf_band = 1;
    info->hw_version = lcdInfo & 0x03;
}

/**
 * TODO: functions temporarily implemented in "nvmem_settings_MDx.c"

int nvm_readVFOChannelData(channel_t *channel)
int nvm_readSettings(settings_t *settings)
int nvm_writeSettings(const settings_t *settings)

*/

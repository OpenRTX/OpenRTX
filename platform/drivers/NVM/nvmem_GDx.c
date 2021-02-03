/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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

#include <interfaces/delays.h>
#include <interfaces/nvmem.h>
#include <calibInfo_GDx.h>
#include "AT24Cx.h"
#include "W25Qx.h"

/**
 * \internal Data structure matching the one used by original GDx firmware to
 * manage channel data inside nonvolatile memory.
 *
 * Taken by dmrconfig repository: https://github.com/sergev/dmrconfig/blob/master/gd77.c
 */
typedef struct
{
    // Bytes 0-15
    uint8_t name[16];

    // Bytes 16-23
    uint32_t rx_frequency;
    uint32_t tx_frequency;

    // Byte 24
    uint8_t channel_mode;

    // Bytes 25-26
    uint8_t _unused25[2];

    // Bytes 27-28
    uint8_t tot;
    uint8_t tot_rekey_delay;

    // Byte 29
    uint8_t admit_criteria;

    // Bytes 30-31
    uint8_t _unused30;
    uint8_t scan_list_index;

    // Bytes 32-35
    uint16_t ctcss_dcs_receive;
    uint16_t ctcss_dcs_transmit;

    // Bytes 36-39
    uint8_t _unused36;
    uint8_t tx_signaling_syst;
    uint8_t _unused38;
    uint8_t rx_signaling_syst;

    // Bytes 40-43
    uint8_t _unused40;
    uint8_t privacy_group;

    uint8_t colorcode_tx;
    uint8_t group_list_index;

    // Bytes 44-47
    uint8_t colorcode_rx;
    uint8_t emergency_system_index;
    uint16_t contact_name_index;

    // Byte 48
    uint8_t _unused48           : 6,
            emergency_alarm_ack : 1,
            data_call_conf      : 1;

    // Byte 49
    uint8_t private_call_conf   : 1,
            _unused49_1         : 3,
            privacy             : 1,
            _unused49_5         : 1,
            repeater_slot2      : 1,
            _unused49_7         : 1;

    // Byte 50
    uint8_t dcdm                : 1,
            _unused50_1         : 4,
            non_ste_frequency   : 1,
            _unused50_6         : 2;

    // Byte 51
    uint8_t squelch             : 1,
            bandwidth           : 1,
            rx_only             : 1,
            talkaround          : 1,
            _unused51_4         : 2,
            vox                 : 1,
            power               : 1;

    // Bytes 52-55
    uint8_t _unused52[4];
}
gdxChannel_t;

#if defined(PLATFORM_GD77)
static const uint32_t UHF_CAL_BASE = 0x8F000;
static const uint32_t VHF_CAL_BASE = 0x8F070;
#elif defined(PLATFORM_DM1801)
static const uint32_t UHF_CAL_BASE = 0x6F000;
static const uint32_t VHF_CAL_BASE = 0x6F070;
#else
#warning GDx calibration: platform not supported
#endif

/**
 * \internal Utility function to convert 4 byte BCD values into a 32-bit
 * unsigned integer ones.
 */
uint32_t _bcd2bin(uint32_t bcd)
{
    return ((bcd >> 28) & 0x0F) * 10000000 +
           ((bcd >> 24) & 0x0F) * 1000000 +
           ((bcd >> 20) & 0x0F) * 100000 +
           ((bcd >> 16) & 0x0F) * 10000 +
           ((bcd >> 12) & 0x0F) * 1000 +
           ((bcd >> 8) & 0x0F)  * 100 +
           ((bcd >> 4) & 0x0F)  * 10 +
           (bcd & 0x0F);
}

/**
 * \internal Utility function for loading band-specific calibration data into
 * the corresponding data structure.
 */
void _loadBandCalData(uint32_t baseAddr, bandCalData_t *cal)
{
    W25Qx_readData(baseAddr + 0x08, &(cal->mod1Bias),              2);
    W25Qx_readData(baseAddr + 0x0A, &(cal->mod2Offset),            1);
    W25Qx_readData(baseAddr + 0x3F, cal->analogSqlThresh,          8);
    W25Qx_readData(baseAddr + 0x47, &(cal->noise1_HighTsh_Wb),     1);
    W25Qx_readData(baseAddr + 0x48, &(cal->noise1_LowTsh_Wb),      1);
    W25Qx_readData(baseAddr + 0x49, &(cal->noise2_HighTsh_Wb),     1);
    W25Qx_readData(baseAddr + 0x4A, &(cal->noise2_LowTsh_Wb),      1);
    W25Qx_readData(baseAddr + 0x4B, &(cal->rssi_HighTsh_Wb),       1);
    W25Qx_readData(baseAddr + 0x4C, &(cal->rssi_LowTsh_Wb),        1);
    W25Qx_readData(baseAddr + 0x4D, &(cal->noise1_HighTsh_Nb),     1);
    W25Qx_readData(baseAddr + 0x4E, &(cal->noise1_LowTsh_Nb),      1);
    W25Qx_readData(baseAddr + 0x4F, &(cal->noise2_HighTsh_Nb),     1);
    W25Qx_readData(baseAddr + 0x50, &(cal->noise2_LowTsh_Nb),      1);
    W25Qx_readData(baseAddr + 0x51, &(cal->rssi_HighTsh_Nb),       1);
    W25Qx_readData(baseAddr + 0x52, &(cal->rssi_LowTsh_Nb),        1);
    W25Qx_readData(baseAddr + 0x53, &(cal->RSSILowerThreshold),    1);
    W25Qx_readData(baseAddr + 0x54, &(cal->RSSIUpperThreshold),    1);
    W25Qx_readData(baseAddr + 0x55, cal->mod1Amplitude,            8);
    W25Qx_readData(baseAddr + 0x5D, &(cal->dacDataRange),          1);
    W25Qx_readData(baseAddr + 0x5E, &(cal->txDev_DTMF),            1);
    W25Qx_readData(baseAddr + 0x5F, &(cal->txDev_tone),            1);
    W25Qx_readData(baseAddr + 0x60, &(cal->txDev_CTCSS_wb),        1);
    W25Qx_readData(baseAddr + 0x61, &(cal->txDev_CTCSS_nb),        1);
    W25Qx_readData(baseAddr + 0x62, &(cal->txDev_DCS_wb),          1);
    W25Qx_readData(baseAddr + 0x63, &(cal->txDev_DCS_nb),          1);
    W25Qx_readData(baseAddr + 0x64, &(cal->PA_drv),                1);
    W25Qx_readData(baseAddr + 0x65, &(cal->PGA_gain),              1);
    W25Qx_readData(baseAddr + 0x66, &(cal->analogMicGain),         1);
    W25Qx_readData(baseAddr + 0x67, &(cal->rxAGCgain),             1);
    W25Qx_readData(baseAddr + 0x68, &(cal->mixGainWideband),       2);
    W25Qx_readData(baseAddr + 0x6A, &(cal->mixGainNarrowband),     2);
    W25Qx_readData(baseAddr + 0x6C, &(cal->rxAudioGainWideband),   1);
    W25Qx_readData(baseAddr + 0x6D, &(cal->rxAudioGainNarrowband), 1);

    uint8_t txPwr[32] = {0};
    W25Qx_readData(baseAddr + 0x0B, txPwr, 32);

    for(uint8_t i = 0; i < 16; i++)
    {
        cal->txLowPower[i]  = txPwr[2*i];
        cal->txHighPower[i] = txPwr[2*i+1];
    }
}

void nvm_init()
{
    W25Qx_init();
    AT24Cx_init();
}

void nvm_terminate()
{
    W25Qx_terminate();
    AT24Cx_terminate();
}

void nvm_readCalibData(void *buf)
{
    W25Qx_wakeup();
    delayUs(5);

    gdxCalibration_t *calib = ((gdxCalibration_t *) buf);

    _loadBandCalData(VHF_CAL_BASE, &(calib->data[0]));  /* Load VHF band calibration data */
    _loadBandCalData(UHF_CAL_BASE, &(calib->data[1]));  /* Load UHF band calibration data */

    W25Qx_sleep();

    /*
     * Finally, load calibration points. These are common among all the GDx
     * devices.
     * VHF calibration head and tail are not equally spaced as the other points,
     * so we manually override the values.
     */
    for(uint8_t i = 0; i < 16; i++)
    {
        uint8_t ii = i/2;
        calib->uhfMod1CalPoints[ii] = 405000000 + (5000000 * ii);
        calib->uhfPwrCalPoints[i]   = 400000000 + (5000000 * i);
    }

    for(uint8_t i = 0; i < 8; i++)
    {
        calib->vhfCalPoints[i] = 135000000 + (5000000 * i);
    }

    calib->vhfCalPoints[0] = 136000000;
    calib->vhfCalPoints[7] = 172000000;
}

void nvm_loadHwInfo(hwInfo_t *info)
{
    /* GDx devices does not have any hardware info in the external flash. */
    (void) info;
}

int nvm_readChannelData(channel_t *channel, uint16_t pos)
{
    (void) channel;
    (void) pos;
    return -1;
}

int nvm_readZoneData(zone_t *zone, uint16_t pos)
{
    (void) zone;
    (void) pos;
    return -1;
}

int nvm_readContactData(contact_t *contact, uint16_t pos)
{
    (void) contact;
    (void) pos;
    return -1;
}

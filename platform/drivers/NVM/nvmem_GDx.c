/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <string.h>
#include "wchar.h"
#include "interfaces/delays.h"
#include "interfaces/nvmem.h"
#include "core/nvmem_access.h"
#include "calibration/calibInfo_GDx.h"
#include "drivers/SPI/spi_bitbang.h"
#include "hwconfig.h"
#include "core/utils.h"
#include "drivers/NVM/AT24Cx.h"
#include "drivers/NVM/W25Qx.h"

static const struct W25QxCfg eflashCfg =
{
    .spi = (const struct spiDevice *) &nvm_spi,
    .cs  = { FLASH_CS }
};

W25Qx_DEVICE_DEFINE(eflash, eflashCfg)
AT24Cx_DEVICE_DEFINE(eeprom)

static const struct nvmDescriptor nvmDevices[] =
{
    {
        .name       = "External flash",
        .dev        = &eflash,
        .baseAddr   = 0x00000000,
        .size       = 0x100000,     // 1 MB,  8 Mbit
        .partNum    = 0,
        .partitions = NULL
    },
    {
        .name       = "EEPROM",
        .dev        = &eeprom,
        .baseAddr   = 0x00000000,
        .size       = 0x10000,      // 64 kB, 512 kbit
        .partNum    = 0,
        .partitions = NULL
    }
};

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
 * \internal Utility function for loading band-specific calibration data into
 * the corresponding data structure.
 *
 * @param baseAddr: start address of the data block;
 * @param cal: pointer to calibration data structure to be filled.
 */
static void _loadBandCalData(uint32_t baseAddr, bandCalData_t *cal)
{
    nvm_devRead(&eflash, baseAddr + 0x08, &(cal->modBias),            2);
    nvm_devRead(&eflash, baseAddr + 0x0A, &(cal->mod2Offset),         1);
    nvm_devRead(&eflash, baseAddr + 0x3F, cal->analogSqlThresh,       8);
    nvm_devRead(&eflash, baseAddr + 0x47, &(cal->noise1_HighTsh_Wb),  1);
    nvm_devRead(&eflash, baseAddr + 0x48, &(cal->noise1_LowTsh_Wb),   1);
    nvm_devRead(&eflash, baseAddr + 0x49, &(cal->noise2_HighTsh_Wb),  1);
    nvm_devRead(&eflash, baseAddr + 0x4A, &(cal->noise2_LowTsh_Wb),   1);
    nvm_devRead(&eflash, baseAddr + 0x4B, &(cal->rssi_HighTsh_Wb),    1);
    nvm_devRead(&eflash, baseAddr + 0x4C, &(cal->rssi_LowTsh_Wb),     1);
    nvm_devRead(&eflash, baseAddr + 0x4D, &(cal->noise1_HighTsh_Nb),  1);
    nvm_devRead(&eflash, baseAddr + 0x4E, &(cal->noise1_LowTsh_Nb),   1);
    nvm_devRead(&eflash, baseAddr + 0x4F, &(cal->noise2_HighTsh_Nb),  1);
    nvm_devRead(&eflash, baseAddr + 0x50, &(cal->noise2_LowTsh_Nb),   1);
    nvm_devRead(&eflash, baseAddr + 0x51, &(cal->rssi_HighTsh_Nb),    1);
    nvm_devRead(&eflash, baseAddr + 0x52, &(cal->rssi_LowTsh_Nb),     1);
    nvm_devRead(&eflash, baseAddr + 0x53, &(cal->RSSILowerThreshold), 1);
    nvm_devRead(&eflash, baseAddr + 0x54, &(cal->RSSIUpperThreshold), 1);
    nvm_devRead(&eflash, baseAddr + 0x55, cal->mod1Amplitude,         8);
    nvm_devRead(&eflash, baseAddr + 0x5D, &(cal->digAudioGain),       1);
    nvm_devRead(&eflash, baseAddr + 0x5E, &(cal->txDev_DTMF),         1);
    nvm_devRead(&eflash, baseAddr + 0x5F, &(cal->txDev_tone),         1);
    nvm_devRead(&eflash, baseAddr + 0x60, &(cal->txDev_CTCSS_wb),     1);
    nvm_devRead(&eflash, baseAddr + 0x61, &(cal->txDev_CTCSS_nb),     1);
    nvm_devRead(&eflash, baseAddr + 0x62, &(cal->txDev_DCS_wb),       1);
    nvm_devRead(&eflash, baseAddr + 0x63, &(cal->txDev_DCS_nb),       1);
    nvm_devRead(&eflash, baseAddr + 0x64, &(cal->PA_drv),             1);
    nvm_devRead(&eflash, baseAddr + 0x65, &(cal->PGA_gain),           1);
    nvm_devRead(&eflash, baseAddr + 0x66, &(cal->analogMicGain),      1);
    nvm_devRead(&eflash, baseAddr + 0x67, &(cal->rxAGCgain),          1);
    nvm_devRead(&eflash, baseAddr + 0x68, &(cal->mixGainWideband),    2);
    nvm_devRead(&eflash, baseAddr + 0x6A, &(cal->mixGainNarrowband),  2);
    nvm_devRead(&eflash, baseAddr + 0x6C, &(cal->rxDacGain),          1);
    nvm_devRead(&eflash, baseAddr + 0x6D, &(cal->rxVoiceGain),        1);

    uint8_t txPwr[32] = {0};
    nvm_devRead(&eflash, baseAddr + 0x0B, txPwr, 32);

    for(uint8_t i = 0; i < 16; i++)
    {
        cal->txLowPower[i]  = txPwr[2*i];
        cal->txHighPower[i] = txPwr[2*i+1];
    }
}


void nvm_init()
{
    spiBitbang_init(&nvm_spi);
    W25Qx_init(&eflash);
    AT24Cx_init();
}

void nvm_terminate()
{
    W25Qx_terminate(&eflash);
    AT24Cx_terminate();
}

const struct nvmDescriptor *nvm_getDesc(const size_t index)
{
    if(index >= ARRAY_SIZE(nvmDevices))
        return NULL;

    return &nvmDevices[index];
}

void nvm_readCalibData(void *buf)
{
    gdxCalibration_t *calib = ((gdxCalibration_t *) buf);

    _loadBandCalData(VHF_CAL_BASE, &(calib->data[0]));  /* Load VHF band calibration data */
    _loadBandCalData(UHF_CAL_BASE, &(calib->data[1]));  /* Load UHF band calibration data */

    /*
     * Finally, load calibration points. These are common among all the GDx
     * devices.
     * VHF calibration head and tail are not equally spaced as the other points,
     * so we manually override the values.
     */
    for(uint8_t i = 0; i < 16; i++)
    {
        uint8_t ii = i/2;
        calib->uhfCalPoints[ii]     = 405000000 + (5000000 * ii);
        calib->uhfPwrCalPoints[i]   = 400000000 + (5000000 * i);
    }

    for(uint8_t i = 0; i < 8; i++)
    {
        calib->vhfCalPoints[i] = 135000000 + (5000000 * i);
    }

    calib->vhfCalPoints[0] = 136000000;
    calib->vhfCalPoints[7] = 172000000;
}

void nvm_readHwInfo(hwInfo_t *info)
{
    /* GDx devices does not have any hardware info in the external flash. */
    (void) info;
}

int nvm_readVfoChannelData(channel_t *channel)
{
    (void) channel;
    return -1;
}

int nvm_readSettings(settings_t *settings)
{
    (void) settings;
    return -1;
}

int nvm_writeSettings(const settings_t *settings)
{
    (void) settings;
    return -1;
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    (void) settings;
    (void) vfo;
    return -1;
}

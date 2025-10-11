/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/nvmem.h"
#include "interfaces/delays.h"
#include "calibration/calibInfo_MDx.h"
#include "core/nvmem_access.h"
#include "drivers/SPI/spi_stm32.h"
#include <string.h>
#include "wchar.h"
#include "core/utils.h"
#include "drivers/NVM/W25Qx.h"

#define SECREG_READ(dev, offs, data, len) \
    nvm_devRead((const struct nvmDevice *) dev, offs, data, len)

static const struct W25QxCfg eflashCfg =
{
    #ifdef PLATFORM_MD9600
    .spi = &spi2,
    #else
    .spi = &nvm_spi,
    #endif
    .cs  = { FLASH_CS }
};

W25Qx_DEVICE_DEFINE(eflash, eflashCfg)
W25Qx_SECREG_DEFINE(cal1,   eflashCfg, 0x1000)
W25Qx_SECREG_DEFINE(cal2,   eflashCfg, 0x2000)
W25Qx_SECREG_DEFINE(hwInfo, eflashCfg, 0x3000)

static const struct nvmDescriptor nvmDevices[] =
{
    {
        .name       = "External flash",
        .dev        = &eflash,
        .size       = 0x1000000,    // 16 MB, 128 Mbit
        .partNum    = 0,
        .partitions = NULL
    },
    {
        .name       = "Cal. data 1",
        .dev        = (const struct nvmDevice *) &cal1,
        .size       = 0x100,        // 256 byte
        .partNum    = 0,
        .partitions = NULL
    },
    {
        .name       = "Cal. data 2",
        .dev        = (const struct nvmDevice *) &cal2,
        .size       = 0x100,        // 256 byte
        .partNum    = 0,
        .partitions = NULL
    }
};


void nvm_init()
{
    #ifndef PLATFORM_MD9600
    gpio_setMode(FLASH_CLK, ALTERNATE | ALTERNATE_FUNC(5));
    gpio_setMode(FLASH_SDO, ALTERNATE | ALTERNATE_FUNC(5));
    gpio_setMode(FLASH_SDI, ALTERNATE | ALTERNATE_FUNC(5));

    spiStm32_init(&nvm_spi, 21000000, 0);
    #endif

    W25Qx_init(&eflash);
}

void nvm_terminate()
{
    W25Qx_terminate(&eflash);
}

const struct nvmDescriptor *nvm_getDesc(const size_t index)
{
    if(index >= ARRAY_SIZE(nvmDevices))
        return NULL;

    return &nvmDevices[index];
}

void nvm_readCalibData(void *buf)
{
    uint32_t freqs[18];

    // Common calibration data between single and dual-band radios
    struct CalData *calib = ((struct CalData *) buf);

    // Security register 1: base address 0x1000
    SECREG_READ(&cal1, 0x0009, &(calib->freqAdjustMid), 1);
    SECREG_READ(&cal1, 0x0010, calib->txHighPower, 9);
    SECREG_READ(&cal1, 0x0020, calib->txLowPower, 9);
    SECREG_READ(&cal1, 0x0030, calib->rxSensitivity, 9);

    // Security register 2: base address 0x2000
    SECREG_READ(&cal2, 0x0030, calib->sendIrange, 9);
    SECREG_READ(&cal2, 0x0040, calib->sendQrange, 9);
    SECREG_READ(&cal2, 0x0070, calib->analogSendIrange, 9);
    SECREG_READ(&cal2, 0x0080, calib->analogSendQrange, 9);
    SECREG_READ(&cal2, 0x00b0, ((uint8_t *) &freqs), 72);

    /*
     * Frequency stored in calibration data is divided by ten: so, after
     * bcdToBin conversion, we have something like 40'135'000. To ajdust
     * things, frequency has to be multiplied by ten.
     */
    for(uint8_t i = 0; i < 9; i++)
    {
        calib->rxFreq[i] = ((freq_t) bcdToBin(freqs[2*i])) * 10;
        calib->txFreq[i] = ((freq_t) bcdToBin(freqs[2*i+1])) * 10;
    }

    // Calibration data for dual-band radios only
    #ifndef PLATFORM_MD3x0
    mduv3x0Calib_t *cal = (mduv3x0Calib_t *) buf;
    struct CalData *vhfCal = &(cal->vhfCal);

    // Security register 1: base address 0x1000
    SECREG_READ(&cal1, 0x000c, (&vhfCal->freqAdjustMid), 1);
    SECREG_READ(&cal1, 0x0019, vhfCal->txHighPower, 5);
    SECREG_READ(&cal1, 0x0029, vhfCal->txLowPower, 5);
    SECREG_READ(&cal1, 0x0039, vhfCal->rxSensitivity, 5);

    // Security register 2: base address 0x2000
    SECREG_READ(&cal2, 0x0039, vhfCal->sendIrange, 5);
    SECREG_READ(&cal2, 0x0049, vhfCal->sendQrange, 5);
    SECREG_READ(&cal2, 0x0079, vhfCal->analogSendIrange, 5);
    SECREG_READ(&cal2, 0x0089, vhfCal->analogSendQrange, 5);
    SECREG_READ(&cal2, 0x0000, ((uint8_t *) &freqs), 40);

    for(uint8_t i = 0; i < 5; i++)
    {
        vhfCal->rxFreq[i] = ((freq_t) bcdToBin(freqs[2*i]));
        vhfCal->txFreq[i] = ((freq_t) bcdToBin(freqs[2*i+1]));
    }
    #endif
}

void nvm_readHwInfo(hwInfo_t *info)
{
    uint16_t freqMin = 0;
    uint16_t freqMax = 0;
    uint8_t  lcdInfo = 0;

    // Security register 3: base address 0x3000
    SECREG_READ(&hwInfo, 0x0000, info->name, 8);
    SECREG_READ(&hwInfo, 0x0014, &freqMin, 2);
    SECREG_READ(&hwInfo, 0x0016, &freqMax, 2);
    SECREG_READ(&hwInfo, 0x001D, &lcdInfo, 1);

    // Ensure correct null-termination of device name by removing the 0xff.
    for(uint8_t i = 0; i < sizeof(info->name); i++)
    {
        if(info->name[i] == 0xFF)
            info->name[i] = '\0';
    }

    freqMin = ((uint16_t) bcdToBin(freqMin))/10;
    freqMax = ((uint16_t) bcdToBin(freqMax))/10;

    info->hw_version = lcdInfo & 0x03;

    #ifdef PLATFORM_MD3x0
    // Single band device, either VHF or UHF
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
    #else
    // For dual band devices load the remaining data
    uint16_t vhf_freqMin = 0;
    uint16_t vhf_freqMax = 0;

    SECREG_READ(&hwInfo, 0x0018, &vhf_freqMin, 2);
    SECREG_READ(&hwInfo, 0x001a, &vhf_freqMax, 2);

    info->vhf_minFreq = ((uint16_t) bcdToBin(vhf_freqMin))/10;
    info->vhf_maxFreq = ((uint16_t) bcdToBin(vhf_freqMax))/10;
    info->uhf_minFreq = freqMin;
    info->uhf_maxFreq = freqMax;
    info->vhf_band = 1;
    info->uhf_band = 1;
    #endif
}

/**
 * TODO: functions temporarily implemented in "nvmem_settings_MDx.c"

int nvm_readVFOChannelData(channel_t *channel)
int nvm_readSettings(settings_t *settings)
int nvm_writeSettings(const settings_t *settings)

*/

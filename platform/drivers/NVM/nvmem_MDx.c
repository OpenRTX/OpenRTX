/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */


 #include "targets/MD-UV3x0/pinmap.h"
 #include "drivers/SPI/spi_stm32.h"
#include "drivers/NVM/eeep.h"
#include "drivers/NVM/W25Qx.h"
#include "interfaces/platform.h"
#include "interfaces/nvmem.h"
#include "calibration/calibInfo_MDx.h"
#include "core/settings.h"
#include "core/vfo.h"
#include "core/nvmem_access.h"
#include "core/settings.h"
#include "core/utils.h"
#include "core/cps.h"

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
W25Qx_SECREG_DEFINE(secReg, eflashCfg)
EEEP_DEVICE_DEFINE(eeepDev)

static const struct nvmPartition eFlashPartitions[] =
{
    {   // Codeplug
        .offset = 0,
        .size = 0xFFC000,
    },
    {   // VFO
        .offset = 0xFFC000,
        .size = 8192
    },
    {   // Settings part A
        .offset = 0xFFE000,
        .size = 4096,
    },
    {   // Settings part B
        .offset = 0xFFF000,
        .size = 4096,
    }
};

static const struct nvmDescriptor nvmDevices[] =
{
    {
        .name       = "External flash",
        .dev        = &eflash,
        .baseAddr   = 0x00000000,
        .size       = 0x1000000,    // 16 MB, 128 Mbit
        .nbPart     = ARRAY_SIZE(eFlashPartitions),
        .partitions = eFlashPartitions,
    },
    {
        .name       = "Cal. data 1",
        .dev        = &secReg,
        .baseAddr   = 0x1000,
        .size       = 0x100,        // 256 byte
        .nbPart     = 0,
        .partitions = NULL
    },
    {
        .name       = "Cal. data 2",
        .dev        = &secReg,
        .baseAddr   = 0x2000,
        .size       = 0x100,        // 256 byte
        .nbPart     = 0,
        .partitions = NULL
    },
    {
        .name       = "Hardware Info",
        .dev        = &secReg,
        .baseAddr   = 0x3000,
        .size       = 0x100,        // 256 byte
        .nbPart     = 0,
        .partitions = NULL
    },
    {
        .name       = "Emulated EEPROM",
        .dev        = &eeepDev,
        .baseAddr   = 0x0000,
        .size       = 0xFFFF, // Virtual address is 16 bits
        .nbPart     = 0,
        .partitions = NULL
    }

};

const struct nvmTable nvmTab = {
    .areas = nvmDevices,
    .nbAreas = ARRAY_SIZE(nvmDevices),
};

struct settings_storage settings_storage;
struct vfo_storage vfo_slicer;

void nvm_init()
{
    #ifndef PLATFORM_MD9600
    gpio_setMode(FLASH_CLK, ALTERNATE | ALTERNATE_FUNC(5));
    gpio_setMode(FLASH_SDO, ALTERNATE | ALTERNATE_FUNC(5));
    gpio_setMode(FLASH_SDI, ALTERNATE | ALTERNATE_FUNC(5));

    spiStm32_init(&nvm_spi, 21000000, 0);
    #endif

    W25Qx_init(&eflash);

    settings_initStorage(&settings_storage, 0, 3, 4);
    eeep_init(&eeepDev, 0, 2);
    vfo_initStorage(&vfo_slicer, 4, 0, 16, CPS_VERSION_NUMBER);
}

void nvm_terminate()
{
    W25Qx_terminate(&eflash);
}

void nvm_readCalibData(void *buf)
{
    uint32_t freqs[18];

    // Common calibration data between single and dual-band radios
    struct CalData *calib = ((struct CalData *) buf);

    // Security register 1: base address 0x1000
    nvm_read(1, 0, 0x9, &(calib->freqAdjustMid), 1);
    nvm_read(1, 0, 0x10, calib->txHighPower, 9);
    nvm_read(1, 0, 0x20, calib->txLowPower, 9);
    nvm_read(1, 0, 0x30, calib->rxSensitivity, 9);

    // Security register 2: base address 0x2000
    nvm_read(2, 0, 0x30, calib->sendIrange, 9);
    nvm_read(2, 0, 0x40, calib->sendQrange, 9);
    nvm_read(2, 0, 0x70, calib->analogSendIrange, 9);
    nvm_read(2, 0, 0x80, calib->analogSendQrange, 9);
    nvm_read(2, 0, 0xb0, ((uint8_t *) &freqs), 72);

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
    nvm_read(1, 0, 0x0c, (&vhfCal->freqAdjustMid), 1);
    nvm_read(1, 0, 0x19, vhfCal->txHighPower, 5);
    nvm_read(1, 0, 0x29, vhfCal->txLowPower, 5);
    nvm_read(1, 0, 0x39, vhfCal->rxSensitivity, 5);

    // Security register 2: base address 0x2000
    nvm_read(2, 0, 0x39, vhfCal->sendIrange, 5);
    nvm_read(2, 0, 0x49, vhfCal->sendQrange, 5);
    nvm_read(2, 0, 0x79, vhfCal->analogSendIrange, 5);
    nvm_read(2, 0, 0x89, vhfCal->analogSendQrange, 5);
    nvm_read(2, 0, 0x00, ((uint8_t *) &freqs), 40);

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

    // Security register 3: base address 0x3000
    nvm_read(3, 0, 0x00, info->name, 8);
    nvm_read(3, 0, 0x14, &freqMin, 2);
    nvm_read(3, 0, 0x16, &freqMax, 2);
    #ifdef PLATFORM_MD9600
    nvm_read(3, 0, 0x35, &info->hw_version, 1);
    #else
    nvm_read(3, 0, 0x1D, &info->hw_version, 1);
    #endif

    // Ensure correct null-termination of device name by removing the 0xff.
    for(uint8_t i = 0; i < sizeof(info->name); i++)
    {
        if(info->name[i] == 0xFF)
            info->name[i] = '\0';
    }

    freqMin = ((uint16_t) bcdToBin(freqMin))/10;
    freqMax = ((uint16_t) bcdToBin(freqMax))/10;

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

    nvm_read(3, 0, 0x18, &vhf_freqMin, 2);
    nvm_read(3, 0, 0x1a, &vhf_freqMax, 2);

    info->vhf_minFreq = ((uint16_t) bcdToBin(vhf_freqMin))/10;
    info->vhf_maxFreq = ((uint16_t) bcdToBin(vhf_freqMax))/10;
    info->uhf_minFreq = freqMin;
    info->uhf_maxFreq = freqMax;
    info->vhf_band = 1;
    info->uhf_band = 1;
    #endif
}

int nvm_readSettings(settings_t *settings)
{
    return settings_load(&settings_storage, settings);
}

int nvm_writeSettings(const settings_t *settings)
{
    return settings_save(&settings_storage, settings);
}

int nvm_readVfoChannelData(channel_t *channel)
{
    return vfo_load(&vfo_slicer, channel);
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    int ret = nvm_writeSettings(settings);
    if(ret < 0)
        return ret;

    return vfo_save(&vfo_slicer, vfo);
}
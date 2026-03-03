/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/cps.h"
#include "core/vfo.h"
#include "interfaces/nvmem.h"
#include "calibration/calibInfo_CS7000.h"
#include "core/nvmem_access.h"
#include "core/utils.h"
#include "core/crc.h"
#include "drivers/SPI/spi_bitbang.h"
#include "drivers/SPI/spi_stm32.h"
#include "drivers/NVM/W25Qx.h"
#include "drivers/NVM/eeep.h"
#include "core/settings.h"

#include <string.h>

static const struct W25QxCfg cfg =
{
    .spi = (const struct spiDevice *) &flash_spi,
    .cs  = { FLASH_CS }
};

W25Qx_DEVICE_DEFINE(eflash, cfg)
EEEP_DEVICE_DEFINE(eeep)

const struct nvmPartition memPartitions[] =
{
    {
        .offset = 0x0000,   // First partition, calibration and other OEM data
        .size   = 32768     // 32kB
    },
#ifdef PLATFORM_CS7000P
    {
        .offset = 0x1000000,// Second partition EEEP storage
        .size   = 16384     // 16kB
    },
    {
        .offset = 0x1004000,// Third partition, settings part A
        .size   = 4096      // 4 kB
    },
    {
        .offset = 0x1005000,// Fourth partition, settings part B
        .size   = 4096      // 4 kB
    },
    {
        .offset = 0x1006000,// Fifth partition, available memory
        .size   = 0xFF2000
    }
#else
    {
        .offset = 0x8000,   // Second partition EEEP storage
        .size   = 16384     // 16kB
    },
    {
        .offset = 0xC000,   // Third partition, settings part A
        .size   = 4096      // 4 kB
    },
    {
        .offset = 0xD000,   // Fourth partition, settings part B
        .size   = 4096      // 4 kB
    },
    {
        .offset = 0xE000,   // Fifth partition, available memory
        .size   = 0xFF2000
    }
#endif
};

static const struct nvmDescriptor extMem[] =
{
    {
        .name       = "External flash",
        .dev        = &eflash,
        .baseAddr   = 0x00000000,
#ifdef PLATFORM_CS7000P
        .size       = 0x2000000,        // 32 MB, 256 Mbit
#else
        .size       = 0x1000000,        // 16 MB, 128 Mbit
#endif
        .nbPart     = sizeof(memPartitions)/sizeof(struct nvmPartition),
        .partitions = memPartitions
    },
    {
        .name       = "Virtual EEPROM",
        .dev        = &eeep,
        .baseAddr   = 0x00000000,
        .size       = 0xFFFF,       // Virtual address is 16 bits
        .nbPart     = 0,
        .partitions = NULL
    }
};

const struct nvmTable nvmTab = {
    .areas = extMem,
    .nbAreas = ARRAY_SIZE(extMem),
};

static struct vfo_storage vfo_storage;
static struct settings_storage settings_storage;

void nvm_init()
{
#ifdef PLATFORM_CS7000P
    gpio_setMode(FLASH_CLK, ALTERNATE | ALTERNATE_FUNC(5));
    gpio_setMode(FLASH_SDI, ALTERNATE | ALTERNATE_FUNC(5));
    gpio_setMode(FLASH_SDO, ALTERNATE | ALTERNATE_FUNC(5));
    spiStm32_init(&flash_spi, 25000000, 0);
#else
    spiBitbang_init(&flash_spi);
#endif
    W25Qx_init(&eflash);
    eeep_init(&eeep, 0, 2);
    vfo_initStorage(&vfo_storage, 1, 0, 16, CPS_VERSION_NUMBER);
    settings_initStorage(&settings_storage, 0, 3, 4);
}

void nvm_terminate()
{
    eeep_terminate(&eeep);
    W25Qx_terminate(&eflash);
#ifdef PLATFORM_CS7000P
    spiStm32_terminate(&flash_spi);
#else
    spiBitbang_terminate(&flash_spi);
#endif
}

void nvm_readCalibData(void *buf)
{
    struct CS7000Calib *calData = (struct CS7000Calib *) buf;

    nvm_read(0, 1, 0x1000, &(calData->txCalFreq),      sizeof(calData->txCalFreq));
    nvm_read(0, 1, 0x1020, &(calData->rxCalFreq),      sizeof(calData->rxCalFreq));
    nvm_read(0, 1, 0x1044, &(calData->rxSensitivity),  sizeof(calData->rxSensitivity));
    nvm_read(0, 1, 0x106C, &(calData->txHighPwr),      sizeof(calData->txHighPwr));
    nvm_read(0, 1, 0x1074, &(calData->txMiddlePwr),    sizeof(calData->txMiddlePwr));
    nvm_read(0, 1, 0x10C4, &(calData->txDigitalPathQ), sizeof(calData->txDigitalPathQ));
    nvm_read(0, 1, 0x10CC, &(calData->txAnalogPathI),  sizeof(calData->txAnalogPathI));
    nvm_read(0, 1, 0x10DC, &(calData->errorRate),      sizeof(calData->errorRate));

    for(int i = 0; i < 8; i++)
    {
        calData->txCalFreq[i] = __builtin_bswap32(calData->txCalFreq[i]);
        calData->rxCalFreq[i] = __builtin_bswap32(calData->rxCalFreq[i]);
    }
}

void nvm_readHwInfo(hwInfo_t *info)
{
    (void) info;
}

int nvm_readVfoChannelData(channel_t *channel)
{
    return vfo_load(&vfo_storage, channel);
}

int nvm_readSettings(settings_t *settings)
{
    return settings_load(&settings_storage, settings);
}

int nvm_writeSettings(const settings_t *settings)
{
    return settings_save(&settings_storage, settings);
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    int ret = nvm_writeSettings(settings);
    if(ret < 0)
        return ret;
    return vfo_save(&vfo_storage, vfo);
}

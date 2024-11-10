/***************************************************************************
 *   Copyright (C) 2024 by Silvano Seva IU2KWO                             *
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
#include <calibInfo_CS7000.h>
#include <nvmem_access.h>
#include <spi_bitbang.h>
#include <spi_stm32.h>
#include <string.h>
#include <wchar.h>
#include <utils.h>
#include <crc.h>
#include <W25Qx.h>
#include <eeep.h>

static const struct W25QxCfg cfg =
{
    .spi = (const struct spiDevice *) &flash_spi,
    .cs  = { FLASH_CS }
};

W25Qx_DEVICE_DEFINE(eflash, cfg, 0x1000000)        // 16 MB, 128 Mbit
EEEP_DEVICE_DEFINE(eeep)

const struct nvmPartition memPartitions[] =
{
    {
        .offset = 0x0000,   // First partition, calibration and other OEM data
        .size   = 32768     // 32kB
    },
    {
        .offset = 0x8000,   // Second partition EEEP storage
        .size   = 16384     // 16kB
    },
    {
        .offset = 0xC000,   // Third partition, available memory
        .size   = 0xFF4000
    }
};

static const struct nvmDescriptor extMem[] =
{
    {
        .name       = "External flash",
        .dev        = &eflash,
        .partNum    = sizeof(memPartitions)/sizeof(struct nvmPartition),
        .partitions = memPartitions
    },
    {
        .name       = "Virtual EEPROM",
        .dev        = &eeep,
        .partNum    = 0,
        .partitions = NULL
    }
};

static uint16_t settingsCrc;
static uint16_t vfoCrc;

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
    eeep_init(&eeep, 0, 1);
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

const struct nvmDescriptor *nvm_getDesc(const size_t index)
{
    if(index > 2)
        return NULL;

    return &extMem[index];
}

void nvm_readCalibData(void *buf)
{
    struct CS7000Calib *calData = (struct CS7000Calib *) buf;

    nvm_read(0, 0, 0x1000, &(calData->txCalFreq),      sizeof(calData->txCalFreq));
    nvm_read(0, 0, 0x1020, &(calData->rxCalFreq),      sizeof(calData->rxCalFreq));
    nvm_read(0, 0, 0x1044, &(calData->rxSensitivity),  sizeof(calData->rxSensitivity));
    nvm_read(0, 0, 0x106C, &(calData->txHighPwr),      sizeof(calData->txHighPwr));
    nvm_read(0, 0, 0x1074, &(calData->txMiddlePwr),    sizeof(calData->txMiddlePwr));
    nvm_read(0, 0, 0x10C4, &(calData->txDigitalPathQ), sizeof(calData->txDigitalPathQ));
    nvm_read(0, 0, 0x10CC, &(calData->txAnalogPathI),  sizeof(calData->txAnalogPathI));
    nvm_read(0, 0, 0x10DC, &(calData->errorRate),      sizeof(calData->errorRate));

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
    memset(channel, 0x00, sizeof(channel_t));
    int ret = nvm_read(1, -1, 0x0001, channel, sizeof(channel_t));
    if(ret < 0)
        return -1;

    vfoCrc = crc_ccitt(channel, sizeof(channel_t));

    return 0;
}

int nvm_readSettings(settings_t *settings)
{
    memset(settings, 0x00, sizeof(settings_t));
    int ret = nvm_read(1, -1, 0x0002, settings, sizeof(settings_t));
    if(ret < 0)
        return -1;

    settingsCrc = crc_ccitt(settings, sizeof(settings_t));

    return 0;
}

int nvm_writeSettings(const settings_t *settings)
{
    (void) settings;

    return -1;
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    uint16_t crc = crc_ccitt(vfo, sizeof(channel_t));
    if(crc != vfoCrc)
        nvm_write(1, -1, 0x0001, vfo, sizeof(channel_t));

    crc = crc_ccitt(settings, sizeof(settings_t));
    if(crc != settingsCrc)
        nvm_write(1, -1, 0x0002, settings, sizeof(settings_t));

    return 0;
}

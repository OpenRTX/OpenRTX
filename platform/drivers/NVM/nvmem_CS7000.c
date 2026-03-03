/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <string.h>
#include "interfaces/nvmem.h"
#include "calibration/calibInfo_CS7000.h"
#include "core/nvmem_access.h"
#include "drivers/SPI/spi_bitbang.h"
#include "drivers/SPI/spi_stm32.h"
#include "drivers/NVM/W25Qx.h"
#include "drivers/NVM/eeep.h"
#include "core/settings.h"
#include "core/utils.h"
#include "core/crc.h"
#include "core/vfo.h"

static const struct W25QxCfg cfg = {
    .spi = (const struct spiDevice *)&flash_spi,
    .cs = { FLASH_CS },
};

W25Qx_DEVICE_DEFINE(eflash, cfg);
EEEP_DEVICE_DEFINE(eeep)
EEEP_DEVICE_DEFINE(eeepDev1)
EEEP_DEVICE_DEFINE(eeepDev2)

const struct nvmPartition memPartitions[] = {
    NVM_PART_DEFINE(0x0000, 32768), // Calibration and other OEM data

#ifdef PLATFORM_CS7000P
    NVM_PART_DEFINE(0x1000000, 16384),      // EEEP storage
    NVM_PART_DEFINE(0x100C000, 0xFF0000),   // Available memory
    NVM_PART_DEFINE(0x1FFC000, 12288),      // Settings part A and VFO
    NVM_PART_DEFINE(0x1FFF000, 4096),       // Settings part B
#else
    NVM_PART_DEFINE(0x8000,   16384),
    NVM_PART_DEFINE(0xC000,   0xFF0000),
    NVM_PART_DEFINE(0xFFC000, 12288),
    NVM_PART_DEFINE(0xFFF000, 4096),
#endif
};

static const struct nvmPartition eeepDev1Partitions[] = {
    NVM_PART_DEFINE(0x0000, 4096), // VFO
    NVM_PART_DEFINE(0x1000, 4096), // Settings part A
};

static const struct nvmDescriptor extMem[] = {
    {
        .name = "External flash",
        .dev = &eflash,
        .baseAddr = 0x00000000,
#ifdef PLATFORM_CS7000P
        .size = 0x2000000, // 32 MB, 256 Mbit
#else
        .size = 0x1000000, // 16 MB, 128 Mbit
#endif
        .nbPart = sizeof(memPartitions) / sizeof(struct nvmPartition),
        .partitions = memPartitions,
    },
    {
        .name = "Virtual EEPROM",
        .dev = &eeep,
        .baseAddr = 0x00000000,
        .size = 0xFFFF,
        .nbPart = 0,
        .partitions = NULL,
    },
    {
        .name = "Emulated EEPROM 1",
        .dev = &eeepDev1,
        .baseAddr = 0x0000,
        .size = 0xFFFF,
        .nbPart = ARRAY_SIZE(eeepDev1Partitions),
        .partitions = eeepDev1Partitions,
    },
    {
        .name = "Emulated EEPROM 2",
        .dev = &eeepDev2,
        .baseAddr = 0x0000,
        .size = 0xFFFF,
        .nbPart = 0,
        .partitions = NULL,
    },
};

const struct nvmTable nvmTab = {
    .areas = extMem,
    .nbAreas = ARRAY_SIZE(extMem),
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
    eeep_init(&eeep, 0, 2);
    eeep_init(&eeepDev1, 0, 4); // Device 0, partition 4
    eeep_init(&eeepDev2, 0, 5); // Device 0, partition 5

    settings_init(2, 2, 3, 0);  // Device 2, partition 2 and device 3
    vfo_init(2, 1);             // Device 2, partition 1
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
    struct CS7000Calib *calData = (struct CS7000Calib *)buf;

    nvm_read(0, 1, 0x1000, &(calData->txCalFreq), sizeof(calData->txCalFreq));
    nvm_read(0, 1, 0x1020, &(calData->rxCalFreq), sizeof(calData->rxCalFreq));
    nvm_read(0, 1, 0x1044, &(calData->rxSensitivity),
             sizeof(calData->rxSensitivity));
    nvm_read(0, 1, 0x106C, &(calData->txHighPwr), sizeof(calData->txHighPwr));
    nvm_read(0, 1, 0x1074, &(calData->txMiddlePwr),
             sizeof(calData->txMiddlePwr));
    nvm_read(0, 1, 0x10C4, &(calData->txDigitalPathQ),
             sizeof(calData->txDigitalPathQ));
    nvm_read(0, 1, 0x10CC, &(calData->txAnalogPathI),
             sizeof(calData->txAnalogPathI));
    nvm_read(0, 1, 0x10DC, &(calData->errorRate), sizeof(calData->errorRate));

    for (int i = 0; i < 8; i++) {
        calData->txCalFreq[i] = __builtin_bswap32(calData->txCalFreq[i]);
        calData->rxCalFreq[i] = __builtin_bswap32(calData->rxCalFreq[i]);
    }
}

void nvm_readHwInfo(hwInfo_t *info)
{
    (void)info;
}

int nvm_readVfoChannelData(channel_t *channel)
{
    memset(channel, 0x00, sizeof(channel_t));
    int ret = nvm_read(1, 0, 0x0001, channel, sizeof(channel_t));
    if (ret < 0)
        return -1;

    vfoCrc = crc_ccitt(channel, sizeof(channel_t));

    return 0;
}

int nvm_readSettings(settings_t *settings)
{
    memset(settings, 0x00, sizeof(settings_t));
    int ret = nvm_read(1, 0, 0x0002, settings, sizeof(settings_t));
    if (ret < 0)
        return -1;

    settingsCrc = crc_ccitt(settings, sizeof(settings_t));

    return 0;
}

int nvm_writeSettings(const settings_t *settings)
{
    (void)settings;

    return -1;
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    uint16_t crc = crc_ccitt(vfo, sizeof(channel_t));
    if (crc != vfoCrc)
        nvm_write(1, 0, 0x0001, vfo, sizeof(channel_t));

    crc = crc_ccitt(settings, sizeof(settings_t));
    if (crc != settingsCrc)
        nvm_write(1, 0, 0x0002, settings, sizeof(settings_t));

    return 0;
}

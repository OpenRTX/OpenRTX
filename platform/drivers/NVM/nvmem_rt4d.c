/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <interfaces/nvmem.h>
#include <interfaces/delays.h>
#include "core/nvmem_device.h"
#include <peripherals/gpio.h>
#include <hwconfig.h>
#include "drivers/SPI/spi_bitbang.h"
#include <string.h>
#include <wchar.h>
#include "core/utils.h"
#include "core/crc.h"
#include "drivers/NVM/W25Qx.h"

static const struct W25QxCfg cfg = {
    .spi = (struct spiDevice *)&flash_spi,
    .cs = { EFLASH_CS },
};

W25Qx_DEVICE_DEFINE(eflash, cfg)

    const struct nvmPartition memPartitions[] = {
        {
            .offset = 0x0000, // First partition
            .size = 0x1000000 // 16MiB
        },
    };

static const struct nvmDescriptor extMem[] = { {
    .name = "External flash",
    .dev = &eflash,
    .baseAddr = 0x00000000,
    .size = 0x1000000, // 16 MB, 128 Mbit
    .nbPart = sizeof(memPartitions) / sizeof(struct nvmPartition),
    .partitions = memPartitions,
} };

const struct nvmTable nvmTab = {
    .areas = extMem,
    .nbAreas = ARRAY_SIZE(extMem),
};

void nvm_init()
{
    spiBitbang_init(&flash_spi);
    W25Qx_init(&eflash);
}

void nvm_terminate()
{
    W25Qx_terminate(&eflash);
    spiBitbang_terminate(&flash_spi);
}

void nvm_readCalibData(void *buf)
{
    (void)buf;
}

void nvm_readHwInfo(hwInfo_t *info)
{
    (void)info;
}

int nvm_readVfoChannelData(channel_t *channel)
{
    (void)channel;

    return -1;
}

int nvm_readSettings(settings_t *settings)
{
    (void)settings;

    return -1;
}

int nvm_writeSettings(const settings_t *settings)
{
    (void)settings;

    return -1;
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    (void)settings;
    (void)vfo;

    return 0;
}

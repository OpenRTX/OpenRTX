/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>
#include "interfaces/nvmem.h"
#include "flash_zephyr.h"

ZEPHYR_FLASH_DEVICE_DEFINE(eflash, flash);

static const struct nvmDescriptor nvMemory =
{
    .name       = "External flash",
    .dev        = &eflash,
    .baseAddr   = 0x00000000,
    .size       = FIXED_PARTITION_SIZE(storage_partition),
    .nbPart     = 0,
    .partitions = NULL
};

const struct nvmTable nvmTab = {
    .areas = &nvMemory,
    .nbAreas = 1,
};

void nvm_init()
{
    zephirFlash_init(&eflash);
}

void nvm_terminate()
{

}

void nvm_readCalibData(void *buf)
{
    (void) buf;
}

void nvm_readHwInfo(hwInfo_t *info)
{
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

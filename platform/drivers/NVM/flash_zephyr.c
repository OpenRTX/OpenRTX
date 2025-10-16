/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <zephyr/drivers/flash.h>
#include "flash_zephyr.h"

#define TO_DEV_HANDLE(x) ((const struct device *) x)


static int nvm_api_read(const struct nvmDevice *dev, uint32_t offset,
                        void *data, size_t len)
{
    return flash_read(TO_DEV_HANDLE(dev->priv), offset, data, len);
}

static int nvm_api_write(const struct nvmDevice *dev, uint32_t offset,
                         const void *data, size_t len)
{
    return flash_write(TO_DEV_HANDLE(dev->priv), offset, data, len);
}

static int nvm_api_erase(const struct nvmDevice *dev, uint32_t offset, size_t size)
{
    return flash_erase(TO_DEV_HANDLE(dev->priv), offset, size);
}


int zephirFlash_init(const struct nvmDevice* dev)
{
    // Retrieve write size
    const struct flash_parameters *info = flash_get_parameters(TO_DEV_HANDLE(dev->priv));

    // TODO: erase size and erase cycles to be retrieved from the real device.
    struct nvmInfo *nvm_info = (struct nvmInfo *) dev->info;
    nvm_info->write_size   = info->write_block_size;
    nvm_info->erase_size   = 4096;
    nvm_info->erase_cycles = 100000;
    nvm_info->device_info  = NVM_FLASH | NVM_WRITE | NVM_BITWRITE | NVM_ERASE;

    return 0;
}

const struct nvmOps zephyr_flash_ops =
{
    .read   = nvm_api_read,
    .write  = nvm_api_write,
    .erase  = nvm_api_erase,
    .sync   = NULL
};

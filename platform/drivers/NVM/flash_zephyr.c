/***************************************************************************
 *   Copyright (C) 2023 - 2025 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

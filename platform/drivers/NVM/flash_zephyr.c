/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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

static int nvm_api_read(const struct nvmDevice *dev, uint32_t offset,
                        void *data, size_t len)
{
    const struct device *fDev = (const struct device *)(dev->config);

    return flash_read(fDev, offset, data, len);
}

static int nvm_api_write(const struct nvmDevice *dev, uint32_t offset,
                         const void *data, size_t len)
{
    const struct device *fDev = (const struct device *)(dev->config);

    return flash_write(fDev, offset, data, len);
}

static int nvm_api_erase(const struct nvmDevice *dev, uint32_t offset, size_t size)
{
    const struct device *fDev = (const struct device *)(dev->config);

    return flash_erase(fDev, offset, size);
}

static const struct nvmParams *nvm_api_params(const struct nvmDevice *dev)
{
    struct nvmParams    *params = (struct nvmParams *)(dev->priv);
    const struct device *fDev   = (const struct device *)(dev->config);

    // Retrieve write size
    const struct flash_parameters *info = flash_get_parameters(fDev);
    params->write_size = info->write_block_size;

    // TODO: erase size and erase cycles to be retrieved from the real device.
    params->erase_size   = 4096;
    params->erase_cycles = 100000;
    params->type         = NVM_FLASH;

    return params;
}

const struct nvmApi zephyr_flash_api =
{
    .read   = nvm_api_read,
    .write  = nvm_api_write,
    .erase  = nvm_api_erase,
    .sync   = NULL,
    .params = nvm_api_params
};

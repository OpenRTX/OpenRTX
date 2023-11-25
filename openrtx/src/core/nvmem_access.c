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

#include <interfaces/nvmem.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>


/**
 * \internal
 * Check if a read/write/erase operation is within the bounds of a given NVM
 * area.
 *
 * @param area: pointer to the NVM area descriptor
 * @param addr: start address of the read/write/erase operation
 * @param len: size of the read/write/erase operation
 * @return true if the operation is within the NVM area bounds
 */
static inline bool checkBounds(const struct nvmArea *area, uint32_t addr, size_t len)
{
    return (addr >= area->startAddr)
        && ((addr + len) < (area->startAddr + area->size));
}


int nvmArea_read(const struct nvmArea *area, uint32_t address, void *data, size_t len)
{
    const struct nvmDevice *dev = area->dev;

    if(checkBounds(area, address, len) == false)
        return -EINVAL;

    return dev->api->read(dev, address, data, len);
}

int nvmArea_write(const struct nvmArea *area, uint32_t address, void *data, size_t len)
{
    const struct nvmDevice *dev = area->dev;

    if(checkBounds(area, address, len) == false)
        return -EINVAL;

    if(dev->api->write == NULL)
        return -ENOTSUP;

    int ret = dev->api->write(dev, address, data, len);
    if(ret < 0)
        return ret;

    if(dev->api->sync != NULL)
        dev->api->sync(dev);

    return 0;
}

int nvmArea_erase(const struct nvmArea *area, uint32_t address, size_t size)
{
    const struct nvmDevice *dev = area->dev;

    if(checkBounds(area, address, size) == false)
        return -EINVAL;

    if(dev->api->erase == NULL)
        return -ENOTSUP;

    return dev->api->erase(dev, address, size);
}

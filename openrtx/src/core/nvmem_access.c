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

#include <interfaces/nvmem.h>
#include <nvmem_access.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>


/**
 * \internal
 * Compute the absolute offset from the beginning of an NVM device, given the
 * device descriptor and the partition number. If the partition number is set
 * to -1, the offset is considered from the beginning of the device.
 * This function performs also a bound check to guarantee that the following
 * operation stays within the limits of the partition (if any).
 *
 * @param nvm: pointer to NVM descriptor.
 * @param part: partition number.
 * @param offset: pointer to the offset.
 * @param len: lenght of the read/write/erase operation.
 * @return a negative error code or zero.
 */
static inline int getAbsOffset(const struct nvmDescriptor *nvm, const int part,
                               uint32_t *offset, size_t len)
{
    const struct nvmPartition *np;

    // Offset is relative to a partition
    if(part >= 0)
    {
        if((size_t) part >= nvm->partNum)
            return -EINVAL;

        np = &nvm->partitions[part];

        if((*offset + len) > np->size)
            return -EINVAL;

        *offset += np->offset;
    }

    return 0;
}


int nvm_read(const uint32_t dev, const int part, uint32_t offset, void *data,
             size_t len)
{
    const struct nvmDescriptor *nvm = nvm_getDesc(dev);
    if(nvm == NULL)
        return -EINVAL;

    int ret = getAbsOffset(nvm, part, &offset, len);
    if(ret < 0)
        return ret;

    return nvm_devRead(nvm->dev, offset, data, len);
}

int nvm_write(const uint32_t dev, const int part, uint32_t offset, const void *data,
              size_t len)
{
    const struct nvmDescriptor *nvm = nvm_getDesc(dev);
    if(nvm == NULL)
        return -EINVAL;

    int ret = getAbsOffset(nvm, part, &offset, len);
    if(ret < 0)
        return ret;

    return nvm_devWrite(nvm->dev, offset, data, len);
}

int nvm_erase(const uint32_t dev, const int part, uint32_t offset, size_t size)
{
    const struct nvmDescriptor *nvm = nvm_getDesc(dev);
    if(nvm == NULL)
        return -EINVAL;

    int ret = getAbsOffset(nvm, part, &offset, size);
    if(ret < 0)
        return ret;

    return nvm_devErase(nvm->dev, offset, size);
}

int nvm_devErase(const struct nvmDevice *dev, uint32_t offset, size_t size)
{
    // Erase operation is allowed
    if(dev->ops->erase == NULL)
        return -ENOTSUP;

    // Out-of-bounds check
    if((offset + size) > dev->size)
        return -EINVAL;

    // Start offset must be aligned to the erase size
    if((offset % dev->info->erase_size) != 0)
        return -EINVAL;

    // Total size must be aligned to the erase size
    if((size % dev->info->erase_size) != 0)
        return -EINVAL;

    return dev->ops->erase(dev, offset, size);
}

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/nvmem.h"
#include "core/nvmem_access.h"
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
    uint32_t offs = *offset;

    // Offset is relative to a partition
    if (part >= 0) {
        if ((size_t)part >= nvm->partNum)
            return -EINVAL;

        np = &nvm->partitions[part];

        if ((offs + len) > np->size)
            return -EINVAL;

        offs += np->offset;
    }

    if ((offs + len) > nvm->size)
        return -EINVAL;

    *offset = offs;
    return 0;
}

int nvm_read(const uint32_t dev, const int part, uint32_t offset, void *data,
             size_t len)
{
    const struct nvmDescriptor *nvm = nvm_getDesc(dev);
    if (nvm == NULL)
        return -EINVAL;

    int ret = getAbsOffset(nvm, part, &offset, len);
    if (ret < 0)
        return ret;

    return nvm_devRead(nvm->dev, nvm->baseAddr + offset, data, len);
}

int nvm_write(const uint32_t dev, const int part, uint32_t offset,
              const void *data, size_t len)
{
    const struct nvmDescriptor *nvm = nvm_getDesc(dev);
    if (nvm == NULL)
        return -EINVAL;

    int ret = getAbsOffset(nvm, part, &offset, len);
    if (ret < 0)
        return ret;

    return nvm_devWrite(nvm->dev, nvm->baseAddr + offset, data, len);
}

int nvm_erase(const uint32_t dev, const int part, uint32_t offset, size_t size)
{
    const struct nvmDescriptor *nvm = nvm_getDesc(dev);
    if (nvm == NULL)
        return -EINVAL;

    int ret = getAbsOffset(nvm, part, &offset, size);
    if (ret < 0)
        return ret;

    return nvm_devErase(nvm->dev, nvm->baseAddr + offset, size);
}

int nvm_devErase(const struct nvmDevice *dev, uint32_t address, size_t size)
{
    // Erase operation is allowed
    if (dev->ops->erase == NULL)
        return -ENOTSUP;

    // Start address must be aligned to the erase size
    if ((address % dev->info->erase_size) != 0)
        return -EINVAL;

    // Total size must be aligned to the erase size
    if ((size % dev->info->erase_size) != 0)
        return -EINVAL;

    return dev->ops->erase(dev, address, size);
}

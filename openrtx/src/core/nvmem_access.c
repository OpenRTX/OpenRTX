/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/nvmem_access.h"
#include "core/nvmem_device.h"
#include <stdint.h>
#include <errno.h>

const struct nvmDescriptor *nvm_getDesc(const uint32_t index)
{
    if (index >= nvmTab.nbAreas)
        return NULL;

    return &nvmTab.areas[index];
}

int nvm_getPart(const uint32_t idx, const uint32_t part,
                struct nvmPartition *pInfo)
{
    const struct nvmDescriptor *desc = nvm_getDesc(idx);
    if (desc == NULL)
        return -EINVAL;

    if (part == 0) {
        pInfo->offset = 0;
        pInfo->size = desc->size;
    } else {
        if (part > desc->nbPart)
            return -EINVAL;

        *pInfo = desc->partitions[part - 1];
    }

    return 0;
}

int nvm_read(const uint32_t idx, const uint32_t part, uint32_t offset,
             void *data, size_t len)
{
    const struct nvmDescriptor *nvm;
    struct nvmPartition np;
    int ret;

    ret = nvm_getPart(idx, part, &np);
    if (ret < 0)
        return ret;

    // Accesses exceeding the nvm->size boundary are caugth by this check,
    // provided that the partition table is well defined.
    if ((offset + len) > np.size)
        return -EINVAL;

    // Index already validated by nvm_getPart
    nvm = &nvmTab.areas[idx];
    offset += np.offset;

    return nvm_devRead(nvm->dev, nvm->baseAddr + offset, data, len);
}

int nvm_write(const uint32_t idx, const uint32_t part, uint32_t offset,
              const void *data, size_t len)
{
    const struct nvmDescriptor *nvm;
    struct nvmPartition np;
    int ret;

    ret = nvm_getPart(idx, part, &np);
    if (ret < 0)
        return ret;

    if ((offset + len) > np.size)
        return -EINVAL;

    nvm = &nvmTab.areas[idx];
    offset += np.offset;

    return nvm_devWrite(nvm->dev, nvm->baseAddr + offset, data, len);
}

int nvm_erase(const uint32_t idx, const uint32_t part, uint32_t offset,
              size_t size)
{
    const struct nvmDescriptor *nvm;
    struct nvmPartition np;
    int ret;

    ret = nvm_getPart(idx, part, &np);
    if (ret < 0)
        return ret;

    if ((offset + size) > np.size)
        return -EINVAL;

    nvm = &nvmTab.areas[idx];
    offset += np.offset;

    return nvm_devErase(nvm->dev, nvm->baseAddr + offset, size);
}

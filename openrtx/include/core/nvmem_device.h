/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NVMEM_DEVICE_H
#define NVMEM_DEVICE_H

#include "interfaces/nvmem.h"
#include <stdint.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Perform a byte-aligned read operation from a nonvolatile memory device.
 *
 * @param dev: pointer to the NVM device descriptor.
 * @param address: start address for the read operation.
 * @param data: pointer to a buffer where to store the data read.
 * @param len: number of bytes to read.
 * @return zero on success, a negative error code otherwise.
 */
static inline int nvm_devRead(const struct nvmDevice *dev, uint32_t address,
                              void *data, size_t len)
{
    return dev->ops->read(dev, address, data, len);
}

/**
 * Perform a write operation to a nonvolatile memory device.
 *
 * @param dev: pointer to the NVM device descriptor.
 * @param address: start address for the write operation.
 * @param data: pointer to a buffer containing the data to write.
 * @param len: number of bytes to write.
 * @return zero on success, a negative error code otherwise.
 */
static inline int nvm_devWrite(const struct nvmDevice *dev, uint32_t address,
                               const void *data, size_t len)
{
    if (dev->ops->write == NULL)
        return -ENOTSUP;

    if ((address % dev->info->write_size) != 0)
        return -EINVAL;

    if ((len % dev->info->write_size) != 0)
        return -EINVAL;

    return dev->ops->write(dev, address, data, len);
}

/**
 * Perform an erase operation on a nonvolatile memory device.
 *
 * @param dev: pointer to the NVM device descriptor.
 * @param address: start address for the erase operation.
 * @param size: size of the area to be erased.
 * @return zero on success, a negative error code otherwise.
 */
static inline int nvm_devErase(const struct nvmDevice *dev, uint32_t address,
                               size_t size)
{
    if (dev->ops->erase == NULL)
        return -ENOTSUP;

    if ((address % dev->info->erase_size) != 0)
        return -EINVAL;

    if ((size % dev->info->erase_size) != 0)
        return -EINVAL;

    return dev->ops->erase(dev, address, size);
}

/**
 * Sync device cache and state to its underlying hardware.
 *
 * @param dev: pointer to NVM device descriptor.
 * @return 0 on success, negative errno code on fail.
 */
static inline int nvm_devSync(const struct nvmDevice *dev)
{
    if (dev->ops->sync == NULL)
        return -ENOTSUP;

    return dev->ops->sync(dev);
}

#ifdef __cplusplus
}
#endif

#endif

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NVMEM_ACCESS_H
#define NVMEM_ACCESS_H

#include "interfaces/nvmem.h"
#include <stdint.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Perform a byte-aligned read operation on a nonvolatile memory.
 *
 * @param dev: NVM device number.
 * @param part: partition number, -1 for direct device access.
 * @param address: offset for the read operation.
 * @param data: pointer to a buffer where to store the data read.
 * @param len: number of bytes to read.
 * @return zero on success, a negative error code otherwise.
 */
int nvm_read(const uint32_t dev, const int part, uint32_t offset, void *data,
             size_t len);

/**
 * Perform a write operation on a nonvolatile memory.
 *
 * @param dev: NVM device number.
 * @param part: partition number, -1 for direct device access.
 * @param offset: offset for the write operation.
 * @param data: pointer to a buffer containing the data to write.
 * @param len: number of bytes to write.
 * @return zero on success, a negative error code otherwise.
 */
int nvm_write(const uint32_t dev, const int part, uint32_t offset,
              const void *data, size_t len);

/**
 * Perform an erase operation on a nonvolatile memory. Acceptable offset and
 * size depend on characteristics of the underlying device.
 *
 * @param dev: NVM device number.
 * @param part: partition number, -1 for direct device access.
 * @param offset: offset for the erase operation.
 * @param size: size of the area to be erased.
 * @return zero on success, a negative error code otherwise.
 */
int nvm_erase(const uint32_t dev, const int part, uint32_t offset, size_t size);

/**
 * Perform a byte-aligned read operation on an NVM area.
 *
 * @param area: pointer to the NVM are descriptor.
 * @param offset: offset for the read operation.
 * @param data: pointer to a buffer where to store the data read.
 * @param len: number of bytes to read.
 * @return zero on success, a negative error code otherwise.
 */
static inline int nvm_devRead(const struct nvmDevice *dev, uint32_t offset,
                              void *data, size_t len)
{
    return dev->ops->read(dev, offset, data, len);
}

/**
 * Perform a byte-aligned write operation on an NVM area. If the underlying
 * device requires state syncing, a sync operation is performed at the end of
 * the write.
 *
 * @param area: pointer to the NVM are descriptor.
 * @param offset: offset for the write operation.
 * @param data: pointer to a buffer containing the data to write.
 * @param len: number of bytes to write.
 * @return zero on success, a negative error code otherwise.
 */
static inline int nvm_devWrite(const struct nvmDevice *dev, uint32_t offset,
                               const void *data, size_t len)
{
    if (dev->ops->write == NULL)
        return -ENOTSUP;

    return dev->ops->write(dev, offset, data, len);
}

/**
 * Perform an erase operation on an NVM area. Acceptable offset and size depend
 * on the NVM device the area belongs to.
 *
 * @param area: pointer to the NVM are descriptor.
 * @param offset: offset for the erase operation.
 * @param size: size of the area to be erased.
 * @return zero on success, a negative error code otherwise.
 */
int nvm_devErase(const struct nvmDevice *dev, uint32_t offset, size_t size);

/**
 * Sync device cache and state to its underlying hardware.
 * If the device does not support sync this function pointer is set to NULL.
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

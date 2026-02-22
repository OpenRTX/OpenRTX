/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NVMEM_ACCESS_H
#define NVMEM_ACCESS_H

#include <stdint.h>
#include "interfaces/nvmem.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Obtain the descriptor of a given nonvolatile memory area.
 *
 * @param index: index of the nonvolatile memory area.
 * @return a pointer to the memory descriptor or NULL if the requested
 * descriptor does not exist.
 */
const struct nvmDescriptor *nvm_getDesc(const uint32_t index);

/**
 * Obtain the descriptor of a given nonvolatile memory area partition.
 * The partition index zero represents the entire NVM area, while actual
 * partitions information are accessed with an index from 1 to nbPart.
 *
 *
 * @param idx: index of the nonvolatile memory area.
 * @param part: index of the partition within the memory area.
 * @param pInfo: pointer to a nvmPartition data structure.
 * @return zero on success, a negative error code otherwise.
 */
int nvm_getPart(const uint32_t idx, const uint32_t part,
                struct nvmPartition *pInfo);

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

#ifdef __cplusplus
}
#endif

#endif

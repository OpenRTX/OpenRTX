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

#ifndef NVMEM_ACCESS_H
#define NVMEM_ACCESS_H

#include <interfaces/nvmem.h>
#include <stdint.h>
#include <errno.h>

/**
 * Perform a byte-aligned read operation on an NVM area.
 *
 * @param area: pointer to the NVM are descriptor.
 * @param address: start address for the read operation.
 * @param data: pointer to a buffer where to store the data read.
 * @param len: number of bytes to read.
 * @return zero on success, a negative error code otherwise.
 */
int nvmArea_read(const struct nvmArea *area, uint32_t address, void *data, size_t len);

/**
 * Perform a byte-aligned write operation on an NVM area. If the underlying
 * device requires state syncing, a sync operation is performed at the end of
 * the write.
 *
 * @param area: pointer to the NVM are descriptor.
 * @param address: start address for the write operation.
 * @param data: pointer to a buffer containing the data to write.
 * @param len: number of bytes to write.
 * @return zero on success, a negative error code otherwise.
 */
int nvmArea_write(const struct nvmArea *area, uint32_t address, const void *data,
                  size_t len);

/**
 * Perform an erase operation on an NVM area. Acceptable start address and size
 * depend on the NVM device the area belongs to.
 *
 * @param area: pointer to the NVM are descriptor.
 * @param address: start address for the erase operation.
 * @param size: size of the area to be erased.
 * @return zero on success, a negative error code otherwise.
 */
int nvmArea_erase(const struct nvmArea *area, uint32_t address, size_t size);

/**
 * Get the parameters of the NVM device associated to a memory area.
 *
 * @param area: pointer to the NVM are descriptor.
 * @return pointer to the device parameters' data structure.
 */
static inline const struct nvmParams *nvmArea_params(const struct nvmArea *area)
{
    const struct nvmDevice *dev = area->dev;

    return dev->api->params(dev);
}

/**
 * Perform a byte-aligned read operation on an NVM area partition.
 *
 * @param area: pointer to the NVM are descriptor.
 * @param pNum: partition number.
 * @param address: start address for the read operation.
 * @param data: pointer to a buffer where to store the data read.
 * @param len: number of bytes to read.
 * @return zero on success, a negative error code otherwise.
 */
static inline int nvmArea_readPartition(const struct nvmArea *area,
                                        const uint32_t pNum, uint32_t offset,
                                        void *data, size_t len)
{
    const struct nvmPartition *partition = &(area->partitions[pNum]);
    const size_t startAddr = area->startAddr + partition->offset + offset;

    return nvmArea_read(area, startAddr, data, len);
}

/**
 * Perform a byte-aligned write operation on an NVM area partition. If the
 * underlying device requires state syncing, a sync operation is performed at
 * the end of the write.
 *
 * @param area: pointer to the NVM are descriptor.
 * @param pNum: partition number.
 * @param address: start address for the write operation.
 * @param data: pointer to a buffer containing the data to write.
 * @param len: number of bytes to write.
 * @return zero on success, a negative error code otherwise.
 */
static inline int nvmArea_writePartition(const struct nvmArea *area,
                                         const uint32_t pNum, uint32_t offset,
                                         const void *data, size_t len)
{
    const struct nvmPartition *partition = &(area->partitions[pNum]);
    const size_t startAddr = area->startAddr + partition->offset + offset;

    return nvmArea_write(area, startAddr, data, len);
}

/**
 * Perform an erase operation on an NVM area partition. Acceptable start address
 * and size depend on the NVM device the area belongs to.
 *
 * @param area: pointer to the NVM are descriptor.
 * @param pNum: partition number.
 * @param address: start address for the erase operation.
 * @param size: size of the area to be erased.
 * @return zero on success, a negative error code otherwise.
 */
static inline int nvmArea_erasePartition(const struct nvmArea *area,
                                         const uint32_t pNum, uint32_t offset,
                                         size_t size)
{
    const struct nvmPartition *partition = &(area->partitions[pNum]);
    const size_t startAddr = area->startAddr + partition->offset + offset;

    return nvmArea_erase(area, startAddr, size);
}

#endif

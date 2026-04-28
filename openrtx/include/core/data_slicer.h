/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DATA_SLICER_H
#define DATA_SLICER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "core/utils.h"

/**
 * \file
 * This file contains a data sclicer implementation: given a block of data of
 * known size and a slice size, it allows to fraction and reconstruct the data
 * block to/from sub-blocks. Each sub-block contains also metadata for
 * integrity check.
 */

struct dataSlicer {
    size_t block_size; ///< Size of the complete data block
    size_t slice_size; ///< Size of each slice (excluding metadata)
};

/**
 * Obtain the number of slices needed to slice the whole data block.
 *
 * @param slicer: pointer to data slicer object
 * @return number of slices required to slice the full block or 0 in case of
 * invalid parameters
 */
size_t dataSlicer_numSlices(const struct dataSlicer *slicer);

/**
 * Obtain the size of a slice, including metadata overhead.
 *
 * @param slicer: pointer to data slicer object
 * @return slice size in bytes or 0 in case of invalid parameters
 */
size_t dataSlicer_ioSliceSize(const struct dataSlicer *slicer);

/**
 * Get the checksum of a data slice.
 *
 * @param slice: pointer to slice data
 * @return slice data checksum
 */
uint8_t dataSlicer_sliceChecksum(const void *slice);

/**
 * Given the byte offset from the beginning of the data block and a size in
 * bytes, compute the index of the first slice updated and the number of slices
 * updated.
 *
 * For example (slices of 4 bytes, data block of 13 bytes), set first to 6 and
 * length to 5 to indicate that bytes 6 to 10 (included) where updated. The
 * function will set first to 1 and length to 2 to indicate that the slices 1
 * to 2 have been updated.
 *
 * @param slicer: pointer to data slicer object
 * @param offset: offset from the beginning of the data block, set to the index
 * of the corresponding slice
 * @param length: data size in bytes, set to the corresponding number of slices
 */
void dataSlicer_offsetToSlices(const struct dataSlicer *slicer, size_t *offset,
                               size_t *size);

/**
 * Get a slice from the source structure, accounting for the header
 *
 * @param slicer: pointer to data slicer object
 * @param src: pointer to the data block to slice
 * @param slice: pointer to the memory location where the slice should be written
 * @param index: index of the slice to get
 */
void dataSlicer_getSlice(const struct dataSlicer *slicer, const void *src,
                         void *slice, size_t index);

/**
 * Insert a slice in a data block
 *
 * @param slicer: pointer to data slicer object
 * @param dest: pointer to the data block where to insert the slice
 * @param slice: pointer to the slice to insert
 * @param index: index of the slice to insert
 * @return 0 if successful, negative error code otherwise
 */
int dataSlicer_putSlice(const struct dataSlicer *slicer, void *dest,
                        const void *slice, size_t index);

/**
 * Reconstruct a data block from an array of slices
 *
 * @param slicer: pointer to data slicer object
 * @param dest: pointer to the data block to reconstruct
 * @param slices: pointer to an array of slices
 * @param nb_slices: number of slices in the array of slices
 * @return 0 if successful, negative error code otherwise
 */
int dataSlicer_reassemble(const struct dataSlicer *slicer, void *dest,
                          const void *slices[], size_t nb_slices);

/**
 * Convert a slice index and number of slices to a byte index and number of
 * bytes.
 *
 * @param slicer: pointer to data slicer object
 * @param first: pointer to the index first slice, will be set to index of
 * first byte
 * @param len: pointer to the number of slices, will be set to the number of
 * bytes
 * @return 0 if successful, negative error code otherwise
 */
void dataSlicer_slice_slicesToOffset(const struct dataSlicer *slicer,
                                     size_t *first, size_t *len);

struct slicedStorage {
    struct dataSlicer slicer;
    uint8_t *crc_cache;
};

/**
 * Instantiate a sliced storage module.
 *
 * @param name: module name
 * @param data_size: size of the data block to manage
 * @param slice_size: slice size, in bytes
 */
#define SLICED_STORAGE_DECLARE(name, data_size, slice_size)              \
    static uint8_t crcCache_##name[DIV_ROUND_UP(data_size, slice_size)]; \
    static const struct slicedStorage name = {                           \
        .slicer = { data_size, slice_size },                             \
        .crc_cache = crcCache_##name,                                    \
    };

/**
 * Load a data block from slices stored in an NVM partition.
 *
 * @param slicer: pointer to data slicer object
 * @param dev: NVM device index
 * @param part: NVM partition index
 * @param data: pointer to the data block to reconstruct
 * @return 0 if successful, a negative error code otherwise
 */
int slicedStorage_nvmLoad(const struct slicedStorage *st, const uint32_t dev,
                          const uint32_t part, void *data);

/**
 * Write a data block storing slices in an NVM partition.
 *
 * This function does differential writing: the data block is compared against
 * a reference version and only the slices containing actual changes will be
 * written. If the pointer to reference block is set to NULL, all the slices
 * will be written.
 *
 * @param slicer: pointer to data slicer object
 * @param dev: NVM device index
 * @param part: NVM partition index
 * @param data: pointer to the data block to write
 * @param ref: pointer to the reference data block
 * @return 0 if successful, a negative error code otherwise
 */
int slicedStorage_nvmStore(const struct slicedStorage *st, const uint32_t dev,
                           const uint32_t part, void *data, bool fullWrite);
#endif

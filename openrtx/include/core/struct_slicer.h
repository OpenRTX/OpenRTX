/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _STRUCT_SLICER_H_
#define _STRUCT_SLICER_H_

#include <stddef.h>
#include <stdint.h>

/**
 * \file
 * This file implements a structure slicer. Given a structure size and a slice
 * size, it handles the comparison of structures with the slices to update, the
 * reconstruction of the structure from slices.
 *
 */

typedef struct {
    size_t struct_size; ///< Size of the complete structure to slice
    size_t slice_size;  ///< Size of each slice (including header)
} struct_slicer_config_t;

/**
 * Initialize a structure slicer config using a set of parameters
 *
 * @param struct_size size of the structure to slice
 * @param slice_size size of one slice (including header)
 * @param config pointer to a struct_slicer_config_t to initialize
 * @return 0 if successful, negative error code otherwise
 */
int struct_slicer_init(size_t struct_size, size_t slice_size,
                       struct_slicer_config_t *config);

/**
 * Returns the number of slices needed to slice the whole structure
 *
 * @param config pointer to the structure slicer configuration
 * @return number of slices required to slice the full structure (0 in case of invalid parameters)
 */
unsigned int struct_slicer_nb_slices(const struct_slicer_config_t *config);

/**
 * Given the byte offset from an update in a struct and a number of bytes
 * updated, returns the index of the first slice updated and the number of
 * slices updated.
 *
 * For example (slices of 4 bytes, structure of 13 bytes), set first to 6 and
 * length to 5 to indicate that bytes 6 to 10 (included) where updated. The
 * function will set first to 1 and length to 2 to indicate that the slices
 * 1 to 2 have been updated.
 *
 * @param config pointer to the structure slicer configuration
 * @param first inout pointer to the index of the first byte updated, set
 *              to the index of the corresponding slice
 * @param length inout pointer to the number of bytes updated, set to the
                 corresponding number of slices
 * @return 0 if successful, negative error code otherwise
 */
int struct_slicer_slices_updated(const struct_slicer_config_t *config,
                                 size_t *first, size_t *length);

/**
 * Get a slice from the source structure, accounting for the header
 *
 * @param config pointer to the structure slicer configuration
 * @param structure pointer to the source structure to slice
 * @param index index of the slice to get
 * @param slice pointer to the memory location where the slice should be written
 * @return 0 if successful, negative error code otherwise
 */
int struct_slicer_get_slice(const struct_slicer_config_t *config,
                            const uint8_t *structure, size_t index,
                            uint8_t *slice);

/**
 * Reconstructs a structure from an array of slices
 *
 * @param config pointer to the structure slicer configuration
 * @param structure pointer to the structure to reconstruct
 * @param slices pointer to an array of slices
 * @param nb_slices number of slices in the array of slices
 * @return 0 if successful, negative error code otherwise
 */
int struct_slicer_get_struct(const struct_slicer_config_t *config,
                             uint8_t *structure, const uint8_t *slices[],
                             size_t nb_slices);

/**
 * Inserts a slice of data in a structure
 *
 * @param config pointer to the structure slicer configuration
 * @param structure pointer to the structure in which to insert the slice
 * @param slice pointer to the slice to insert
 * @param index index of the slice to insert
 * @return 0 if successful, negative error code otherwise
 */
int struct_slicer_insert(const struct_slicer_config_t *config,
                         uint8_t *structure, const uint8_t *slice,
                         size_t index);

/**
 * Returns the size of the header added to each slice
 *
 * @return unsigned int size of the header added to each slice
 */
unsigned int struct_slicer_header_size();

/**
 * Converts a pair of slice index and number of slices to a pair of byte index
 * and number of bytes.
 *
 * @param config pointer to the structure slicer configuration
 * @param first pointer to the index first slice, will be set to index of first byte upon return
 * @param len pointer to the number of slices, will be set to the number of bytes upon return
 * @return 0 if successful, negative error code otherwise
 */
int struct_slicer_slice_idx2bytes(const struct_slicer_config_t *config,
                                  size_t *first, size_t *len);

#endif
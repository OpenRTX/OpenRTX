/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/struct_slicer.h"
#include "core/utils.h"

#include <stddef.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct {
    uint8_t crc8;
} __attribute__((packed)) slice_header_t;

uint8_t crc8(const uint8_t *data, const size_t len)
{
    uint8_t crc = 0xff;

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];

        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}

int struct_slicer_init(size_t struct_size, size_t slice_size,
                       struct_slicer_config_t *config)
{
    config->struct_size = struct_size;
    config->slice_size = slice_size;

    return 0;
}

unsigned int struct_slicer_nb_slices(const struct_slicer_config_t *config)
{
    if (config == NULL)
        return 0;

    // subtract header size to slice size
    return DIV_ROUND_UP(config->struct_size,
                        config->slice_size - sizeof(slice_header_t));
}

int struct_slicer_slices_updated(const struct_slicer_config_t *config,
                                 size_t *first, size_t *length)
{
    if ((config == NULL) || (first == NULL) || (length == NULL))
        return -EINVAL;

    // Index of slice containing last byte
    size_t last = (*first + *length - 1)/(config->slice_size - sizeof(slice_header_t));

    // Index of slice containing first byte
    *first = *first / (config->slice_size - sizeof(slice_header_t));

    *length = last - *first + 1;
    return 0;
}

int struct_slicer_get_slice(const struct_slicer_config_t *config,
                            const uint8_t *structure, size_t index,
                            uint8_t *slice)
{
    size_t struct_offset = index
                         * (config->slice_size - sizeof(slice_header_t));
    size_t copy_length = MIN(config->struct_size - struct_offset,
                             config->slice_size - sizeof(slice_header_t));
    memcpy(slice + sizeof(slice_header_t), structure + struct_offset,
           copy_length);

    if (copy_length < config->slice_size - sizeof(slice_header_t))
        memset(slice + 2 + copy_length, 0,
               config->slice_size - sizeof(slice_header_t) - copy_length);

    // Compute CRC over data content, not including zero padding or header
    slice[0] = crc8(slice + sizeof(slice_header_t), copy_length);

    return 0;
}

int struct_slicer_get_struct(const struct_slicer_config_t *config,
                             uint8_t *structure, const uint8_t **slices,
                             size_t nb_slices)
{
    // Check that we have correct amount of slicers
    if (nb_slices != struct_slicer_nb_slices(config))
        return -EINVAL;

    for (size_t i = 0; i < nb_slices; i++) {
        int ret = struct_slicer_insert(config, structure, slices[i], i);
        if (ret < 0)
            return ret;
    }

    return 0;
}

int struct_slicer_insert(const struct_slicer_config_t *config,
                         uint8_t *structure, const uint8_t *slice, size_t index)
{
    if (config == NULL)
        return -EINVAL;

    // Check that the crc is correct
    size_t struct_offset =
        index
        * (config->slice_size
           - sizeof(slice_header_t)); // Offset of slice in struct
    size_t length = MIN(config->struct_size - struct_offset,
                        config->slice_size - sizeof(slice_header_t));

    if (slice[0] != crc8(slice + sizeof(slice_header_t), length))
        return -EINVAL;

    memcpy(structure + struct_offset, slice + sizeof(slice_header_t), length);

    return 0;
}

unsigned int struct_slicer_header_size()
{
    return sizeof(slice_header_t);
}

int struct_slicer_slice_idx2bytes(const struct_slicer_config_t *config,
                                  size_t *first, size_t *len)
{
    if (config == NULL)
        return -EINVAL;
    *first *= config->slice_size - sizeof(slice_header_t);
    *len *= config->slice_size - sizeof(slice_header_t);

    return 0;
}
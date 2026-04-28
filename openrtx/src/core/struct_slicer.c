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
#include "core/crc.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define DATA_LENGTH(config) (config->slice_size - sizeof(struct slice))

struct slice {
    uint8_t crc8;
    uint8_t data[];
} __attribute__((packed));

unsigned int struct_slicer_nb_slices(const struct struct_slicer_config *config)
{
    if (config == NULL)
        return 0;

    // subtract header size to slice size
    return DIV_ROUND_UP(config->struct_size, DATA_LENGTH(config));
}

int struct_slicer_slices_updated(const struct struct_slicer_config *config,
                                 size_t *first, size_t *length)
{
    if ((config == NULL) || (first == NULL) || (length == NULL))
        return -EINVAL;

    // Index of slice containing last byte
    size_t last = (*first + *length - 1) / DATA_LENGTH(config);

    // Index of slice containing first byte
    *first = *first / DATA_LENGTH(config);

    *length = last - *first + 1;
    return 0;
}

int struct_slicer_get_slice(const struct struct_slicer_config *config,
                            const uint8_t *structure, size_t index,
                            void *slice)
{
    struct slice *s = (struct slice *)slice;
    const size_t struct_offset = index * DATA_LENGTH(config);
    const size_t copy_len = MIN(config->struct_size - struct_offset,
                                DATA_LENGTH(config));

    memcpy(s->data, structure + struct_offset, copy_len);

    if (copy_len < DATA_LENGTH(config))
        memset(s->data + copy_len, 0, DATA_LENGTH(config) - copy_len);

    // Compute CRC over data content, not including zero padding or header
    s->crc8 = crc8(s->data, copy_len);

    return 0;
}

int struct_slicer_get_struct(const struct struct_slicer_config *config,
                             uint8_t *structure, const void *slices[],
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

int struct_slicer_insert(const struct struct_slicer_config *config,
                         uint8_t *structure, const void *slice, size_t index)
{
    if (config == NULL)
        return -EINVAL;

    struct slice *s = (struct slice *)slice;

    // Offset of slice in struct
    size_t struct_offset = index * DATA_LENGTH(config);
    size_t length = MIN(config->struct_size - struct_offset,
                        DATA_LENGTH(config));

    // Check that the crc is correct
    if (s->crc8 != crc8(s->data, length))
        return -EINVAL;

    memcpy(structure + struct_offset, s->data, length);

    return 0;
}

unsigned int struct_slicer_header_size()
{
    return sizeof(struct slice);
}

int struct_slicer_slice_idx2bytes(const struct struct_slicer_config *config,
                                  size_t *first, size_t *len)
{
    if (config == NULL)
        return -EINVAL;

    *first *= DATA_LENGTH(config);
    *len *= DATA_LENGTH(config);

    return 0;
}
/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/data_slicer.h"
#include "core/utils.h"
#include "core/nvmem_access.h"
#include <stddef.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include "core/crc.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX_SLICE_SIZE 64

struct slice {
    uint8_t crc8;
    uint8_t data[];
} __attribute__((packed));

size_t dataSlicer_numSlices(const struct dataSlicer *slicer)
{
    if (slicer == NULL)
        return 0;

    return DIV_ROUND_UP(slicer->block_size, slicer->slice_size);
}

size_t dataSlicer_ioSliceSize(const struct dataSlicer *slicer)
{
    if (slicer == NULL)
        return 0;

    return slicer->slice_size + sizeof(struct slice);
}

uint8_t dataSlicer_sliceChecksum(const void *slice)
{
    const struct slice *sl = slice;
    return sl->crc8;
}

void dataSlicer_offsetToSlices(const struct dataSlicer *slicer, size_t *offset,
                               size_t *size)
{
    // Index of slice containing last byte
    size_t last = (*offset + *size - 1) / slicer->slice_size;

    // Index of slice containing first byte
    *offset = *offset / slicer->slice_size;
    *size = last - *offset + 1;
}

void dataSlicer_getSlice(const struct dataSlicer *slicer, const void *src,
                         void *slice, size_t index)
{
    struct slice *s = (struct slice *)slice;
    size_t offset = index * slicer->slice_size;
    size_t copyLen = MIN(slicer->block_size - offset, slicer->slice_size);

    memcpy(s->data, ((uint8_t *)src) + offset, copyLen);

    if (copyLen < slicer->slice_size)
        memset(s->data + copyLen, 0, slicer->slice_size - copyLen);

    s->crc8 = crc_oneWire(s->data, copyLen);
}

int dataSlicer_putSlice(const struct dataSlicer *slicer, void *dest,
                        const void *slice, size_t index)
{
    struct slice *s = (struct slice *)slice;

    // Offset of slice in struct
    size_t offset = index * slicer->slice_size;
    size_t length = MIN(slicer->block_size - offset, slicer->slice_size);

    // Check that the crc is correct
    if (s->crc8 != crc_oneWire(s->data, length))
        return -EIO;

    memcpy(((uint8_t *)dest) + offset, s->data, length);

    return 0;
}

int dataSlicer_reassemble(const struct dataSlicer *slicer, void *dest,
                          const void *slices[], size_t nb_slices)
{
    // Check that we have correct amount of slicers
    if (nb_slices != dataSlicer_numSlices(slicer))
        return -EINVAL;

    for (size_t i = 0; i < nb_slices; i++) {
        int ret = dataSlicer_putSlice(slicer, dest, slices[i], i);
        if (ret < 0)
            return ret;
    }

    return 0;
}

void dataSlicer_slice_slicesToOffset(const struct dataSlicer *slicer,
                                     size_t *first, size_t *len)
{
    *first *= slicer->slice_size;
    *len *= slicer->slice_size;
}

int slicedStorage_nvmLoad(const struct slicedStorage *st, const uint32_t dev,
                          const uint32_t part, void *data)
{
    const size_t readSize = dataSlicer_ioSliceSize(&st->slicer);

    if (readSize >= MAX_SLICE_SIZE)
        return -ENOMEM;

    for (size_t i = 0; i < dataSlicer_numSlices(&st->slicer); i++) {
        uint8_t tmp[MAX_SLICE_SIZE];
        size_t offset = i * readSize;
        int ret;

        ret = nvm_read(dev, part, offset, tmp, readSize);
        if (ret < 0)
            return ret;

        ret = dataSlicer_putSlice(&st->slicer, data, tmp, i);
        if (ret < 0)
            return ret;

        st->crc_cache[i] = dataSlicer_sliceChecksum(tmp);
    }

    return 0;
}

int slicedStorage_nvmStore(const struct slicedStorage *st, const uint32_t dev,
                           const uint32_t part, void *data, bool fullWrite)
{
    const size_t writeSize = dataSlicer_ioSliceSize(&st->slicer);

    if (writeSize >= MAX_SLICE_SIZE)
        return -ENOMEM;

    for (size_t i = 0; i < dataSlicer_numSlices(&st->slicer); i++) {
        uint8_t tmp[MAX_SLICE_SIZE];

        dataSlicer_getSlice(&st->slicer, data, tmp, i);

        if (!fullWrite && (st->crc_cache[i] == dataSlicer_sliceChecksum(tmp)))
            continue;

        int ret = nvm_write(dev, part, (i * writeSize), tmp, writeSize);
        if (ret < 0)
            return ret;
    }

    return 0;
}

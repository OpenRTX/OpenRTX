/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/vfo.h"
#include "core/nvmem_access.h"
#include "core/struct_slicer.h"
#include "core/cps.h"
#include "core/nvmem_access.h"

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/**
 * Compare two buffers of length len, returning the first index at which
 * buffers differ
 *
 * @param b1 first buffer
 * @param b2 second buffer
 * @param len length of buffers
 * @return size_t index at which the buffers differ. Returns len for identical
 * buffers
 */
size_t compare_buffers(const void *b1, const void *b2, const size_t len)
{
    const uint8_t *arr1 = (uint8_t *)b1;
    const uint8_t *arr2 = (uint8_t *)b2;

    size_t i = 0;
    while ((arr1[i] == arr2[i]) && (i < len))
        i++;

    return i;
}

int vfo_slicer_init(vfo_slicer_t *s, const int nvm_dev, const int nvm_part,
                    const size_t eeeprom_entry_size, const uint16_t vfo_version)
{
    s->nvm_dev = nvm_dev;
    s->nvm_part = nvm_part;
    s->entry_size = eeeprom_entry_size;
    s->version = vfo_version;
    s->loaded = false;
    s->full_write_needed = false;

    int ret = struct_slicer_init(sizeof(vfo_t), eeeprom_entry_size,
                                 &(s->slicer_conf));
    if (ret < 0)
        return ret;

    return 0;
}

int vfo_slicer_load(vfo_slicer_t *s, channel_t *vfo)
{
    if (s->loaded)
        *vfo = s->saved_vfo.vfo;

    unsigned int n = struct_slicer_nb_slices(&(s->slicer_conf));
    bool read_error = false;

    uint8_t *tmp = (uint8_t *)malloc(s->entry_size);
    if (tmp == NULL)
        return -ENOMEM;

    // Read the slices from memory, insert them in the structure
    for (size_t i = 0; i < n; i++) {
        int ret = nvm_read(s->nvm_dev, s->nvm_part, i, tmp, s->entry_size);
        if (ret == s->entry_size)
            ret = struct_slicer_insert(&(s->slicer_conf),
                                       (uint8_t *)&(s->saved_vfo), tmp, i);
        if (ret < 0) {
            read_error = true;
            break;
        }
    }

    // Check that we read VFO correctly and version matches
    if (read_error || (s->saved_vfo.version != s->version)) {
        s->saved_vfo.version = s->version;
        s->saved_vfo.vfo = cps_getDefaultChannel();
        s->full_write_needed = true;
    }

    *vfo = s->saved_vfo.vfo;
    free(tmp);
    s->loaded = true;

    return 0;
}

int vfo_slicer_save(vfo_slicer_t *s, const channel_t *vfo)
{
    if (s->full_write_needed) {
        uint8_t *tmp = (uint8_t *)malloc(s->entry_size);
        if (tmp == NULL) {
            return -ENOMEM;
        }

        s->saved_vfo.vfo = *vfo;

        unsigned int n = struct_slicer_nb_slices(&(s->slicer_conf));
        for (size_t i = 0; i < n; i++) {
            int ret = struct_slicer_get_slice(
                &(s->slicer_conf), (const uint8_t *)&(s->saved_vfo), i, tmp);
            if (ret == 0)
                ret = nvm_write(s->nvm_dev, s->nvm_part, i, tmp, s->entry_size);

            if (ret < 0) {
                free(tmp);
                return ret;
            }
        }

        s->full_write_needed = false;
    } else {
        vfo_t buffer = {
            .version = s->version,
            .vfo = *vfo,
        };

        size_t n = compare_buffers(&(s->saved_vfo), &buffer,
                                   sizeof(vfo_t));
        if (n == sizeof(vfo_t))
            return 0;

        uint8_t *tmp = (uint8_t *)malloc(s->entry_size);
        if (tmp == NULL)
            return -ENOMEM;

        while (n != sizeof(vfo_t)) {
            if (n < 0)
                n = -n;

            // Count number of different bytes
            size_t i = n;
            while (((uint8_t *)&(s->saved_vfo))[i] != ((uint8_t *)(vfo))[i])
                i++;

            size_t len = i - n;
            // Get the slices to write to memory
            struct_slicer_slices_updated(&(s->slicer_conf), &n, &len);
            for (size_t slice = n; slice < n + len; slice++) {
                int ret = struct_slicer_get_slice(
                    &(s->slicer_conf), (const uint8_t *)&buffer, slice, tmp);
                if (ret == 0)
                    ret = nvm_write(s->nvm_dev, s->nvm_part, slice, tmp,
                                    s->entry_size);
                if (ret < 0) {
                    free(tmp);
                    return ret;
                }
            }

            // Copy to source struct the bytes that were written back to memory
            struct_slicer_slice_idx2bytes(&(s->slicer_conf), (size_t *)&n,
                                          &len);
            memcpy((uint8_t *)(&(s->saved_vfo)) + n, (uint8_t *)(&buffer) + n,
                   len);

            n = compare_buffers(&(s->saved_vfo), &buffer, sizeof(vfo_t));
        }
    }

    return 0;
}

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _VFO_SLICER_H_
#define _VFO_SLICER_H_

#include "core/struct_slicer.h"
#include "core/cps.h"

struct vfo {
    uint16_t version;
    channel_t vfo;
} __attribute__((packed));

struct vfo_storage {
    int nvm_dev;
    int nvm_part;
    size_t entry_size;
    uint16_t version;
    struct struct_slicer_config slicer_conf;
    struct vfo saved_vfo;
    bool loaded;
    bool full_write_needed;
};

/**
 * Initialize a vfo_slicer_t structure to save and load VFO in an EEEPROM. The
 * VFO will be stored as slices of the structure, only saving the slices that
 * changed.
 *
 * @return 0 if successful, negative error code otherwise
 */
int vfo_initStorage(struct vfo_storage *s, const int nvm_dev, const int nvm_part,
                    const size_t eeeprom_entry_size,
                    const uint16_t vfo_version);

/**
 * Load device VFO from EEEPROM.
 *
 * @param s pointer to an initialized vfo_slicer_t structure
 * @param settings pointer to a settings_t structure where to write the
 * loaded settings
 * @return 0 if successful, negative error code otherwise
 */
int vfo_load(struct vfo_storage *s, channel_t *vfo);

/**
 * Save device settings to non-volatile memory.
 * Will not perform any actual write if the settings haven't changed.
 *
 * @return 0 if successful, negative error code otherwise
 */
int vfo_save(struct vfo_storage *s, const channel_t *vfo);

/**
 * Get the storage overhead over the size of the vfo itself (channel_t).
 * If s is a pointer to a valid vfo_storage struct, the overhead is computed by
 * taking the number of slices into account. If s is a NULL pointer, the
 * overhead is computed over a single slice.
 *
 * @param s NULL or pointer to a vfo_storage struct
 * @return size_t storage overhead in bytes
 */
size_t vfo_storageOverhead(const struct vfo_storage *s);

#endif
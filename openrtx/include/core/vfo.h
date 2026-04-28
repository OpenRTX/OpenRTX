/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _VFO_SLICER_H_
#define _VFO_SLICER_H_

#include "core/struct_slicer.h"
#include "core/cps.h"

typedef struct {
    uint16_t version;
    channel_t vfo;
} __attribute__((packed)) vfo_t;

typedef struct {
    int nvm_dev;
    int nvm_part;
    size_t entry_size;
    uint16_t version;
    struct_slicer_config_t slicer_conf;
    vfo_t saved_vfo;
    bool loaded;
    bool full_write_needed;
} vfo_slicer_t;

/**
 * Initialize a vfo_slicer_t structure to save and load VFO in an EEEPROM. The
 * VFO will be stored as slices of the structure, only saving the slices that
 * changed.
 *
 * @return 0 if successful, negative error code otherwise
 */
int vfo_slicer_init(vfo_slicer_t *s, const int nvm_dev, const int nvm_part,
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
int vfo_slicer_load(vfo_slicer_t *s, channel_t *vfo);

/**
 * Save device settings to non-volatile memory.
 * Will not perform any actual write if the settings haven't changed.
 *
 * @return 0 if successful, negative error code otherwise
 */
int vfo_slicer_save(vfo_slicer_t *s, const channel_t *vfo);

#endif
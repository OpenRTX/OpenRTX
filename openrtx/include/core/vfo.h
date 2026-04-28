/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VFO_H
#define VFO_H

/**
 * Initialize VFO data storage.
 *
 * @param nvm_dev: NVM device number for VFO storage
 * @param nvm_part: partition number for VFO storage
 */
void vfo_init(const uint32_t nvm_dev, const uint32_t nvm_part);

/**
 * Load device VFO data from nonvolatile memory.
 *
 * @param vfo: pointer to VFO data.
 * @return 0 if successful, a negative error code otherwise
 */
int vfo_load(channel_t *vfo);

/**
 * Save VFO data to nonvolatile memory.
 * Will not perform any actual write if VFO data haven't changed.
 *
 * @param vfo: pointer to VFO data.
 * @return 0 if successful, a negative error code otherwise
 */
int vfo_save(const channel_t *vfo);

#endif

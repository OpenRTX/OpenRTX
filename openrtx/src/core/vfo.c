/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <string.h>
#include "core/nvmem_access.h"
#include "core/data_slicer.h"
#include "core/cps.h"
#include "core/vfo.h"

struct vfo_data {
    uint16_t version;
    channel_t vfo;
} __attribute__((packed));

struct vfo_storage {
    uint32_t dev;
    uint32_t part;
    bool full_write_needed;
};

SLICED_STORAGE_DECLARE(slicedVfo, sizeof(struct vfo_data), 16)
static struct vfo_storage storage;

void vfo_init(const uint32_t nvm_dev, const uint32_t nvm_part)
{
    storage.dev = nvm_dev;
    storage.part = nvm_part;
    storage.full_write_needed = false;
}

int vfo_load(channel_t *vfo)
{
    struct vfo_data saved_vfo;
    int err = 0;

    err = slicedStorage_nvmLoad(&slicedVfo, storage.dev, storage.part,
                                &saved_vfo);
    if (err)
        return err;

    if (saved_vfo.version != CPS_VERSION_NUMBER) {
        *vfo = cps_getDefaultChannel();
        storage.full_write_needed = true;
        return -1;
    }

    memcpy(vfo, &saved_vfo.vfo, sizeof(channel_t));

    return 0;
}

int vfo_save(const channel_t *vfo)
{
    const struct nvmInfo *ni = nvm_getDevInfo(storage.dev);
    struct vfo_data vfo_data;
    int ret = 0;

    vfo_data.version = CPS_VERSION_NUMBER;
    vfo_data.vfo = *vfo;

    if (storage.full_write_needed && (ni->device_info & NVM_ERASE)) {
        ret = nvm_erasePartition(storage.dev, storage.part);
        if (ret < 0)
            return ret;
    }

    ret = slicedStorage_nvmStore(&slicedVfo, storage.dev, storage.part,
                                 &vfo_data, storage.full_write_needed);
    if (!ret)
        storage.full_write_needed = false;

    return ret;
}

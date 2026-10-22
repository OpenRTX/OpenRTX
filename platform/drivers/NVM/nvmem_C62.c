/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/printk.h>
#include "interfaces/nvmem.h"
#include "calibration/calib_C62.h"

/**
 * Read from the stock firmware's settings journal partition.
 *
 * @param ctx: flash area of the journal.
 * @param off: offset in the partition.
 * @param buf: destination buffer.
 * @param len: number of bytes to read.
 * @return zero on success, a negative error code otherwise.
 */
static int journalRead(void *ctx, uint32_t off, void *buf, size_t len)
{
    return flash_area_read((const struct flash_area *)ctx, off, buf, len);
}

void nvm_init()
{
}

void nvm_terminate()
{
}

void nvm_readCalibData(void *buf)
{
    struct C62Calib *cal = (struct C62Calib *)buf;
    const struct flash_area *journal;
    int ret = -1;

    if (flash_area_open(FIXED_PARTITION_ID(factory_cal), &journal) == 0) {
        ret = calibC62_read(journalRead, (void *)journal, journal->fa_size,
                            cal);
        flash_area_close(journal);
    }

    if (ret < 0) {
        calibC62_defaults(cal);
        printk("TX calibration: stock defaults, no valid factory data\n");
    } else {
        printk("TX calibration: factory data, %u VHF and %u UHF points\n",
               cal->vhf.count, cal->uhf.count);
    }
}

void nvm_readHwInfo(hwInfo_t *info)
{
    (void)info;
}

int nvm_readVfoChannelData(channel_t *channel)
{
    (void)channel;

    return -1;
}

int nvm_readSettings(settings_t *settings)
{
    (void)settings;

    return -1;
}

int nvm_writeSettings(const settings_t *settings)
{
    (void)settings;

    return -1;
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    (void)settings;
    (void)vfo;

    return -1;
}

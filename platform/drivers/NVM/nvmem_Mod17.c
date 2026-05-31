/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "calibration/calibInfo_Mod17.h"
#include "drivers/NVM/flash_stm32.h"
#include "core/nvmem_access.h"
#include "drivers/NVM/eeep.h"
#include "interfaces/nvmem.h"
#include "core/settings.h"
#include "core/utils.h"
#include "core/vfo.h"
#include "core/crc.h"
#include <string.h>
#include <errno.h>

struct nvmCalRecord {
    uint16_t crc;
    uint16_t length;
    mod17Calib_t data;
} __attribute__((packed));

extern uint8_t _flash_eeep_start_;
mod17Calib_t mod17CalData; // Calibration data, to be saved and loaded
uint16_t calibCrc;

INTERNAL_FLASH_DEVICE_DEFINE(flashDevice)
EEEP_DEVICE_DEFINE(eeepDevice)

const struct nvmPartition eeepPartitions[] = {
    NVM_PART_DEFINE(0x0000, 1024), // First partition, sliced vfo
    NVM_PART_DEFINE(0x0400, 1024), // Second partition, settings
    NVM_PART_DEFINE(0x0800, 1024), // Third partition, calib data
};

const struct nvmDescriptor storage[] = {
    {
        .name = "Internal flash storage",
        .dev = (const struct nvmDevice *)&flashDevice,
        .baseAddr = (const size_t)(&_flash_eeep_start_),
        .size = 256 * 1024, // 256K, last two sectors
        .nbPart = 0,
        .partitions = NULL,
    },
    {
        .name = "Emulated EEPROM",
        .dev = (const struct nvmDevice *)&eeepDevice,
        .baseAddr = 0x00000000,
        .size = 0xFFFF,
        .nbPart = ARRAY_SIZE(eeepPartitions),
        .partitions = eeepPartitions,
    }
};

const struct nvmTable nvmTab = {
    .areas = storage,
    .nbAreas = ARRAY_SIZE(storage),
};

void nvm_init()
{
    stm32Flash_init(&flashDevice, 128 * 1024);
    eeep_init(&eeepDevice, 0, 0);

    /*
     * Settings: device 1, partition 2
     * VFO: device 1, partition 1
     */
    settings_initSingle(1, 2);
    vfo_init(1, 1);
    nvm_readCalibData(&mod17CalData);
}

void nvm_terminate()
{
    eeep_terminate(&eeepDevice);
    stm32Flash_terminate(&flashDevice);
}

void nvm_readCalibData(void *buf)
{
    struct nvmCalRecord record;

    memcpy(buf, &default_mod17Calib, sizeof(default_mod17Calib));
    nvm_read(1, 3, 0, &record, sizeof(record));

    if (record.length > sizeof(mod17Calib_t))
        return;

    if (record.crc != crc_ccitt(&record.data, record.length))
        return;

    memcpy(buf, &record.data, record.length);
    calibCrc = record.crc;
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

    return 0;
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    (void)settings;
    (void)vfo;

    return 0;
}

int nvm_writeCalibData(void *cal)
{
    struct nvmCalRecord record;

    memcpy(&record.data, cal, sizeof(mod17Calib_t));
    record.length = sizeof(mod17Calib_t);
    record.crc = crc_ccitt(&record.data, record.length);
    if (record.crc == calibCrc)
        return 0;

    return nvm_write(1, 3, 0, &record, sizeof(record));
}

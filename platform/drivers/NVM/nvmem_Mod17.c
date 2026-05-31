/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/cps.h"
#include "core/nvmem_access.h"
#include "core/utils.h"
#include "core/vfo.h"
#include "core/crc.h"
#include "core/settings.h"
#include "drivers/NVM/flash_stm32.h"
#include "drivers/NVM/eeep.h"
#include "interfaces/nvmem.h"
#include "calibration/calibInfo_Mod17.h"

#include <string.h>
#include <errno.h>

extern uint8_t _flash_eeep_start_;

mod17Calib_t mod17CalData;   // Calibration data, to be saved and loaded
struct vfo_storage vfo_storage;

INTERNAL_FLASH_DEVICE_DEFINE(flashDevice)
EEEP_DEVICE_DEFINE(eeepDevice)

static const size_t settings_part = 2;
static const size_t calib_part = 3;

const struct nvmPartition eeepPartitions[] =
{
    {
        .offset = 0x0000,   // First partition, sliced vfo
        .size = 1024
    },
    {
        .offset = 1024,     // Second partition, settings
        .size = 1024
    },
    {
        .offset = 2048,     // Third partition, calib data
        .size = 1024
    }
};

const struct nvmDescriptor storage[] =
{
    {
        .name       = "Internal flash storage",
        .dev        = (const struct nvmDevice *)&flashDevice,
        .baseAddr   = (const size_t)(&_flash_eeep_start_), // Defined in linker script
        .size       = 256*1024, // 256K, last two sectors
        .nbPart     = 0,
        .partitions = NULL,
    },
    {
        .name       = "Emulated EEPROM",
        .dev        = (const struct nvmDevice *)&eeepDevice,
        .baseAddr   = 0x00000000,
        .size       = 0xFFFF, // Virtual address is 16 bits
        .nbPart     = ARRAY_SIZE(eeepPartitions),
        .partitions = eeepPartitions,
    }
};

const struct nvmTable nvmTab = {
    .areas = storage,
    .nbAreas = ARRAY_SIZE(storage),
};

static const size_t crc_offset = 0;
static const size_t len_offset = sizeof(uint8_t);
static const size_t dat_offset = len_offset + sizeof(uint16_t);

void nvm_init()
{
    stm32Flash_init(&flashDevice, 128*1024);
    eeep_init(&eeepDevice, 0, 0);
    vfo_initStorage(&vfo_storage, 1, 1, 16, CPS_VERSION_NUMBER);
}

void nvm_terminate()
{
    stm32Flash_terminate(&flashDevice);
    eeep_terminate(&eeepDevice);
}

void nvm_readCalibData(void *buf)
{
    (void) buf;
}

void nvm_readHwInfo(hwInfo_t *info)
{
    (void) info;
}

int nvm_readVfoChannelData(channel_t *channel)
{
    return vfo_load(&vfo_storage, channel);
}

int nvm_readSettings(settings_t *settings)
{
    uint8_t crc = 0;
    uint16_t length = 0;
    *settings = default_settings;

    nvm_read(1, settings_part, crc_offset, &crc, 1); // Read settings CRC
    nvm_read(1, settings_part, len_offset, &length, 2); // Read settings length
    if(length > sizeof(settings_t)){
        return -E2BIG;
    }
    nvm_read(1, settings_part, dat_offset, settings, length);

    if(crc != crc8((const uint8_t *)settings, length))
    {
        *settings = default_settings;
    }

    mod17Calib_t cal = default_mod17Calib;

    nvm_read(1, calib_part, crc_offset, &crc, 1); // Read calib CRC
    nvm_read(1, calib_part, len_offset, &length, 2); // Read calib length
    if(length > sizeof(mod17Calib_t)){
        return -E2BIG;
    }
    nvm_read(1, calib_part, dat_offset, &cal, length);

    if(crc != crc8((const uint8_t *)&cal, length))
    {
        cal = default_mod17Calib;
    }

    mod17CalData = cal;

    return 0;
}

int nvm_writeSettings(const settings_t *settings)
{
    uint8_t crc = 0;
    uint16_t length = 0;

    // Settings
    nvm_read(1, settings_part, crc_offset, &crc, 1);
    nvm_read(1, settings_part, len_offset, &length, 2);

    if(length != sizeof(settings_t))
    {
        length = sizeof(settings_t);
        crc = crc8((const uint8_t *)settings, length);
        nvm_write(1, settings_part, crc_offset, &crc, 1);
        nvm_write(1, settings_part, len_offset, &length, 2);
        nvm_write(1, settings_part, dat_offset, settings, length);
    } else if(crc8((const uint8_t *)settings, length) != crc)
    {
        crc = crc8((const uint8_t *)settings, length);
        nvm_write(1, settings_part, crc_offset, &crc, 1);
        nvm_write(1, settings_part, dat_offset, settings, length);
    }

    // Calibration data
    nvm_read(1, calib_part, crc_offset, &crc, 1);
    nvm_read(1, calib_part, len_offset, &length, 2);

    if(length != sizeof(mod17Calib_t))
    {
        length = sizeof(mod17Calib_t);
        crc = crc8((const uint8_t *)&mod17CalData, length);
        nvm_write(1, calib_part, crc_offset, &crc, 1);
        nvm_write(1, calib_part, len_offset, &length, 2);
        nvm_write(1, calib_part, dat_offset, &mod17CalData, length);
    } else if(crc8((const uint8_t *)&mod17CalData, length) != crc)
    {
        crc = crc8((const uint8_t *)&mod17CalData, length);
        nvm_write(1, calib_part, crc_offset, &crc, 1);
        nvm_write(1, calib_part, dat_offset, &mod17CalData, length);
    }

    return 0;
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    int ret = vfo_save(&vfo_storage, vfo);
    if(ret < 0)
        return ret;

    return nvm_writeSettings(settings);
}

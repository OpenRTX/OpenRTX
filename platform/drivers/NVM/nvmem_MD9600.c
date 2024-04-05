/***************************************************************************
 *   Copyright (C) 2020 - 2024 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <wchar.h>
#include <string.h>
#include <interfaces/nvmem.h>
#include <interfaces/delays.h>
#include <calibInfo_MDx.h>
#include <utils.h>
#include "W25Qx.h"

W25Qx_DEVICE_DEFINE(eflash, 0x1000000)        // 16 MB, 128 Mbit
W25Qx_SECREG_DEFINE(cal1,   0x1000, 0x100)    // 256 byte
W25Qx_SECREG_DEFINE(cal2,   0x2000, 0x100)    // 256 byte

static const struct nvmDescriptor nvmDevices[] =
{
    {
        .name       = "External flash",
        .dev        = &eflash,
        .partNum    = 0,
        .partitions = NULL
    },
    {
        .name       = "Cal. data 1",
        .dev        = (const struct nvmDevice *) &cal1,
        .partNum    = 0,
        .partitions = NULL
    },
    {
        .name       = "Cal. data 2",
        .dev        = (const struct nvmDevice *) &cal2,
        .partNum    = 0,
        .partitions = NULL
    }
};


void nvm_init()
{
    W25Qx_init();
}

void nvm_terminate()
{
    W25Qx_terminate();
}

const struct nvmDescriptor *nvm_getDesc(const size_t index)
{
    if(index > 3)
        return NULL;

    return &nvmDevices[index];
}

void nvm_readCalibData(void *buf)
{
    (void) buf;
    return;
}

void nvm_readHwInfo(hwInfo_t *info)
{
    (void) info;
}

/**
 * TODO: functions temporarily implemented in "nvmem_settings_MDx.c"

int nvm_readVFOChannelData(channel_t *channel)
int nvm_readSettings(settings_t *settings)
int nvm_writeSettings(const settings_t *settings)

*/

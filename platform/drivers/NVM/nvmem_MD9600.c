/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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

W25Qx_DEVICE_DEFINE(W25Q128_main, W25Qx_api)
W25Qx_DEVICE_DEFINE(W25Q128_secr, W25Qx_secReg_api)

static const struct nvmArea areas[] =
{
    {
        .name       = "External flash",
        .dev        = &W25Q128_main,
        .startAddr  = 0x0000,
        .size       = 0x1000000,  // 16 MB, 128 Mbit
        .partitions = NULL
    },
    {
        .name       = "Cal. data 1",
        .dev        = &W25Q128_secr,
        .startAddr  = 0x1000,
        .size       = 0x100,      // 256 byte
        .partitions = NULL
    },
    {
        .name       = "Cal. data 2",
        .dev        = &W25Q128_secr,
        .startAddr  = 0x2000,
        .size       = 0x100,      // 256 byte
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

size_t nvm_getMemoryAreas(const struct nvmArea **list)
{
    *list = &areas[0];

    return (sizeof(areas) / sizeof(struct nvmArea));
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

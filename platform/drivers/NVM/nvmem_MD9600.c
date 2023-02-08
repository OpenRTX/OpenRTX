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

void nvm_init()
{
    W25Qx_init();
}

void nvm_terminate()
{
    W25Qx_terminate();
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

/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Amedeo Izzo IU2NUO,             *
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
    return;
}

/*
TODO: temporarily implemented in "nvmem_settings_MDx.c"

int nvm_readVFOChannelData(channel_t *channel)
{
    return _cps_readChannelAtAddress(channel, vfoChannelBaseAddr);
}
*/

/*

TODO: temporarily implemented in "nvmem_settings_MDx.c"

int nvm_readSettings(settings_t *settings)
{
    settings_t newSettings;
    W25Qx_wakeup();
    delayUs(5);
    W25Qx_readData(settingsAddr, ((uint8_t *) &newSettings), sizeof(settings_t));
    W25Qx_sleep();
    if(memcmp(newSettings.valid, default_settings.valid, 6) != 0)
        return -1;
    memcpy(settings, &newSettings, sizeof(settings_t));
    return 0;
}
*/

int nvm_writeSettings(const settings_t *settings)
{
    // Disable settings write until DFU is implemented for flash backups
    return -1;
}

/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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

#include <interfaces/nvmem.h>

void nvm_init()
{

}

void nvm_terminate()
{

}

void nvm_readCalibData(void *buf)
{
    (void) buf;
}

void nvm_loadHwInfo(hwInfo_t *info)
{
    (void) info;
}

int nvm_readVFOChannelData(channel_t *channel)
{
    (void) channel;
    return -1;
}

int nvm_readChannelData(channel_t *channel, uint16_t pos)
{
    (void) channel;
    (void) pos;
    return -1;
}

int nvm_readZoneData(zone_t *zone, uint16_t pos)
{
    (void) zone;
    (void) pos;
    return -1;
}

int nvm_readContactData(contact_t *contact, uint16_t pos)
{
    (void) contact;
    (void) pos;
    return -1;
}

int nvm_readSettings(settings_t *settings)
{
    (void) settings;
    return -1;
}

int nvm_writeSettings(settings_t *settings)
{
    (void) settings;
    return -1;
}


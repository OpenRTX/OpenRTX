/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
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

#include "interfaces/nvmem.h"

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
    // Module 17 has no channels: just load default values for it
    channel->mode         = OPMODE_M17;
    channel->bandwidth    = BW_12_5;
    channel->power        = 1.0;
    channel->rx_frequency = 430000000;
    channel->tx_frequency = 430000000;
    channel->fm.rxToneEn  = 0; //disabled
    channel->fm.rxTone    = 0; //and no ctcss/dcs selected
    channel->fm.txToneEn  = 0;
    channel->fm.txTone    = 0;

    return 0;
}

int nvm_readSettings(settings_t *settings)
{
    (void) settings;
    return -1;
}

int nvm_writeSettings(const settings_t *settings)
{
    (void) settings;
    return -1;
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    (void) settings;
    (void) vfo;
    return -1;
}

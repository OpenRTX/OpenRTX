/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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

#include <interfaces/radio.h>

void radio_init(const rtxStatus_t *rtxState)
{
    // TODO
    ;
}

void radio_terminate()
{
    // TODO
    ;
}

void radio_tuneVcxo(const int16_t vhfOffset, const int16_t uhfOffset)
{
    // TODO
    ;
}

void radio_setOpmode(const enum opmode mode)
{
    // TODO
    ;
}

bool radio_checkRxDigitalSquelch()
{
    // TODO
    return false;
}

void radio_enableRx()
{
    // TODO
    ;
}

void radio_enableTx()
{
    // TODO
    ;
}

void radio_disableRtx()
{
    // TODO
    ;
}

void radio_updateConfiguration()
{
    // TODO
    ;
}

float radio_getRssi()
{
    // TODO
    return 0.0f;
}

enum opstatus radio_getStatus()
{
    // TODO
    return OFF;
}

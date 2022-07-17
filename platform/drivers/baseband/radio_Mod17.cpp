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

#include <hwconfig.h>

#include "interfaces/gpio.h"
#include "interfaces/platform.h"
#include "interfaces/radio.h"

enum opstatus radioStatus;               // Current operating status

void radio_init(const rtxStatus_t *rtxState)
{
    (void) rtxState;
    radioStatus = OFF;
}

void radio_terminate()
{
    radioStatus = OFF;
}

void radio_tuneVcxo(const int16_t vhfOffset, const int16_t uhfOffset)
{
    (void) vhfOffset;
    (void) uhfOffset;
}

void radio_setOpmode(const enum opmode mode)
{
    (void) mode;
}

bool radio_checkRxDigitalSquelch()
{
    return false;
}

void radio_enableRx()
{
    radioStatus = RX;
    gpio_clearPin(PTT_OUT);
}

void radio_enableTx()
{
    radioStatus = TX;
    gpio_setPin(PTT_OUT);
}

void radio_disableRtx()
{

}

void radio_updateConfiguration()
{

}

float radio_getRssi()
{
    return -123.0f;
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}

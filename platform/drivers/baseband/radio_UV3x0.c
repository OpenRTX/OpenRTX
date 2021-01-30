/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <interfaces/radio.h>

void radio_init()
{

}

void radio_terminate()
{

}

void radio_setBandwidth(const enum bandwidth bw)
{
    (void) bw;
}

void radio_setOpmode(const enum opmode mode)
{
    (void) mode;
}

void radio_setVcoFrequency(const freq_t frequency, const bool isTransmitting)
{
    (void) frequency;
    (void) isTransmitting;
}

void radio_setCSS(const tone_t rxCss, const tone_t txCss)
{
    (void) rxCss;
    (void) txCss;
}

bool radio_checkRxDigitalSquelch()
{

}

void radio_enableRx()
{

}

void radio_enableTx(const float txPower, const bool enableCss)
{
    (void) txPower;
    (void) enableCss;
}

void radio_disableRtx()
{

}

void radio_updateCalibrationParams(const rtxStatus_t* rtxCfg)
{
    (void) rtxCfg;
}

float radio_getRssi(const freq_t rxFreq)
{
    (void) rxFreq;
    return -100.0f;
}

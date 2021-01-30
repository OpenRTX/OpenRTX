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
#include <stdio.h>

void radio_init()
{
    puts("radio_linux: init() called");
}

void radio_terminate()
{
    puts("radio_linux: terminate() called");
}

void radio_setBandwidth(const enum bandwidth bw)
{

    char *band = (bw == BW_12_5) ? "12.5" :
                 ((bw == BW_20)  ? "20"   : "25");

    printf("radio_linux: setting bandwidth to %skHz\n", band);
}

void radio_setOpmode(const enum opmode mode)
{
    char *mod = (mode == FM) ? "FM" : "DMR";
    printf("radio_linux: setting opmode to %s", mod);
}

void radio_setVcoFrequency(const freq_t frequency, const bool isTransmitting)
{
    char *txrx = isTransmitting ? "RX" : "RX";
    printf("radio_linux: setting %s VCO frequency to %d\n", txrx, frequency);
}

void radio_setCSS(const tone_t rxCss, const tone_t txCss)
{
    printf("radio_linux: setting CTCSS: RX to %.1f and TX to %.1f\n",
           rxCss/10.0f, txCss/10.0f);
}

bool radio_checkRxDigitalSquelch()
{
    puts("radio_linux: radio_checkRxDigitalSquelch(), returning 'true'");
    return true;
}

void radio_enableRx()
{
    puts("radio_linux: enableRx() called");
}

void radio_enableTx(const float txPower, const bool enableCss)
{
    printf("radio_linux: enabling TX with output power of %.2fW and CTCSS %s\n",
           txPower, enableCss ? "enabled" : "disabled");
}

void radio_disableRtx()
{
    puts("radio_linux: disableRtx() called");
}

void radio_updateCalibrationParams(const rtxStatus_t* rtxCfg)
{
    (void) rtxCfg;
    puts("radio_linux: updateCalibrationParams() called");
}

float radio_getRssi(const freq_t rxFreq)
{
    printf("radio_linux: requested RSSI at freq %d, returning -100dBm\n", rxFreq);
    return -100.0f;
}

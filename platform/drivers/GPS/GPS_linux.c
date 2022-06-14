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

#include <interfaces/gps.h>
#include <interfaces/delays.h>
#include <hwconfig.h>
#include <string.h>

#define MAX_NMEA_LEN 80
#define NMEA_SAMPLES 8

char test_nmea_sentences [NMEA_SAMPLES][MAX_NMEA_LEN] =
{
    "$GPGGA,223659.522,5333.735,N,00959.130,E,1,12,1.0,0.0,M,0.0,M,,*62",
    "$GPGSA,A,3,01,02,03,04,05,06,07,08,09,10,11,12,1.0,1.0,1.0*30",
    "$GPGSV,3,1,12,30,79,066,27,05,63,275,21,07,42,056,,13,40,289,13*76",
    "$GPGSV,3,2,12,14,36,147,20,28,30,151,,09,13,100,,02,08,226,30*72",
    "$GPGSV,3,3,12,18,05,333,,15,04,289,22,08,03,066,,27,02,030,*79",
    "$GPRMC,223659.522,A,5333.735,N,00959.130,E,,,160221,000.0,W*70",
    "$GPVTG,92.15,T,,M,0.15,N,0.28,K,A*0C"
};

void gps_init(const uint16_t baud)
{
    (void) baud;
    return;
}

void gps_terminate()
{
    return;
}

void gps_enable()
{
    return;
}

void gps_disable()
{
    return;
}

bool gps_detect(uint16_t timeout)
{
    (void) timeout;
    return true;
}

int gps_getNmeaSentence(char *buf, const size_t maxLength)
{
    static int i = 0;

    // Emulate GPS device by sending NMEA sentences every 1s
    if(i == 0)
        sleepFor(1u, 0u);
    size_t len = strnlen(test_nmea_sentences[i], MAX_NMEA_LEN);
    if (len > maxLength)
        return -1;
    strncpy(buf, test_nmea_sentences[i], maxLength);
    i++;
    i %= NMEA_SAMPLES;
    return len;
}

bool gps_nmeaSentenceReady()
{
    return true;
}

void gps_waitForNmeaSentence()
{
    return;
}


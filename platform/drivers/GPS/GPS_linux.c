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

#include <interfaces/gps.h>
#include <hwconfig.h>
#include <string.h>
#include <os.h>

void gps_init(const uint16_t baud)
{
    ;
}

void gps_terminate()
{
    ;
}

void gps_enable()
{
    ;
}

void gps_disable()
{
    ;
}

bool gps_detect(uint16_t timeout)
{
    return true;
}

int gps_getNmeaSentence(char *buf, const size_t maxLength)
{
    OS_ERR os_err;

    // Emulate GPS device by sending NMEA sentence every 1s
    OSTimeDlyHMSM(0u, 0u, 1u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    char *nmea_sample = "$GPRMC,083703.000,A,4229.7596,N,00912.5377,E,0.15,92.15,040221,,,A*51";
    strncpy(buf, nmea_sample, maxLength);
}


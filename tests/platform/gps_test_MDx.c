/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
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

#include <hwconfig.h>
#include <interfaces/delays.h>
#include <interfaces/gpio.h>
#include <interfaces/gps.h>
#include <interfaces/platform.h>
#include <minmea.h>
#include <stdint.h>
#include <stdio.h>

char line[MINMEA_MAX_LENGTH * 10];

int main()
{
    platform_init();

    printf("Checking for GPS... ");
    bool hasGps = gps_detect(5000);
    printf(" %s.\r\n", hasGps ? "OK" : "TIMEOUT");

    gps_init(9600);
    gps_enable();

    while (1)
    {
        int len = gps_getNmeaSentence(line, MINMEA_MAX_LENGTH * 10);
        if (len != -1)
        {
            printf("Got sentence with length %d:\r\n", len);
            printf("%s\r\n", line);
            switch (minmea_sentence_id(line, false))
            {
                case MINMEA_SENTENCE_RMC:
                {
                    struct minmea_sentence_rmc frame;
                    if (minmea_parse_rmc(&frame, line))
                    {
                        printf(
                            "$RMC: raw coordinates and speed: (%d/%d,%d/%d) "
                            "%d/%d\n\r",
                            frame.latitude.value, frame.latitude.scale,
                            frame.longitude.value, frame.longitude.scale,
                            frame.speed.value, frame.speed.scale);
                        printf(
                            "$RMC fixed-point coordinates and speed scaled to "
                            "three decimal places: (%d,%d) %d\n\r",
                            minmea_rescale(&frame.latitude, 1000),
                            minmea_rescale(&frame.longitude, 1000),
                            minmea_rescale(&frame.speed, 1000));
                        printf(
                            "$RMC floating point degree coordinates and speed: "
                            "(%f,%f) %f\n\r",
                            minmea_tocoord(&frame.latitude),
                            minmea_tocoord(&frame.longitude),
                            minmea_tofloat(&frame.speed));
                    }
                }
                break;

                case MINMEA_SENTENCE_GGA:
                {
                    struct minmea_sentence_gga frame;
                    if (minmea_parse_gga(&frame, line))
                    {
                        printf("$GGA: fix quality: %d\n\r", frame.fix_quality);
                    }
                }
                break;

                case MINMEA_SENTENCE_GSV:
                {
                    struct minmea_sentence_gsv frame;
                    if (minmea_parse_gsv(&frame, line))
                    {
                        printf("$GSV: message %d of %d\n\r", frame.msg_nr,
                               frame.total_msgs);
                        printf("$GSV: satellites in view: %d\n\r",
                               frame.total_sats);
                        for (int i = 0; i < 4; i++)
                            printf(
                                "$GSV: sat nr %d, elevation: %d, azimuth: %d, "
                                "snr: %d dbm\n\r",
                                frame.sats[i].nr, frame.sats[i].elevation,
                                frame.sats[i].azimuth, frame.sats[i].snr);
                    }
                }
                break;

                case MINMEA_SENTENCE_VTG:
                {
                }
                break;

                // Ignore this message as we take data from RMC
                case MINMEA_SENTENCE_GLL:;
                    break;

                // These messages are never sent by the Jumpstar JS-M710 Module
                case MINMEA_SENTENCE_GSA:
                    break;
                case MINMEA_SENTENCE_GST:
                    break;
                case MINMEA_SENTENCE_ZDA:
                    break;

                // Error handling
                case MINMEA_INVALID:
                {
                    printf("Error: Invalid NMEA sentence!\n\r");
                }
                break;

                case MINMEA_UNKNOWN:
                {
                    printf("Error: Unsupported NMEA sentence!\n\r");
                }
                break;
            }
        }
    }

    return 0;
}

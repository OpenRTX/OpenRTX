/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
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
#include <minmea.h>
#include <stdio.h>
#include <state.h>

/**
 * This function parses a GPS NMEA sentence and updates radio state
 */
void gps_taskFunc(char *line, int len, gps_t *state)
{
    switch (minmea_sentence_id(line, false)) {
        case MINMEA_SENTENCE_RMC:
        {
            struct minmea_sentence_rmc frame;
            if (minmea_parse_rmc(&frame, line)) {
                state->latitude = minmea_tocoord(&frame.latitude);
                state->longitude = minmea_tocoord(&frame.longitude);
                state->speed = minmea_tofloat(&frame.speed);
                state->timestamp.hour = frame.time.hours;
                state->timestamp.minute = frame.time.minutes;
                state->timestamp.second = frame.time.seconds;
                state->timestamp.day = 0;
                state->timestamp.date = frame.date.day;
                state->timestamp.month = frame.date.month;
                state->timestamp.year = frame.date.year;
            }
        } break;

        case MINMEA_SENTENCE_GGA:
        {
            struct minmea_sentence_gga frame;
            if (minmea_parse_gga(&frame, line))
            {
                state->fix_quality = frame.fix_quality;
                state->satellites_tracked = frame.satellites_tracked;
                state->altitude = minmea_tofloat(&frame.altitude);
            }
        } break;

        case MINMEA_SENTENCE_GSA:
        {
            state->active_sats = 0;
            struct minmea_sentence_gsa frame;
            if (minmea_parse_gsa(&frame, line))
            {
                state->fix_type = frame.fix_type;
                for (int i = 0; i < 12; i++)
                {
                    if (frame.sats[i] != 0)
                    {
                        state->active_sats |= 1 << frame.sats[i];
                    }
                }
            }
        } break;

        case MINMEA_SENTENCE_GSV:
        {
            struct minmea_sentence_gsv frame;
            if (minmea_parse_gsv(&frame, line))
            {
                state->satellites_in_view = frame.total_sats; 
                for (int i = 0; i < 4; i++)
                {
                    int index = 4 * (frame.msg_nr - 1) + i;
                    state->satellites[index].id = frame.sats[i].nr;
                    state->satellites[index].elevation = frame.sats[i].elevation;
                    state->satellites[index].azimuth = frame.sats[i].azimuth;
                    state->satellites[index].snr = frame.sats[i].snr;
                }
                // Zero out unused satellite slots
                if (frame.msg_nr == frame.total_msgs && frame.total_msgs < 3)
                    bzero(&state->satellites[4 * frame.msg_nr],
                          sizeof(sat_t) * 12 - frame.total_msgs * 4);
            }
        } break;

        case MINMEA_SENTENCE_VTG:
        {
            struct minmea_sentence_vtg frame;
            if (minmea_parse_vtg(&frame, line)) {
                state->speed = minmea_tofloat(&frame.speed_kph);
                state->tmg_mag = minmea_tofloat(&frame.magnetic_track_degrees);
                state->tmg_true = minmea_tofloat(&frame.true_track_degrees);
            }

        } break;

        // Ignore this message as we take data from RMC
        case MINMEA_SENTENCE_GLL: break;

        // These messages are never sent by the Jumpstar JS-M710 Module
        case MINMEA_SENTENCE_GST: break;
        case MINMEA_SENTENCE_ZDA: break;

        // Error handling
        case MINMEA_INVALID: break;
        case MINMEA_UNKNOWN: break;
    }
}

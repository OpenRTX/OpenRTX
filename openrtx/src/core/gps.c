/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
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

#include <interfaces/platform.h>
#include <peripherals/gps.h>
#include <gps.h>
#include <minmea.h>
#include <stdio.h>
#include <state.h>
#include <string.h>
#include <stdbool.h>

#define KNOTS2KMH 1.852f

static char sentence[2*MINMEA_MAX_LENGTH];
static bool gpsEnabled        = false;
static bool readNewSentence   = true;
#ifdef CONFIG_RTC
static bool isRtcSyncronised  = false;
#endif

void gps_task()
{
    // No GPS, return
    if(state.gpsDetected == false)
        return;

    // Handle GPS turn on/off
    if(state.settings.gps_enabled != gpsEnabled)
    {
        gpsEnabled = state.settings.gps_enabled;

        if(gpsEnabled)
            gps_enable();
        else
            gps_disable();
    }

    // GPS disabled, nothing to do
    if(gpsEnabled == false)
        return;

    // Acquire a new NMEA sentence from GPS
    if(readNewSentence)
    {
        int status = gps_getNmeaSentence(sentence, 2*MINMEA_MAX_LENGTH);
        if(status != 0) return;
        readNewSentence = false;
    }

    // Waiting for a sentence...
    if(gps_nmeaSentenceReady() == false)
        return;

    // Discard all non-GPS sentences
    if((sentence[0] != '$') || (sentence[1] != 'G'))
    {
        readNewSentence = true;
        return;
    }

    // Parse the sentence. Work on a local state copy to minimize the time
    // spent with the state mutex locked
    gps_t gps_data;
    pthread_mutex_lock(&state_mutex);
    gps_data = state.gps_data;
    pthread_mutex_unlock(&state_mutex);

    int32_t sId = minmea_sentence_id(sentence, false);
    switch(sId)
    {
        case MINMEA_SENTENCE_RMC:
        {
            struct minmea_sentence_rmc frame;
            if (minmea_parse_rmc(&frame, sentence))
            {
                gps_data.latitude = minmea_tocoord(&frame.latitude);
                gps_data.longitude = minmea_tocoord(&frame.longitude);
                gps_data.timestamp.hour = frame.time.hours;
                gps_data.timestamp.minute = frame.time.minutes;
                gps_data.timestamp.second = frame.time.seconds;
                gps_data.timestamp.day = 0;
                gps_data.timestamp.date = frame.date.day;
                gps_data.timestamp.month = frame.date.month;
                gps_data.timestamp.year = frame.date.year;
            }

            gps_data.tmg_true = minmea_tofloat(&frame.course);
            gps_data.speed = minmea_tofloat(&frame.speed) * KNOTS2KMH;
        }
        break;

        case MINMEA_SENTENCE_GGA:
        {
            struct minmea_sentence_gga frame;
            if (minmea_parse_gga(&frame, sentence))
            {
                gps_data.fix_quality = frame.fix_quality;
                gps_data.satellites_tracked = frame.satellites_tracked;
                gps_data.altitude = minmea_tofloat(&frame.altitude);
            }
        }
        break;

        case MINMEA_SENTENCE_GSA:
        {
            gps_data.active_sats = 0;
            struct minmea_sentence_gsa frame;
            if (minmea_parse_gsa(&frame, sentence))
            {
                gps_data.fix_type = frame.fix_type;
                for (int i = 0; i < 12; i++)
                {
                    if (frame.sats[i] != 0)
                    {
                        gps_data.active_sats |= 1 << (frame.sats[i] - 1);
                    }
                }
            }
        }
        break;

        case MINMEA_SENTENCE_GSV:
        {
            // Parse only sentences 1 - 3, maximum 12 satellites
            struct minmea_sentence_gsv frame;
            if (minmea_parse_gsv(&frame, sentence) && (frame.msg_nr < 3))
            {
                // When the first sentence arrives, clear all the old data
                if (frame.msg_nr == 1)
                {
                    memset(&gps_data.satellites[0], 0x00, 12 * sizeof(gpssat_t));
                }

                gps_data.satellites_in_view = frame.total_sats;
                for (int i = 0; i < 4; i++)
                {
                    int index = 4 * (frame.msg_nr - 1) + i;
                    gps_data.satellites[index].id = frame.sats[i].nr;
                    gps_data.satellites[index].elevation = frame.sats[i].elevation;
                    gps_data.satellites[index].azimuth = frame.sats[i].azimuth;
                    gps_data.satellites[index].snr = frame.sats[i].snr;
                }
            }
        }
        break;

        case MINMEA_SENTENCE_VTG:
        {
            struct minmea_sentence_vtg frame;
            if (minmea_parse_vtg(&frame, sentence))
            {
                gps_data.speed = minmea_tofloat(&frame.speed_kph);
                gps_data.tmg_mag = minmea_tofloat(&frame.magnetic_track_degrees);
                gps_data.tmg_true = minmea_tofloat(&frame.true_track_degrees);
            }
        }
        break;

        // Ignore this message as we take data from RMC
        case MINMEA_SENTENCE_GLL: break;

        // These messages are never sent by the Jumpstar JS-M710 Module
        case MINMEA_SENTENCE_GST: break;
        case MINMEA_SENTENCE_ZDA: break;

        // Error handling
        case MINMEA_INVALID: break;
        case MINMEA_UNKNOWN: break;
    }

    // Update GPS data inside radio state
    pthread_mutex_lock(&state_mutex);
    state.gps_data = gps_data;
    pthread_mutex_unlock(&state_mutex);

    // Synchronize RTC with GPS UTC clock, only when fix is done
    #ifdef CONFIG_RTC
    if(state.gps_set_time)
    {
        if((sId == MINMEA_SENTENCE_RMC) &&
           (gps_data.fix_quality > 0)   &&
           (isRtcSyncronised == false))
        {
            platform_setTime(gps_data.timestamp);
            isRtcSyncronised = true;
        }
    }
    else
    {
        isRtcSyncronised = false;
    }
    #endif

    // Finally, trigger the acquisition of a new NMEA sentence
    readNewSentence = true;
}

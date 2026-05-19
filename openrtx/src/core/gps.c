/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "core/gps.h"
#include <minmea.h>
#include <stdio.h>
#include "core/state.h"
#include <string.h>
#include <stdbool.h>

#define KNOTS2KMH(x) ((((int) x) * 1852) / 1000)

static bool gpsEnabled = false;

#ifdef CONFIG_RTC
static bool rtcSyncDone = false;
static void syncRtc(datetime_t timestamp)
{
    if(state.settings.gpsSetTime == false) {
        rtcSyncDone = false;
        return;
    }

    if(rtcSyncDone)
        return;

    platform_setTime(timestamp);
    rtcSyncDone = true;
}
#endif

void gps_task(const struct gpsDevice *dev)
{
    char sentence[2*MINMEA_MAX_LENGTH];
    int ret;

    // No GPS, return
    if(dev == NULL)
        return;

    // Handle GPS turn on/off
    if(state.settings.gps_enabled != gpsEnabled)
    {
        gpsEnabled = state.settings.gps_enabled;

        if(gpsEnabled)
            gps_enable(dev);
        else
            gps_disable(dev);
    }

    // GPS disabled, nothing to do
    if(gpsEnabled == false)
        return;

    // Acquire a new NMEA sentence from GPS
    ret = gps_getSentence(dev, sentence, sizeof(sentence));
    if(ret <= 0)
        return;

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
                gps_data.timestamp.hour = frame.time.hours;
                gps_data.timestamp.minute = frame.time.minutes;
                gps_data.timestamp.second = frame.time.seconds;
                gps_data.timestamp.day = 0;
                gps_data.timestamp.date = frame.date.day;
                gps_data.timestamp.month = frame.date.month;
                gps_data.timestamp.year = frame.date.year;
                gps_data.speed = KNOTS2KMH(minmea_toint(&frame.speed));
                #ifdef CONFIG_RTC
                if(frame.valid)
                    syncRtc(gps_data.timestamp);
                #endif
            }
        }
        break;

        case MINMEA_SENTENCE_GGA:
        {
            struct minmea_sentence_gga frame;
            if (minmea_parse_gga(&frame, sentence))
            {
                gps_data.latitude = minmea_tofixedpoint(&frame.latitude);
                gps_data.longitude = minmea_tofixedpoint(&frame.longitude);
                gps_data.fix_quality = frame.fix_quality;
                gps_data.satellites_tracked = frame.satellites_tracked;
                gps_data.altitude = minmea_toint(&frame.altitude);
            }
        }
        break;

        case MINMEA_SENTENCE_GSA:
        {
            gps_data.active_sats = 0;
            struct minmea_sentence_gsa frame;
            if (minmea_parse_gsa(&frame, sentence))
            {
                gps_data.hdop = minmea_toscaledint(&frame.hdop, 100);
                gps_data.fix_type = frame.fix_type;
                for (int i = 0; i < 12; i++)
                {
                    if (frame.sats[i] != 0 && frame.sats[i] < 31)
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
                gps_data.speed = minmea_toint(&frame.speed_kph);
                gps_data.tmg_mag = minmea_toint(&frame.magnetic_track_degrees);
                gps_data.tmg_true = minmea_toint(&frame.true_track_degrees);
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
}

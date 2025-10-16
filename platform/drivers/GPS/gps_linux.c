/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <sys/time.h>
#include <stdbool.h>
#include <string.h>
#include "core/gps.h"

#define MAX_NMEA_LEN 80
#define NMEA_SAMPLES 8

static const char test_nmea_sentences[NMEA_SAMPLES][MAX_NMEA_LEN] =
{
    "$GPGGA,223659.522,5333.735,N,00959.130,E,1,12,1.0,0.0,M,0.0,M,,*62",
    "$GPGSA,A,3,01,02,03,04,05,06,07,08,09,10,11,12,1.0,1.0,1.0*30",
    "$GPGSV,3,1,12,30,79,066,27,05,63,275,21,07,42,056,,13,40,289,13*76",
    "$GPGSV,3,2,12,14,36,147,20,28,30,151,,09,13,100,,02,08,226,30*72",
    "$GPGSV,3,3,12,18,05,333,,15,04,289,22,08,03,066,,27,02,030,*79",
    "$GPRMC,223659.522,A,5333.735,N,00959.130,E,,,160221,000.0,W*70",
    "$GPVTG,92.15,T,,M,0.15,N,0.28,K,A*0C"
};

static long long startTime;
static bool enabled = true;
static int currSentence = 0;

static inline long long now()
{
    struct timeval te;
    gettimeofday(&te, NULL);

    return (te.tv_sec*1000LL) + (te.tv_usec/1000);
}

static void enable(void *priv)
{
    (void) priv;

    enabled = true;
    currSentence = 0;
    startTime = now();
}

static void disable(void *priv)
{
    (void) priv;

    enabled = false;
}

static int getNmeaSentence(void *priv, char *buf, const size_t bufSize)
{
    (void) priv;

    // GPS off > no data
    if(!enabled)
        return 0;

    // Emit one sentence every 1s
    long long currTime = now();
    if((currTime - startTime) < 1000)
        return 0;

    size_t len = strnlen(test_nmea_sentences[currSentence], MAX_NMEA_LEN);
    if(len > bufSize)
        return -1;

    strncpy(buf, test_nmea_sentences[currSentence], bufSize);
    currSentence += 1;
    currSentence %= NMEA_SAMPLES;
    startTime = currTime;

    return (int) len;
}

const struct gpsDevice gps = {
    .enable = enable,
    .disable = disable,
    .getSentence = getNmeaSentence
};

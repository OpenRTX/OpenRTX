/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "hwconfig.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "nmea_rbuf.h"

static inline size_t nmeaRbuf_size(struct nmeaRbuf *rbuf)
{
    if(rbuf->wrPos >= rbuf->rdPos)
        return rbuf->wrPos - rbuf->rdPos;
    else
        return CONFIG_NMEA_RBUF_SIZE + rbuf->wrPos - rbuf->rdPos;
}

void nmeaRbuf_reset(struct nmeaRbuf *rbuf)
{
    rbuf->wrPos = 0;
    rbuf->rdPos = 0;
    rbuf->rdLimit = 0;
    rbuf->filling = false;
}

int nmeaRbuf_putChar(struct nmeaRbuf *rbuf, const char c)
{
    if((rbuf->filling == false) && (c == '$'))
        rbuf->filling = true;

    if(rbuf->filling) {
        size_t next = (rbuf->wrPos + 1) % CONFIG_NMEA_RBUF_SIZE;

        // No more space, drop current sentence and restart
        if(next == rbuf->rdPos) {
            rbuf->filling = false;
            rbuf->wrPos = rbuf->rdLimit;

            return -1;
        }

        // Append the new character
        rbuf->data[rbuf->wrPos] = c;
        rbuf->wrPos = next;

        // Check if a full sentence is present
        if(c == '\n') {
            rbuf->filling = false;
            rbuf->rdLimit = rbuf->wrPos;
        }
    }

    return 0;
}

int nmeaRbuf_putSentence(struct nmeaRbuf *rbuf, const char *sentence)
{
    size_t len = strlen(sentence);
    size_t next = (rbuf->wrPos + len) % CONFIG_NMEA_RBUF_SIZE;
    size_t free = CONFIG_NMEA_RBUF_SIZE - nmeaRbuf_size(rbuf);

    // Bad-formatted string
    if((sentence[0] != '$') || (sentence[len - 1] != '\n'))
        return -1;

    // Not enough space
    if(len >= free)
        return -2;

    // Handle write pointer wrap-around
    if((rbuf->wrPos + len) >= CONFIG_NMEA_RBUF_SIZE) {
        size_t chunkSize = CONFIG_NMEA_RBUF_SIZE - rbuf->wrPos;
        memcpy(&rbuf->data[rbuf->wrPos], sentence, chunkSize);
        sentence += chunkSize;
        len -= chunkSize;
        rbuf->wrPos = 0;
    }

   memcpy(&rbuf->data[rbuf->wrPos], sentence, len);
   rbuf->wrPos = next;
   rbuf->rdLimit = next;

   return 0;
}

int nmeaRbuf_getSentence(struct nmeaRbuf *rbuf, char *buf, const size_t maxLen)
{
    size_t bufPos = 0;
    char c;

    if(rbuf->rdPos == rbuf->rdLimit)
        return 0;

    do {
        // Pop one character from the buffer
        c = rbuf->data[rbuf->rdPos];
        rbuf->rdPos += 1;
        rbuf->rdPos %= CONFIG_NMEA_RBUF_SIZE;

        // Store it
        buf[bufPos] = c;
        if(bufPos < maxLen)
            bufPos += 1;

    } while(c != '\n');

    if(bufPos == maxLen)
        return -1;

    return (int) bufPos;
}


/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NMEA_RBUF_H
#define NMEA_RBUF_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Ad-hoc ring buffer for storing NMEA sentences.
 *
 * The implementation of the ring buffer is lock-free, allowing to call the
 * put/get functions also from whithin an IRQ. This implementation limits the
 * buffer to have ONE producer and ONE consumer.
 * The sentences can be put into the buffer either one character at a time or
 * one sentence at a time; in either case the put functions ensure that only
 * valid sentences are inserted in the buffer. Extraction of sentences from the
 * buffer can be done only by a per-sentence basis.
 */

struct nmeaRbuf {
    size_t wrPos;
    size_t rdPos;
    size_t rdLimit;
    bool filling;
    char data[CONFIG_NMEA_RBUF_SIZE];
};

/**
 * Reset the internal state of a ring buffer.
 *
 * @param rbuf: pointer to ring buffer.
 */
void nmeaRbuf_reset(struct nmeaRbuf *rbuf);

/**
 * Insert an NMEA sentence character.
 * This function implments a finite state machine guaranteeing that the stored
 * characters are always part of a valid NMEA sentence.
 *
 * @param rbuf: pointer to ring buffer.
 * @param c: incoming character.
 * @return zero on success, -1 if the ring buffer is full.
 */
int nmeaRbuf_putChar(struct nmeaRbuf *rbuf, const char c);

/**
 * Insert a full NMEA sentence.
 * The sentence has to begin with an '$' character and terminate with a '\n'.
 *
 * @param rbuf: pointer to ring buffer.
 * @param sentence: NMEA sentence.
 * @return zero on success, -1 if the sentence is not valid and -2 if the ring
 * buffer is full
 */
int nmeaRbuf_putSentence(struct nmeaRbuf *rbuf, const char *sentence);

/**
 * Extract a full NMEA sentence.
 * If the sentence is longer than the maximum size of the destination buffer,
 * the characters not written in the destination are removed from the ring
 * buffer anyways.
 *
 * @param rbuf: pointer to ring buffer.
 * @param buf: pointer to NMEA sentence destination buffer.
 * @param maxLen: maximum acceptable size for the destination buffer.
 * @return the length of the extracted sentence or -1 if the sentence is longer
 * than the maximum allowed size. If the ring buffer is empty, zero is returned.
 */
int nmeaRbuf_getSentence(struct nmeaRbuf *rbuf, char *buf, const size_t maxLen);

#ifdef __cplusplus
}
#endif

#endif  // NMEA_RBUF_H

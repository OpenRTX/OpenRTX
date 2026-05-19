/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GPS_ZEPHYR_H
#define GPS_ZEPHYR_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise the GPS driver.
 * This function does not turn on the GPS module.
 */
void gpsZephyr_init();

/**
 * Terminate the GPS driver.
 */
void gpsZephyr_terminate();

/**
 * Retrieve a NMEA sentence from the GPS.
 * If the sentence is longer than the maximum size of the destination buffer,
 * the characters not written in the destination are lost.
 *
 * @param priv: unused parameter, for function signature compatibility.
 * @param buf: pointer to NMEA sentence destination buffer.
 * @param maxLen: maximum acceptable size for the destination buffer.
 * @return the length of the extracted sentence or -1 if the sentence is longer
 * than the maximum allowed size. If the ring buffer is empty, zero is returned.
 */
int gpsZephyr_getNmeaSentence(void *priv, char *buf, const size_t maxLength);

#ifdef __cplusplus
}
#endif

#endif /* GPS_STM32_H */

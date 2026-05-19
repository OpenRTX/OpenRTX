/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GPS_STM32_H
#define GPS_STM32_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise the GPS driver.
 * This function does not turn on the GPS module.
 *
 * @param baud: baud rate of GPS serial interface.
 */
void gpsStm32_init(const uint16_t baud);

/**
 * Shut down the GPS module and terminate the GPS driver.
 */
void gpsStm32_terminate();

/**
 * Turn on the GPS module.
 *
 * @param priv: unused parameter, for function signature compatibility.
 */
void gpsStm32_enable(void *priv);

/**
 * Turn off GPS module.
 *
 * @param priv: unused parameter, for function signature compatibility.
 */
void gpsStm32_disable(void *priv);

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
int gpsStm32_getNmeaSentence(void *priv, char *buf, const size_t maxLength);

#ifdef __cplusplus
}
#endif

#endif /* GPS_STM32_H */

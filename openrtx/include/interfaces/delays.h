/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DELAYS_H
#define DELAYS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function prototypes for microsecond and millisecond delays.
 * Their implementation is device-specific, thus it is placed inside the drivers
 * folder.
 */

/**
 * Exact microsecond delay.
 * @param useconds: delay value
 */
void delayUs(unsigned int useconds);

/**
 * Exact millisecond delay.
 * @param mseconds: delay value
 */
void delayMs(unsigned int mseconds);

/**
 * Puts the calling thread in a sleeping state for the specified amount of time.
 * @param seconds: sleep time, seconds.
 * @param mseconds: sleep time, milliseconds.
 */
void sleepFor(unsigned int seconds, unsigned int mseconds);

/**
 * Puts the calling thread in a sleeping state until the system time reaches
 * the target value passed as parameter.
 * @param timestamp: traget timestamp for the wakeup.
 */
void sleepUntil(long long timestamp);

/**
 * Get the current value of the system tick.
 * @return current system tick value.
 */
long long getTick();

#ifdef __cplusplus
}
#endif

#endif /* DELAYS_H */

/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2020-2025 OpenRTX Contributors
 *
 * This file is part of OpenRTX.
 */

#ifndef RNG_H
#define RNG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Driver for Random Number Generator.
 */

/**
 * Initialise the RNG driver.
 */
void rng_init();

/**
 * Shut down the RNG module.
 */
void rng_terminate();

/**
 * Generate a 32 bit random number.
 *
 * @return random number.
 */
uint32_t rng_get();

#ifdef __cplusplus
}
#endif

#endif /* INTERFACES_GPS_H */

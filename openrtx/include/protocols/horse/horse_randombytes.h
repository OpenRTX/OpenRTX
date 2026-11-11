/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HORSE_RANDOMBYTES_H
#define HORSE_RANDOMBYTES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Install the platform randombytes implementation before
 *        sodium_init(). On MD-3x0 this is the STM32F405 HASH_RNG
 *        peripheral. On the host this is a no-op (libsodium sysrandom).
 *
 * @return 0 on success, -1 on failure.
 */
int horse_randombytes_install(void);

/**
 * \brief True after a HASH_RNG seed, clock, timeout, or repeat failure.
 */
int horse_randombytes_failed(void);

#ifdef HORSE_RANDOMBYTES_TEST
void horse_randombytes_test_reset(void);
void horse_randombytes_test_feed(const uint32_t *words, size_t n);
void horse_randombytes_test_set_sr(int secs, int cecs);
int horse_randombytes_test_read(uint32_t *out);
#endif

#ifdef __cplusplus
}
#endif

#endif

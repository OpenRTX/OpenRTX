/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HORSE_RANDOMBYTES_H
#define HORSE_RANDOMBYTES_H

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

#ifdef __cplusplus
}
#endif

#endif

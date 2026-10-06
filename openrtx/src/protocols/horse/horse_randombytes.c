/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * libsodium randombytes backed by the STM32F405 HASH_RNG peripheral
 * (true TRNG, PLL48 clock). Not a software CSPRNG.
 */

#include "protocols/horse/horse_randombytes.h"

#if defined(HAVE_LIBSODIUM) && defined(PLATFORM_MD3x0)

#include "peripherals/rng.h"
#include <sodium.h>
#include <string.h>

static const char *horse_rng_name(void)
{
    return "stm32f405-hash-rng";
}

static uint32_t horse_rng_random(void)
{
    return rng_get();
}

static void horse_rng_buf(void *const buf, const size_t size)
{
    uint8_t *p = (uint8_t *)buf;
    size_t n = size;

    while (n > 0) {
        uint32_t r = rng_get();
        size_t k = n < sizeof r ? n : sizeof r;
        memcpy(p, &r, k);
        p += k;
        n -= k;
    }
}

static const randombytes_implementation horse_rng_impl = {
    SODIUM_C99(.implementation_name =) horse_rng_name,
    SODIUM_C99(.random =) horse_rng_random,
    SODIUM_C99(.stir =) NULL,
    SODIUM_C99(.uniform =) NULL,
    SODIUM_C99(.buf =) horse_rng_buf,
    SODIUM_C99(.close =) NULL
};

int horse_randombytes_install(void)
{
    rng_init();
    if (randombytes_set_implementation(&horse_rng_impl) != 0)
        return -1;
    return 0;
}

#else

int horse_randombytes_install(void)
{
    return 0;
}

#endif

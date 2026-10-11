/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * libsodium randombytes: STM32F405 HASH_RNG on MD-3x0, fail closed.
 * Does not call platform rng_get() (that retries errors forever).
 */

#include "protocols/horse/horse_randombytes.h"

#include <string.h>

#if defined(HORSE_RANDOMBYTES_TEST) \
    || (defined(HAVE_LIBSODIUM) && defined(PLATFORM_MD3x0))
#include <pthread.h>
#endif

#if defined(HAVE_LIBSODIUM) && defined(PLATFORM_MD3x0) \
    && !defined(HORSE_RANDOMBYTES_TEST)
#include "interfaces/delays.h"
#include "peripherals/rng.h"
#include "stm32f4xx.h"
#endif

#if defined(HAVE_LIBSODIUM)
#include <sodium.h>
#endif

#if defined(HORSE_RANDOMBYTES_TEST) \
    || (defined(HAVE_LIBSODIUM) && defined(PLATFORM_MD3x0))
static pthread_mutex_t rng_mu = PTHREAD_MUTEX_INITIALIZER;
static int rng_failed;
static int rng_have_last;
static uint32_t rng_last;
static int rng_discarded;
#endif

#if defined(HORSE_RANDOMBYTES_TEST)

static uint32_t mock_words[64];
static size_t mock_n;
static size_t mock_i;
static int mock_secs;
static int mock_cecs;

void horse_randombytes_test_reset(void)
{
    pthread_mutex_lock(&rng_mu);
    rng_failed = 0;
    rng_have_last = 0;
    rng_last = 0;
    rng_discarded = 0;
    mock_n = 0;
    mock_i = 0;
    mock_secs = 0;
    mock_cecs = 0;
    pthread_mutex_unlock(&rng_mu);
}

void horse_randombytes_test_feed(const uint32_t *words, size_t n)
{
    if (words == NULL || n > (sizeof mock_words / sizeof mock_words[0]))
        return;
    pthread_mutex_lock(&rng_mu);
    memcpy(mock_words, words, n * sizeof(uint32_t));
    mock_n = n;
    mock_i = 0;
    pthread_mutex_unlock(&rng_mu);
}

void horse_randombytes_test_set_sr(int secs, int cecs)
{
    pthread_mutex_lock(&rng_mu);
    mock_secs = secs;
    mock_cecs = cecs;
    pthread_mutex_unlock(&rng_mu);
}

static int horse_rng_hw_read(uint32_t *out)
{
    if (mock_secs || mock_cecs)
        return -1;
    if (mock_i >= mock_n)
        return -1;
    *out = mock_words[mock_i++];
    return 0;
}

static int horse_rng_hw_init(void)
{
    return 0;
}

#elif defined(HAVE_LIBSODIUM) && defined(PLATFORM_MD3x0)

enum { HORSE_RNG_SPIN_MAX = 10000 };

static int horse_rng_hw_read(uint32_t *out)
{
    unsigned i;

    for (i = 0; i < HORSE_RNG_SPIN_MAX; i++) {
        uint32_t sr = RNG->SR;

        if ((sr & (RNG_SR_SECS | RNG_SR_CECS | RNG_SR_SEIS | RNG_SR_CEIS)) != 0)
            return -1;
        if ((sr & RNG_SR_DRDY) != 0) {
            *out = RNG->DR;
            return 0;
        }
        delayUs(1);
    }
    return -1;
}

static int horse_rng_hw_init(void)
{
    rng_init();
    RNG->CR = RNG_CR_RNGEN;
    return 0;
}

#endif

#if defined(HORSE_RANDOMBYTES_TEST) \
    || (defined(HAVE_LIBSODIUM) && defined(PLATFORM_MD3x0))

static int horse_rng_next_word(uint32_t *out)
{
    uint32_t w;

    if (rng_failed)
        return -1;
    if (horse_rng_hw_read(&w) != 0) {
        rng_failed = 1;
        return -1;
    }
    if (rng_have_last && w == rng_last) {
        rng_failed = 1;
        return -1;
    }
    rng_last = w;
    rng_have_last = 1;
    *out = w;
    return 0;
}

static const char *horse_rng_name(void)
{
    return "stm32f405-hash-rng";
}

static uint32_t horse_rng_random(void)
{
    uint32_t w = 0;

    pthread_mutex_lock(&rng_mu);
    if (horse_rng_next_word(&w) != 0)
        w = 0;
    pthread_mutex_unlock(&rng_mu);
    return w;
}

static void horse_rng_buf(void *const buf, const size_t size)
{
    uint8_t *p = (uint8_t *)buf;
    size_t n = size;

    pthread_mutex_lock(&rng_mu);
    while (n > 0) {
        uint32_t r = 0;
        size_t k;

        if (horse_rng_next_word(&r) != 0) {
            memset(p, 0, n);
            break;
        }
        k = n < sizeof r ? n : sizeof r;
        memcpy(p, &r, k);
        p += k;
        n -= k;
    }
    pthread_mutex_unlock(&rng_mu);
}

#if defined(HAVE_LIBSODIUM)
static const randombytes_implementation horse_rng_impl = {
    SODIUM_C99(.implementation_name =) horse_rng_name,
    SODIUM_C99(.random =) horse_rng_random,
    SODIUM_C99(.stir =) NULL,
    SODIUM_C99(.uniform =) NULL,
    SODIUM_C99(.buf =) horse_rng_buf,
    SODIUM_C99(.close =) NULL
};
#endif

int horse_randombytes_install(void)
{
    uint32_t discard;

    pthread_mutex_lock(&rng_mu);
    if (rng_failed) {
        pthread_mutex_unlock(&rng_mu);
        return -1;
    }
    if (rng_discarded) {
        pthread_mutex_unlock(&rng_mu);
        return 0;
    }
    if (horse_rng_hw_init() != 0) {
        rng_failed = 1;
        pthread_mutex_unlock(&rng_mu);
        return -1;
    }
    if (horse_rng_next_word(&discard) != 0) {
        pthread_mutex_unlock(&rng_mu);
        return -1;
    }
    rng_discarded = 1;
    (void)discard;
    pthread_mutex_unlock(&rng_mu);

#if defined(HAVE_LIBSODIUM)
    if (randombytes_set_implementation(&horse_rng_impl) != 0) {
        rng_failed = 1;
        return -1;
    }
#endif
    return 0;
}

int horse_randombytes_failed(void)
{
    int f;

    pthread_mutex_lock(&rng_mu);
    f = rng_failed;
    pthread_mutex_unlock(&rng_mu);
    return f;
}

#ifdef HORSE_RANDOMBYTES_TEST
int horse_randombytes_test_read(uint32_t *out)
{
    int rc;

    if (out == NULL)
        return -1;
    pthread_mutex_lock(&rng_mu);
    rc = horse_rng_next_word(out);
    pthread_mutex_unlock(&rng_mu);
    return rc;
}
#endif

#else

int horse_randombytes_install(void)
{
    return 0;
}

int horse_randombytes_failed(void)
{
    return 0;
}

#endif

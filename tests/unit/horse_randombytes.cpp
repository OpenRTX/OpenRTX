/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HASH_RNG policy: seed/clock flags, discard first word, reject repeats,
 * fail closed. Hardware is mocked.
 */

#include "protocols/horse/horse_randombytes.h"
#include "protocols/horse/horse_crypto.h"
#include <cstdio>
#include <cstring>
#include <cstdint>

static int expect_ok(int cond, const char *msg)
{
    if (!cond) {
        std::printf("horse_randombytes_test: %s\n", msg);
        return 0;
    }
    return 1;
}

int main(void)
{
    uint32_t words[8];
    uint32_t w = 0;
    uint8_t pk[32], sk[32];
    size_t i;

    horse_randombytes_test_reset();
    words[0] = 0x11111111u;
    words[1] = 0x22222222u;
    words[2] = 0x33333333u;
    horse_randombytes_test_feed(words, 3);
    if (horse_randombytes_install() != 0) {
        std::printf("horse_randombytes_test: install failed on good feed\n");
        return -1;
    }
    if (!expect_ok(horse_randombytes_test_read(&w) == 0 && w == 0x22222222u,
                   "first consumed word must be after discarded enable word"))
        return -1;
    if (!expect_ok(horse_randombytes_test_read(&w) == 0 && w == 0x33333333u,
                   "second consumed word"))
        return -1;

    horse_randombytes_test_reset();
    horse_randombytes_test_set_sr(1, 0);
    words[0] = 1;
    horse_randombytes_test_feed(words, 1);
    if (!expect_ok(horse_randombytes_install() != 0
                       && horse_randombytes_failed(),
                   "seed error must fail closed"))
        return -1;

    horse_randombytes_test_reset();
    horse_randombytes_test_set_sr(0, 1);
    horse_randombytes_test_feed(words, 1);
    if (!expect_ok(horse_randombytes_install() != 0
                       && horse_randombytes_failed(),
                   "clock error must fail closed"))
        return -1;

    horse_randombytes_test_reset();
    words[0] = 0xaaaaaaaau;
    words[1] = 0xbbbbbbbbu;
    words[2] = 0xbbbbbbbbu;
    horse_randombytes_test_feed(words, 3);
    if (horse_randombytes_install() != 0)
        return -1;
    if (!expect_ok(horse_randombytes_test_read(&w) == 0 && w == 0xbbbbbbbbu,
                   "unique word after discard"))
        return -1;
    if (!expect_ok(horse_randombytes_test_read(&w) != 0
                       && horse_randombytes_failed(),
                   "identical consecutive words must fail"))
        return -1;

#ifdef HAVE_LIBSODIUM
    horse_randombytes_test_reset();
    horse_randombytes_test_set_sr(1, 0);
    words[0] = 0x12345678u;
    horse_randombytes_test_feed(words, 1);
    memset(pk, 0xff, sizeof pk);
    memset(sk, 0xff, sizeof sk);
    if (horse_crypto_x25519_keypair(pk, sk) || !horse_randombytes_failed()
        || horse_crypto_available()) {
        std::printf("horse_randombytes_test: keygen did not fail closed\n");
        return -1;
    }
    for (i = 0; i < sizeof sk; i++) {
        if (sk[i] != 0) {
            std::printf("horse_randombytes_test: secret not wiped\n");
            return -1;
        }
    }
#else
    (void)pk;
    (void)sk;
    (void)i;
#endif
    return 0;
}

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/horse/horse_keystore.h"
#include "protocols/horse/horse_crypto.h"
#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef HAVE_LIBSODIUM
#include <sodium.h>
#endif

static volatile int stop_flag;
static volatile int torn;

static void *copier(void *)
{
    horse_identity_keys_t id;
    while (!stop_flag) {
        if (horse_keystore_copy_identity(&id)) {
            if (id.version != 1)
                torn = 1;
            if (memcmp(id.ed25519_sk + 32, id.ed25519_pk, 32) != 0)
                torn = 1;
        }
        horse_crypto_memzero(&id, sizeof id);
    }
    return nullptr;
}

static void *locker(void *)
{
    while (!stop_flag) {
        horse_keystore_lock();
        usleep(10);
        horse_identity_keys_t id;
        if (horse_keystore_copy_identity(&id)) {
            if (id.version != 1)
                torn = 1;
            if (memcmp(id.ed25519_sk + 32, id.ed25519_pk, 32) != 0)
                torn = 1;
        }
        horse_crypto_memzero(&id, sizeof id);
        usleep(10);
    }
    return nullptr;
}

int main()
{
#ifndef HAVE_LIBSODIUM
    std::printf("horse_keystore_test: skipped (no libsodium)\n");
    return 77;
#else
    if (sodium_init() < 0)
        return -1;

    horse_identity_keys_t id;
    memset(&id, 0, sizeof id);
    id.version = 1;
    crypto_sign_ed25519_keypair(id.ed25519_pk, id.ed25519_sk);
    crypto_kx_keypair(id.x25519_pk, id.x25519_sk);

    horse_keystore_init();
    if (horse_keystore_copy_identity(&id))
        return -1;

    if (horse_keystore_hold_passphrase("", 0)
        || horse_keystore_hold_passphrase("1234567", 7)
        || horse_keystore_unlock("", 0) || horse_keystore_unlock("1234567", 7)
        || horse_keystore_store_plaintext(&id, "", 0)
        || horse_keystore_store_plaintext(&id, "1234567", 7)) {
        std::printf("horse_keystore_test: short passphrase accepted\n");
        horse_crypto_memzero(&id, sizeof id);
        return -1;
    }

    char xdg[64];
    snprintf(xdg, sizeof xdg, "/tmp/horse_ksXXXXXX");
    if (mkdtemp(xdg) == NULL) {
        std::printf("horse_keystore_test: mkdtemp failed\n");
        horse_crypto_memzero(&id, sizeof id);
        return -1;
    }
    char appdir[96];
    snprintf(appdir, sizeof appdir, "%s/OpenRTX", xdg);
    mkdir(appdir, 0700);
    setenv("XDG_STATE_HOME", xdg, 1);

    if (!horse_keystore_store_plaintext(&id, "test-pass", 9)) {
        std::printf("horse_keystore_test: store failed\n");
        horse_crypto_memzero(&id, sizeof id);
        return -1;
    }

    {
        char ident[128];
        snprintf(ident, sizeof ident, "%s/OpenRTX/horse_identity.bin", xdg);
        int fd = open(ident, O_RDONLY);
        unsigned char hdr[16];
        uint32_t ops = 0, mem = 0;
        if (fd < 0 || read(fd, hdr, sizeof hdr) != (ssize_t)sizeof hdr) {
            std::printf("horse_keystore_test: blob header read failed\n");
            return -1;
        }
        close(fd);
        memcpy(&ops, hdr + 8, 4);
        memcpy(&mem, hdr + 12, 4);
        if (hdr[4] != HORSE_IDENTITY_STORE_VERSION
            || hdr[5] != HORSE_KDF_VERSION || ops != HORSE_ARGON2ID_OPSLIMIT
            || mem != HORSE_ARGON2ID_MEMLIMIT) {
            std::printf("horse_keystore_test: blob header mismatch\n");
            return -1;
        }
    }

    if (!horse_keystore_hold_passphrase("test-pass", 9)
        || !horse_keystore_has_passphrase()) {
        std::printf("horse_keystore_test: hold passphrase failed\n");
        return -1;
    }
    horse_identity_keys_t out;
    memset(&out, 0, sizeof out);
    horse_keystore_lock();
    if (horse_keystore_has_passphrase() || horse_keystore_unlock_held()
        || horse_keystore_copy_identity(&out)) {
        std::printf("horse_keystore_test: lock did not wipe passphrase/keys\n");
        return -1;
    }
    if (!horse_keystore_hold_passphrase("test-pass", 9)
        || !horse_keystore_unlock_held()) {
        std::printf("horse_keystore_test: unlock_held failed\n");
        return -1;
    }

    if (!horse_keystore_copy_identity(&out)
        || memcmp(out.ed25519_pk, id.ed25519_pk, sizeof id.ed25519_pk) != 0) {
        std::printf("horse_keystore_test: copy mismatch\n");
        return -1;
    }

    pthread_t t1, t2;
    stop_flag = 0;
    torn = 0;
    pthread_create(&t1, nullptr, copier, nullptr);
    pthread_create(&t2, nullptr, locker, nullptr);
    usleep(50000);
    if (!horse_keystore_store_plaintext(&id, "test-pass", 9))
        return -1;
    usleep(50000);
    stop_flag = 1;
    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    horse_crypto_memzero(&id, sizeof id);
    horse_crypto_memzero(&out, sizeof out);
    horse_keystore_lock();

    if (torn) {
        std::printf("horse_keystore_test: torn identity observed\n");
        return -1;
    }
    std::printf("horse_keystore_test: passed\n");
    return 0;
#endif
}

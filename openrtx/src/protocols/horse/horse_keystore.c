/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/horse/horse_keystore.h"
#include <string.h>

#ifdef HAVE_LIBSODIUM
#include <sodium.h>
#endif

#ifdef PLATFORM_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#else
#include "core/nvmem_access.h"
#endif

#define HORSE_STORE_MAGIC 0x484B5354U /* "HKST" */
#define HORSE_STORE_VERSION 1U
#define HORSE_KDF_SALT_BYTES 16U

#ifndef HORSE_IDENTITY_NVM_OFFSET
#define HORSE_IDENTITY_NVM_OFFSET 0x00FE0000U
#endif

typedef struct
{
    uint32_t magic;
    uint8_t version;
    uint8_t salt[HORSE_KDF_SALT_BYTES];
    uint16_t blob_len;
    uint8_t blob[256];
} horse_identity_store_t;

static horse_identity_keys_t unlocked_identity;
static bool identity_unlocked;

static int horse_store_read(horse_identity_store_t *store)
{
#ifdef PLATFORM_LINUX
    const char *env = getenv("XDG_STATE_HOME");
    const char *home = getenv("HOME");
    char path[256];

    if (env != NULL)
        snprintf(path, sizeof path, "%s/OpenRTX/horse_identity.bin", env);
    else if (home != NULL)
        snprintf(path, sizeof path, "%s/.local/state/OpenRTX/horse_identity.bin",
                 home);
    else
        return -1;

    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;

    ssize_t n = read(fd, store, sizeof *store);
    close(fd);
    if (n != (ssize_t)sizeof *store)
        return -1;
    return 0;
#else
    return nvm_read(0, 0, HORSE_IDENTITY_NVM_OFFSET, store, sizeof *store);
#endif
}

static int horse_store_write(const horse_identity_store_t *store)
{
#ifdef PLATFORM_LINUX
    const char *env = getenv("XDG_STATE_HOME");
    const char *home = getenv("HOME");
    char dir[256];
    char path[256];

    if (env != NULL)
        snprintf(dir, sizeof dir, "%s/OpenRTX", env);
    else if (home != NULL)
        snprintf(dir, sizeof dir, "%s/.local/state/OpenRTX", home);
    else
        return -1;

    mkdir(dir, 0700);
    snprintf(path, sizeof path, "%s/horse_identity.bin", dir);

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0)
        return -1;

    ssize_t n = write(fd, store, sizeof *store);
    close(fd);
    return (n == (ssize_t)sizeof *store) ? 0 : -1;
#else
    return nvm_write(0, 0, HORSE_IDENTITY_NVM_OFFSET, store, sizeof *store);
#endif
}

static bool horse_derive_wrap_key(const char *passphrase, size_t passphrase_len,
                                  const uint8_t *salt, uint8_t *key_out)
{
    return horse_crypto_argon2id_derive(passphrase, passphrase_len, salt,
                                        HORSE_KDF_SALT_BYTES, key_out,
                                        HORSE_SESSION_KEY_BYTES);
}

void horse_keystore_init(void)
{
    identity_unlocked = false;
    memset(&unlocked_identity, 0, sizeof unlocked_identity);
}

void horse_keystore_terminate(void)
{
    horse_keystore_lock();
}

bool horse_keystore_is_unlocked(void)
{
    return identity_unlocked;
}

const horse_identity_keys_t *horse_keystore_get_identity(void)
{
    if (!identity_unlocked)
        return NULL;
    return &unlocked_identity;
}

void horse_keystore_lock(void)
{
    memset(&unlocked_identity, 0, sizeof unlocked_identity);
    identity_unlocked = false;
}

bool horse_keystore_unlock(const char *passphrase, size_t passphrase_len)
{
    horse_identity_store_t store;
    uint8_t wrap_key[HORSE_SESSION_KEY_BYTES];

    if (!horse_crypto_available() || passphrase == NULL)
        return false;

    if (horse_store_read(&store) != 0)
        return false;

    if (store.magic != HORSE_STORE_MAGIC || store.version != HORSE_STORE_VERSION)
        return false;

    if (store.blob_len == 0 || store.blob_len > sizeof store.blob)
        return false;

    if (!horse_derive_wrap_key(passphrase, passphrase_len, store.salt, wrap_key))
        return false;

    horse_identity_keys_t identity;
    if (!horse_crypto_decrypt_identity(store.blob, store.blob_len, wrap_key,
                                         HORSE_SESSION_KEY_BYTES, &identity))
        return false;

    horse_keystore_lock();
    unlocked_identity = identity;
    identity_unlocked = true;
    return true;
}

bool horse_keystore_store_plaintext(const horse_identity_keys_t *identity,
                                    const char *passphrase,
                                    size_t passphrase_len)
{
    horse_identity_store_t store;
    uint8_t wrap_key[HORSE_SESSION_KEY_BYTES];
    size_t blob_len = 0;

    if (!horse_crypto_available() || identity == NULL || passphrase == NULL)
        return false;

    memset(&store, 0, sizeof store);
    store.magic = HORSE_STORE_MAGIC;
    store.version = HORSE_STORE_VERSION;

#ifdef HAVE_LIBSODIUM
    randombytes_buf(store.salt, sizeof store.salt);
#else
    return false;
#endif

    if (!horse_derive_wrap_key(passphrase, passphrase_len, store.salt, wrap_key))
        return false;

    if (!horse_crypto_encrypt_identity(identity, wrap_key,
                                       HORSE_SESSION_KEY_BYTES,
                                       store.blob, sizeof store.blob,
                                       &blob_len))
        return false;

    store.blob_len = (uint16_t)blob_len;
    if (horse_store_write(&store) != 0)
        return false;

    horse_keystore_lock();
    unlocked_identity = *identity;
    identity_unlocked = true;
    return true;
}

bool horse_keystore_fingerprint(uint8_t fp_out[32])
{
    if (!identity_unlocked || fp_out == NULL)
        return false;
    return horse_crypto_identity_fingerprint(&unlocked_identity, fp_out);
}

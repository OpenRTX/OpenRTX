/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/horse/horse_keystore.h"
#include <string.h>
#include <pthread.h>

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
static pthread_mutex_t identity_mu = PTHREAD_MUTEX_INITIALIZER;

static void horse_identity_wipe(horse_identity_keys_t *id)
{
    horse_crypto_memzero(id, sizeof *id);
}

static int horse_linux_identity_path(char *path, size_t path_len)
{
    const char *env = getenv("XDG_STATE_HOME");
    const char *home = getenv("HOME");
    int n;

    if (env != NULL)
        n = snprintf(path, path_len, "%s/OpenRTX/horse_identity.bin", env);
    else if (home != NULL)
        n = snprintf(path, path_len, "%s/.local/state/OpenRTX/horse_identity.bin",
                     home);
    else
        return -1;

    if (n < 0 || (size_t)n >= path_len)
        return -1;
    return 0;
}

static int horse_linux_identity_dir(char *dir, size_t dir_len)
{
    const char *env = getenv("XDG_STATE_HOME");
    const char *home = getenv("HOME");
    int n;

    if (env != NULL)
        n = snprintf(dir, dir_len, "%s/OpenRTX", env);
    else if (home != NULL)
        n = snprintf(dir, dir_len, "%s/.local/state/OpenRTX", home);
    else
        return -1;

    if (n < 0 || (size_t)n >= dir_len)
        return -1;
    return 0;
}

static int horse_store_read(horse_identity_store_t *store)
{
#ifdef PLATFORM_LINUX
    char path[512];

    if (horse_linux_identity_path(path, sizeof path) != 0)
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
    char dir[512];
    char path[512];

    if (horse_linux_identity_dir(dir, sizeof dir) != 0)
        return -1;
    {
        int n = snprintf(path, sizeof path, "%s/horse_identity.bin", dir);
        if (n < 0 || (size_t)n >= sizeof path)
            return -1;
    }

    mkdir(dir, 0700);

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
    pthread_mutex_lock(&identity_mu);
    identity_unlocked = false;
    horse_identity_wipe(&unlocked_identity);
    pthread_mutex_unlock(&identity_mu);
}

void horse_keystore_terminate(void)
{
    horse_keystore_lock();
}

bool horse_keystore_is_unlocked(void)
{
    bool unlocked;

    pthread_mutex_lock(&identity_mu);
    unlocked = identity_unlocked;
    pthread_mutex_unlock(&identity_mu);
    return unlocked;
}

bool horse_keystore_copy_identity(horse_identity_keys_t *out)
{
    bool ok = false;

    if (out == NULL)
        return false;

    pthread_mutex_lock(&identity_mu);
    if (identity_unlocked)
    {
        *out = unlocked_identity;
        ok = true;
    }
    pthread_mutex_unlock(&identity_mu);
    return ok;
}

void horse_keystore_lock(void)
{
    pthread_mutex_lock(&identity_mu);
    horse_identity_wipe(&unlocked_identity);
    identity_unlocked = false;
    pthread_mutex_unlock(&identity_mu);
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
    {
        horse_crypto_memzero(wrap_key, sizeof wrap_key);
        return false;
    }

    horse_crypto_memzero(wrap_key, sizeof wrap_key);
    pthread_mutex_lock(&identity_mu);
    horse_identity_wipe(&unlocked_identity);
    unlocked_identity = identity;
    identity_unlocked = true;
    pthread_mutex_unlock(&identity_mu);
    horse_identity_wipe(&identity);
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
    {
        horse_crypto_memzero(wrap_key, sizeof wrap_key);
        return false;
    }

    horse_crypto_memzero(wrap_key, sizeof wrap_key);
    store.blob_len = (uint16_t)blob_len;
    if (horse_store_write(&store) != 0)
        return false;

    pthread_mutex_lock(&identity_mu);
    horse_identity_wipe(&unlocked_identity);
    unlocked_identity = *identity;
    identity_unlocked = true;
    pthread_mutex_unlock(&identity_mu);
    return true;
}

bool horse_keystore_fingerprint(uint8_t fp_out[32])
{
    horse_identity_keys_t id;
    bool ok;

    if (fp_out == NULL)
        return false;
    if (!horse_keystore_copy_identity(&id))
        return false;
    ok = horse_crypto_identity_fingerprint(&id, fp_out);
    horse_identity_wipe(&id);
    return ok;
}

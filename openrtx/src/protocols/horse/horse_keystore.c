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

#ifndef HORSE_IDENTITY_NVM_OFFSET
#define HORSE_IDENTITY_NVM_OFFSET 0x00FE0000U
#endif

typedef struct
{
    uint32_t magic;
    uint8_t store_version;
    uint8_t kdf_version;
    uint8_t reserved[2];
    uint32_t opslimit;
    uint32_t memlimit;
    uint8_t salt[HORSE_KDF_SALT_BYTES];
    uint16_t blob_len;
    uint8_t blob[256];
} __attribute__((packed)) horse_identity_store_t;

static horse_identity_keys_t unlocked_identity;
static bool identity_unlocked;
static char held_passphrase[HORSE_PASSPHRASE_MAX + 1];
static size_t held_passphrase_len;
static bool have_passphrase;
static pthread_mutex_t identity_mu = PTHREAD_MUTEX_INITIALIZER;

static void horse_identity_wipe(horse_identity_keys_t *id)
{
    horse_crypto_memzero(id, sizeof *id);
}

static void horse_passphrase_wipe_locked(void)
{
    horse_crypto_memzero(held_passphrase, sizeof held_passphrase);
    held_passphrase_len = 0;
    have_passphrase = false;
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
    horse_passphrase_wipe_locked();
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
    horse_passphrase_wipe_locked();
    pthread_mutex_unlock(&identity_mu);
}

bool horse_keystore_hold_passphrase(const char *passphrase, size_t passphrase_len)
{
    if (passphrase == NULL || passphrase_len == 0 ||
        passphrase_len > HORSE_PASSPHRASE_MAX)
        return false;

    pthread_mutex_lock(&identity_mu);
    horse_passphrase_wipe_locked();
    memcpy(held_passphrase, passphrase, passphrase_len);
    held_passphrase[passphrase_len] = '\0';
    held_passphrase_len = passphrase_len;
    have_passphrase = true;
    pthread_mutex_unlock(&identity_mu);
    return true;
}

bool horse_keystore_has_passphrase(void)
{
    bool have;

    pthread_mutex_lock(&identity_mu);
    have = have_passphrase;
    pthread_mutex_unlock(&identity_mu);
    return have;
}

bool horse_keystore_unlock_held(void)
{
    char pass[HORSE_PASSPHRASE_MAX + 1];
    size_t n;
    bool ok;

    pthread_mutex_lock(&identity_mu);
    if (!have_passphrase)
    {
        pthread_mutex_unlock(&identity_mu);
        return false;
    }
    n = held_passphrase_len;
    memcpy(pass, held_passphrase, n);
    pass[n] = '\0';
    pthread_mutex_unlock(&identity_mu);

    ok = horse_keystore_unlock(pass, n);
    horse_crypto_memzero(pass, sizeof pass);
    return ok;
}

bool horse_keystore_unlock(const char *passphrase, size_t passphrase_len)
{
    horse_identity_store_t store;
    uint8_t wrap_key[HORSE_SESSION_KEY_BYTES];

    if (!horse_crypto_available() || passphrase == NULL)
        return false;

    if (horse_store_read(&store) != 0)
        return false;

    if (store.magic != HORSE_STORE_MAGIC ||
        store.store_version != HORSE_IDENTITY_STORE_VERSION ||
        store.kdf_version != HORSE_KDF_VERSION ||
        store.opslimit != HORSE_ARGON2ID_OPSLIMIT ||
        store.memlimit != HORSE_ARGON2ID_MEMLIMIT)
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
    store.store_version = HORSE_IDENTITY_STORE_VERSION;
    store.kdf_version = HORSE_KDF_VERSION;
    store.opslimit = HORSE_ARGON2ID_OPSLIMIT;
    store.memlimit = HORSE_ARGON2ID_MEMLIMIT;

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

bool horse_keystore_store_with_held(const horse_identity_keys_t *identity)
{
    char pass[HORSE_PASSPHRASE_MAX + 1];
    size_t n;
    bool ok;

    pthread_mutex_lock(&identity_mu);
    if (!have_passphrase)
    {
        pthread_mutex_unlock(&identity_mu);
        return false;
    }
    n = held_passphrase_len;
    memcpy(pass, held_passphrase, n);
    pass[n] = '\0';
    pthread_mutex_unlock(&identity_mu);

    ok = horse_keystore_store_plaintext(identity, pass, n);
    horse_crypto_memzero(pass, sizeof pass);
    return ok;
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

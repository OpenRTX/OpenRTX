/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Horse crypto worker hand-off and host stack high-water of libsodium
 * calls. Cortex-M4 figures will differ from these host measurements.
 */

#include "protocols/horse/horse_crypto.h"
#include "protocols/horse/horse_crypto_worker.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <pthread.h>

#ifdef HAVE_LIBSODIUM
#include <sodium.h>
#endif

enum { PROBE_STK = 256 * 1024 };

struct probe_job {
    void (*fn)(void);
    size_t used;
    uint8_t *stk;
};

static void probe_empty(void)
{
}

static uint8_t g_pk[32], g_sk[32], g_pk2[32], g_sk2[32];
static uint8_t g_kenc[32], g_ktag[32];
static uint8_t g_edpk[32], g_edsk[64], g_sig[64];
static uint8_t g_msg[46], g_ct[12], g_pt[12], g_tag[4], g_nonce[12];
static uint8_t g_src[6] = { 1, 2, 3, 4, 5, 6 };
static uint8_t g_dst[6] = { 6, 5, 4, 3, 2, 1 };
static uint8_t g_argon[32];
static uint8_t g_salt[16];

static void probe_x25519(void)
{
    (void)horse_crypto_x25519_keypair(g_pk, g_sk);
}

static void probe_derive(void)
{
    (void)horse_crypto_derive_session_keys(g_sk, g_pk2, g_src, g_dst, g_pk, 1,
                                           HORSE_LSF_VERSION, g_kenc, g_ktag);
}

static void probe_sign(void)
{
    (void)horse_crypto_sign(g_edsk, g_msg, sizeof g_msg, g_sig);
}

static void probe_verify(void)
{
    (void)horse_crypto_verify(g_edpk, g_msg, sizeof g_msg, g_sig);
}

static void probe_blake2_xchacha(void)
{
    horse_crypto_voice_nonce_from_fn(1, g_nonce);
    horse_crypto_voice_encrypt(g_kenc, g_ktag, HORSE_VOICE_DIR_FORWARD, 1,
                               g_nonce, g_pt, 12, g_ct, g_tag);
}

static void probe_argon2(void)
{
    (void)horse_crypto_argon2id_derive("test-pass", 9, g_salt, sizeof g_salt,
                                       g_argon, 32);
}

#ifdef HAVE_LIBSODIUM
static void probe_blake2b_raw(void)
{
    uint8_t out[32];
    crypto_generichash(out, sizeof out, g_msg, sizeof g_msg, g_ktag, 32);
}

static void probe_xchacha_raw(void)
{
    uint8_t n[24];
    memset(n, 0, sizeof n);
    crypto_stream_xchacha20_xor(g_ct, g_pt, 12, n, g_kenc);
}
#endif

static void *probe_thread(void *arg)
{
    struct probe_job *j = (struct probe_job *)arg;
    j->fn();
    return NULL;
}

static int measure_fn(const char *name, void (*fn)(void), size_t empty)
{
    struct probe_job j {
    };
    pthread_attr_t attr;
    pthread_t th;
    void *stk = NULL;
    uint8_t *p;
    size_t i, used;

    if (posix_memalign(&stk, 16, PROBE_STK) != 0)
        return -1;
    memset(stk, 0xA5, PROBE_STK);
    j.fn = fn;
    j.stk = (uint8_t *)stk;
    pthread_attr_init(&attr);
    if (pthread_attr_setstack(&attr, stk, PROBE_STK) != 0) {
        free(stk);
        return -1;
    }
    if (pthread_create(&th, &attr, probe_thread, &j) != 0) {
        pthread_attr_destroy(&attr);
        free(stk);
        return -1;
    }
    pthread_join(th, NULL);
    pthread_attr_destroy(&attr);
    p = (uint8_t *)stk;
    for (i = 0; i < PROBE_STK; i++) {
        if (p[i] != 0xA5)
            break;
    }
    used = PROBE_STK - i;
    std::printf("crypto_stack host %s used=%zu delta_vs_empty=%ld "
                "(Cortex-M4 will differ)\n",
                name, used, (long)used - (long)empty);
    free(stk);
    return 0;
}

static int test_stack_hwm(void)
{
    if (!horse_crypto_available()) {
        std::printf("horse_crypto_worker_test: skipped (no libsodium)\n");
        return 77;
    }
    memset(g_pt, 0x11, sizeof g_pt);
    memset(g_salt, 0x22, sizeof g_salt);
    memset(g_msg, 0x33, sizeof g_msg);
    if (!horse_crypto_x25519_keypair(g_pk, g_sk)
        || !horse_crypto_x25519_keypair(g_pk2, g_sk2))
        return -1;
#ifdef HAVE_LIBSODIUM
    if (crypto_sign_keypair(g_edpk, g_edsk) != 0)
        return -1;
#else
    return -1;
#endif
    if (!horse_crypto_sign(g_edsk, g_msg, sizeof g_msg, g_sig))
        return -1;
    if (!horse_crypto_derive_session_keys(g_sk, g_pk2, g_src, g_dst, g_pk, 1,
                                          HORSE_LSF_VERSION, g_kenc, g_ktag))
        return -1;

    size_t empty_used = 0;
    {
        struct probe_job j {
        };
        pthread_attr_t attr;
        pthread_t th;
        void *stk = NULL;
        if (posix_memalign(&stk, 16, PROBE_STK) != 0)
            return -1;
        memset(stk, 0xA5, PROBE_STK);
        j.fn = probe_empty;
        pthread_attr_init(&attr);
        pthread_attr_setstack(&attr, stk, PROBE_STK);
        if (pthread_create(&th, &attr, probe_thread, &j) != 0)
            return -1;
        pthread_join(th, NULL);
        pthread_attr_destroy(&attr);
        uint8_t *p = (uint8_t *)stk;
        size_t i;
        for (i = 0; i < PROBE_STK; i++) {
            if (p[i] != 0xA5)
                break;
        }
        empty_used = PROBE_STK - i;
        free(stk);
        std::printf("crypto_stack host empty used=%zu\n", empty_used);
    }

    if (measure_fn("X25519_keypair", probe_x25519, empty_used) != 0)
        return -1;
    if (measure_fn("X25519_BLAKE2b_derive", probe_derive, empty_used) != 0)
        return -1;
    if (measure_fn("Ed25519_sign", probe_sign, empty_used) != 0)
        return -1;
    if (measure_fn("Ed25519_verify", probe_verify, empty_used) != 0)
        return -1;
    if (measure_fn("XChaCha20_BLAKE2b_voice", probe_blake2_xchacha, empty_used)
        != 0)
        return -1;
#ifdef HAVE_LIBSODIUM
    if (measure_fn("BLAKE2b_raw", probe_blake2b_raw, empty_used) != 0)
        return -1;
    if (measure_fn("XChaCha20_raw", probe_xchacha_raw, empty_used) != 0)
        return -1;
#endif
    if (measure_fn("Argon2id_16KiB", probe_argon2, empty_used) != 0)
        return -1;
    return 0;
}

static int test_handoff_and_cancel(void)
{
    uint8_t pk[32], sk[32], pk2[32], sk2[32];
    uint8_t kenc[32], ktag[32], sig[64];
    uint8_t ct[12], tag[4], pt[12], out[12];
    uint8_t nonce[12];
    uint8_t argon[32];
    uint8_t salt[16];

    horse_crypto_worker_init();
    if (!horse_crypto_req_x25519_keypair()
        || horse_crypto_wait() != HORSE_CRYPTO_ST_DONE
        || !horse_crypto_take_keypair(pk, sk))
        return -1;
    if (!horse_crypto_req_x25519_keypair()
        || horse_crypto_wait() != HORSE_CRYPTO_ST_DONE
        || !horse_crypto_take_keypair(pk2, sk2))
        return -1;
    if (!horse_crypto_req_derive(sk, pk2, g_src, g_dst, pk, 1,
                                 HORSE_LSF_VERSION)
        || horse_crypto_wait() != HORSE_CRYPTO_ST_DONE
        || !horse_crypto_take_session(kenc, ktag))
        return -1;
#ifdef HAVE_LIBSODIUM
    if (crypto_sign_keypair(g_edpk, g_edsk) != 0)
        return -1;
#endif
    if (!horse_crypto_req_sign(g_edsk, g_msg, sizeof g_msg)
        || horse_crypto_wait() != HORSE_CRYPTO_ST_DONE
        || !horse_crypto_take_sig(sig))
        return -1;
    if (!horse_crypto_req_verify(g_edpk, g_msg, sizeof g_msg, sig)
        || horse_crypto_wait() != HORSE_CRYPTO_ST_DONE)
        return -1;
    memset(pt, 0x44, sizeof pt);
    horse_crypto_voice_nonce_from_fn(3, nonce);
    if (!horse_crypto_req_voice_enc(kenc, ktag, HORSE_VOICE_DIR_FORWARD, 3,
                                    nonce, pt, 12)
        || horse_crypto_wait() != HORSE_CRYPTO_ST_DONE
        || !horse_crypto_take_voice_enc(ct, tag, 12))
        return -1;
    if (!horse_crypto_req_voice_dec(kenc, ktag, HORSE_VOICE_DIR_FORWARD, 3,
                                    nonce, ct, 12, tag)
        || horse_crypto_wait() != HORSE_CRYPTO_ST_DONE
        || !horse_crypto_take_voice_dec(out, 12) || memcmp(out, pt, 12) != 0)
        return -1;

    memset(salt, 7, sizeof salt);
    if (!horse_crypto_req_argon2("ptt-cancel", 10, salt, sizeof salt))
        return -1;
    horse_crypto_cancel();
    horse_crypto_st st = horse_crypto_wait();
    if (st != HORSE_CRYPTO_ST_CANCELLED && st != HORSE_CRYPTO_ST_DONE) {
        std::printf("horse_crypto_worker_test: cancel st=%d\n", (int)st);
        horse_crypto_worker_terminate();
        return -1;
    }
    if (st == HORSE_CRYPTO_ST_DONE)
        (void)horse_crypto_take_argon2(argon);
    std::printf("horse_crypto_worker_test: ptt-release cancel st=%d "
                "(CANCELLED=4)\n",
                (int)st);

    /* Second argon2: cancel immediately (PTT released before compute). */
    if (!horse_crypto_req_argon2("ptt-release", 11, salt, sizeof salt))
        return -1;
    horse_crypto_cancel();
    st = horse_crypto_wait();
    if (st != HORSE_CRYPTO_ST_CANCELLED && st != HORSE_CRYPTO_ST_DONE) {
        std::printf("horse_crypto_worker_test: immediate cancel st=%d\n",
                    (int)st);
        horse_crypto_worker_terminate();
        return -1;
    }
    horse_crypto_worker_terminate();
    return 0;
}

int main()
{
    int rc = test_stack_hwm();
    if (rc != 0)
        return rc;
    rc = test_handoff_and_cancel();
    if (rc != 0)
        return rc;
    std::printf("horse_crypto_worker_test: all tests passed\n");
    return 0;
}

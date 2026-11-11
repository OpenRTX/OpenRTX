/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/horse/horse_crypto_worker.h"
#include "core/threads.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

enum {
    OP_NONE = 0,
    OP_KEYPAIR,
    OP_DERIVE,
    OP_SIGN,
    OP_VERIFY,
    OP_VENC,
    OP_VDEC,
    OP_VTAG,
    OP_VVERIFY,
    OP_ARGON2
};

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static pthread_t thr;
static pthread_attr_t attr;
static int live;
static void *stk;

static struct {
    int op;
    horse_crypto_st st;
    int cancel;
    int ok;
    uint8_t sk[32];
    uint8_t pk[32];
    uint8_t eph[32];
    uint8_t src[6];
    uint8_t dst[6];
    uint8_t flags;
    uint8_t ver;
    uint8_t dir;
    uint16_t fn;
    uint8_t kenc[32];
    uint8_t ktag[32];
    uint8_t nonce[12];
    uint8_t pt[12];
    uint8_t ct[12];
    uint8_t tag[4];
    uint8_t msg[HORSE_SESSION_MSG_BYTES];
    size_t msg_len;
    uint8_t sig[64];
    uint8_t edsk[64];
    uint8_t edpk[32];
    uint8_t pass[HORSE_PASSPHRASE_MAX];
    size_t pass_len;
    uint8_t salt[16];
    size_t salt_len;
    uint8_t argon[32];
    size_t pay_len;
} job;

static void wipe_secrets(void)
{
    horse_crypto_memzero(job.sk, sizeof job.sk);
    horse_crypto_memzero(job.edsk, sizeof job.edsk);
    horse_crypto_memzero(job.kenc, sizeof job.kenc);
    horse_crypto_memzero(job.ktag, sizeof job.ktag);
    horse_crypto_memzero(job.pass, sizeof job.pass);
    horse_crypto_memzero(job.argon, sizeof job.argon);
}

static void run_op(void)
{
    if (job.cancel) {
        job.ok = 0;
        return;
    }
    job.ok = 0;
    switch (job.op) {
        case OP_KEYPAIR:
            job.ok = horse_crypto_x25519_keypair(job.pk, job.sk);
            break;
        case OP_DERIVE:
            job.ok = horse_crypto_derive_session_keys(job.sk, job.pk, job.src,
                                                      job.dst, job.eph,
                                                      job.flags, job.ver,
                                                      job.kenc, job.ktag);
            break;
        case OP_SIGN:
            job.ok = horse_crypto_sign(job.edsk, job.msg, job.msg_len, job.sig);
            break;
        case OP_VERIFY:
            job.ok = horse_crypto_verify(job.edpk, job.msg, job.msg_len,
                                         job.sig);
            break;
        case OP_VENC:
            horse_crypto_voice_encrypt(job.kenc, job.ktag, job.dir, job.fn,
                                       job.nonce, job.pt, job.pay_len, job.ct,
                                       job.tag);
            job.ok = 1;
            break;
        case OP_VDEC:
            job.ok = horse_crypto_voice_decrypt(job.kenc, job.ktag, job.dir,
                                                job.fn, job.nonce, job.ct,
                                                job.pay_len, job.tag, job.pt);
            break;
        case OP_VTAG:
            job.ok = horse_crypto_voice_auth_tag(job.ktag, job.dir, job.fn,
                                                 job.pt, job.tag);
            break;
        case OP_VVERIFY:
            job.ok = horse_crypto_voice_auth_verify(job.ktag, job.dir, job.fn,
                                                    job.pt, job.tag);
            break;
        case OP_ARGON2:
            job.ok = horse_crypto_argon2id_derive((char *)job.pass,
                                                  job.pass_len, job.salt,
                                                  job.salt_len, job.argon, 32);
            break;
        default:
            break;
    }
}

static void *worker_main(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&mu);
    while (live) {
        while (live && job.st != HORSE_CRYPTO_ST_PENDING)
            pthread_cond_wait(&cv, &mu);
        if (!live)
            break;
        if (job.cancel) {
            job.st = HORSE_CRYPTO_ST_CANCELLED;
            wipe_secrets();
            pthread_cond_broadcast(&cv);
            continue;
        }
        pthread_mutex_unlock(&mu);
        run_op();
        pthread_mutex_lock(&mu);
        if (job.cancel) {
            job.st = HORSE_CRYPTO_ST_CANCELLED;
            wipe_secrets();
        } else
            job.st = job.ok ? HORSE_CRYPTO_ST_DONE : HORSE_CRYPTO_ST_FAILED;
        pthread_cond_broadcast(&cv);
    }
    pthread_mutex_unlock(&mu);
    return NULL;
}

static bool prepare_op(int op)
{
    if (!live || job.st == HORSE_CRYPTO_ST_PENDING)
        return false;
    memset(&job, 0, sizeof job);
    job.op = op;
    job.cancel = 0;
    return true;
}

static void launch_op(void)
{
    job.st = HORSE_CRYPTO_ST_PENDING;
    pthread_cond_signal(&cv);
}

void horse_crypto_worker_init(void)
{
    pthread_mutex_lock(&mu);
    if (live) {
        pthread_mutex_unlock(&mu);
        return;
    }
    memset(&job, 0, sizeof job);
    live = 1;
    pthread_mutex_unlock(&mu);
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, CODEC2_THREAD_STKSIZE);
#if !defined(_MIOSIX) && !defined(__ZEPHYR__)
    stk = malloc(CODEC2_THREAD_STKSIZE);
    if (stk)
        pthread_attr_setstack(&attr, stk, CODEC2_THREAD_STKSIZE);
#endif
    if (pthread_create(&thr, &attr, worker_main, NULL) != 0) {
        pthread_mutex_lock(&mu);
        live = 0;
        pthread_mutex_unlock(&mu);
    }
}

void horse_crypto_worker_terminate(void)
{
    pthread_mutex_lock(&mu);
    if (!live) {
        pthread_mutex_unlock(&mu);
        return;
    }
    live = 0;
    job.cancel = 1;
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&mu);
    pthread_join(thr, NULL);
    pthread_attr_destroy(&attr);
    wipe_secrets();
#if !defined(_MIOSIX) && !defined(__ZEPHYR__)
    free(stk);
    stk = NULL;
#endif
}

void horse_crypto_cancel(void)
{
    pthread_mutex_lock(&mu);
    job.cancel = 1;
    if (job.st == HORSE_CRYPTO_ST_PENDING)
        pthread_cond_broadcast(&cv);
    else if (job.st == HORSE_CRYPTO_ST_IDLE)
        ;
    else {
        job.st = HORSE_CRYPTO_ST_CANCELLED;
        wipe_secrets();
        pthread_cond_broadcast(&cv);
    }
    pthread_mutex_unlock(&mu);
}

horse_crypto_st horse_crypto_poll(void)
{
    horse_crypto_st st;
    pthread_mutex_lock(&mu);
    st = job.st;
    pthread_mutex_unlock(&mu);
    return st;
}

horse_crypto_st horse_crypto_wait(void)
{
    horse_crypto_st st;
    pthread_mutex_lock(&mu);
    while (job.st == HORSE_CRYPTO_ST_PENDING)
        pthread_cond_wait(&cv, &mu);
    st = job.st;
    pthread_mutex_unlock(&mu);
    return st;
}

bool horse_crypto_req_x25519_keypair(void)
{
    bool ok;
    pthread_mutex_lock(&mu);
    ok = prepare_op(OP_KEYPAIR);
    if (ok)
        launch_op();
    pthread_mutex_unlock(&mu);
    return ok;
}

bool horse_crypto_take_keypair(uint8_t pk[32], uint8_t sk[32])
{
    bool ok = false;
    pthread_mutex_lock(&mu);
    if (job.st == HORSE_CRYPTO_ST_DONE && job.op == OP_KEYPAIR) {
        memcpy(pk, job.pk, 32);
        memcpy(sk, job.sk, 32);
        ok = true;
        job.st = HORSE_CRYPTO_ST_IDLE;
        wipe_secrets();
    }
    pthread_mutex_unlock(&mu);
    return ok;
}

bool horse_crypto_req_derive(const uint8_t *local_sk, const uint8_t *remote_pk,
                             const uint8_t src[6], const uint8_t dst[6],
                             const uint8_t eph_pk[32], uint8_t flags,
                             uint8_t version)
{
    bool ok;
    pthread_mutex_lock(&mu);
    ok = prepare_op(OP_DERIVE);
    if (ok) {
        memcpy(job.sk, local_sk, 32);
        memcpy(job.pk, remote_pk, 32);
        memcpy(job.src, src, 6);
        memcpy(job.dst, dst, 6);
        memcpy(job.eph, eph_pk, 32);
        job.flags = flags;
        job.ver = version;
        launch_op();
    }
    pthread_mutex_unlock(&mu);
    return ok;
}

bool horse_crypto_take_session(uint8_t k_enc[32], uint8_t k_tag[32])
{
    bool ok = false;
    pthread_mutex_lock(&mu);
    if (job.st == HORSE_CRYPTO_ST_DONE && job.op == OP_DERIVE) {
        memcpy(k_enc, job.kenc, 32);
        memcpy(k_tag, job.ktag, 32);
        ok = true;
        job.st = HORSE_CRYPTO_ST_IDLE;
        wipe_secrets();
    }
    pthread_mutex_unlock(&mu);
    return ok;
}

bool horse_crypto_req_sign(const uint8_t sk[64], const uint8_t *msg,
                           size_t msg_len)
{
    bool ok;
    if (msg_len > sizeof job.msg)
        return false;
    pthread_mutex_lock(&mu);
    ok = prepare_op(OP_SIGN);
    if (ok) {
        memcpy(job.edsk, sk, 64);
        memcpy(job.msg, msg, msg_len);
        job.msg_len = msg_len;
        launch_op();
    }
    pthread_mutex_unlock(&mu);
    return ok;
}

bool horse_crypto_take_sig(uint8_t sig[64])
{
    bool ok = false;
    pthread_mutex_lock(&mu);
    if (job.st == HORSE_CRYPTO_ST_DONE && job.op == OP_SIGN) {
        memcpy(sig, job.sig, 64);
        ok = true;
        job.st = HORSE_CRYPTO_ST_IDLE;
        wipe_secrets();
    }
    pthread_mutex_unlock(&mu);
    return ok;
}

bool horse_crypto_req_verify(const uint8_t pk[32], const uint8_t *msg,
                             size_t msg_len, const uint8_t sig[64])
{
    bool ok;
    if (msg_len > sizeof job.msg)
        return false;
    pthread_mutex_lock(&mu);
    ok = prepare_op(OP_VERIFY);
    if (ok) {
        memcpy(job.edpk, pk, 32);
        memcpy(job.msg, msg, msg_len);
        memcpy(job.sig, sig, 64);
        job.msg_len = msg_len;
        launch_op();
    }
    pthread_mutex_unlock(&mu);
    return ok;
}

bool horse_crypto_req_voice_enc(const uint8_t *k_enc, const uint8_t *k_tag,
                                uint8_t dir, uint16_t fn,
                                const uint8_t nonce[12], const uint8_t *pt,
                                size_t pt_len)
{
    bool ok;
    if (pt_len > sizeof job.pt)
        return false;
    pthread_mutex_lock(&mu);
    ok = prepare_op(OP_VENC);
    if (ok) {
        memcpy(job.kenc, k_enc, 32);
        memcpy(job.ktag, k_tag, 32);
        memcpy(job.nonce, nonce, 12);
        memcpy(job.pt, pt, pt_len);
        job.dir = dir;
        job.fn = fn;
        job.pay_len = pt_len;
        launch_op();
    }
    pthread_mutex_unlock(&mu);
    return ok;
}

bool horse_crypto_take_voice_enc(uint8_t *ct, uint8_t tag[4], size_t pt_len)
{
    bool ok = false;
    pthread_mutex_lock(&mu);
    if (job.st == HORSE_CRYPTO_ST_DONE && job.op == OP_VENC) {
        memcpy(ct, job.ct, pt_len);
        memcpy(tag, job.tag, 4);
        ok = true;
        job.st = HORSE_CRYPTO_ST_IDLE;
        wipe_secrets();
    }
    pthread_mutex_unlock(&mu);
    return ok;
}

bool horse_crypto_req_voice_dec(const uint8_t *k_enc, const uint8_t *k_tag,
                                uint8_t dir, uint16_t fn,
                                const uint8_t nonce[12], const uint8_t *ct,
                                size_t ct_len, const uint8_t tag[4])
{
    bool ok;
    if (ct_len > sizeof job.ct)
        return false;
    pthread_mutex_lock(&mu);
    ok = prepare_op(OP_VDEC);
    if (ok) {
        memcpy(job.kenc, k_enc, 32);
        memcpy(job.ktag, k_tag, 32);
        memcpy(job.nonce, nonce, 12);
        memcpy(job.ct, ct, ct_len);
        memcpy(job.tag, tag, 4);
        job.dir = dir;
        job.fn = fn;
        job.pay_len = ct_len;
        launch_op();
    }
    pthread_mutex_unlock(&mu);
    return ok;
}

bool horse_crypto_take_voice_dec(uint8_t *pt, size_t ct_len)
{
    bool ok = false;
    pthread_mutex_lock(&mu);
    if (job.st == HORSE_CRYPTO_ST_DONE && job.op == OP_VDEC) {
        memcpy(pt, job.pt, ct_len);
        ok = true;
        job.st = HORSE_CRYPTO_ST_IDLE;
        wipe_secrets();
    }
    pthread_mutex_unlock(&mu);
    return ok;
}

bool horse_crypto_req_voice_tag(const uint8_t *k_tag, uint8_t dir, uint16_t fn,
                                const uint8_t *payload)
{
    bool ok;
    pthread_mutex_lock(&mu);
    ok = prepare_op(OP_VTAG);
    if (ok) {
        memcpy(job.ktag, k_tag, 32);
        memcpy(job.pt, payload, 12);
        job.dir = dir;
        job.fn = fn;
        launch_op();
    }
    pthread_mutex_unlock(&mu);
    return ok;
}

bool horse_crypto_take_voice_tag(uint8_t tag[4])
{
    bool ok = false;
    pthread_mutex_lock(&mu);
    if (job.st == HORSE_CRYPTO_ST_DONE && job.op == OP_VTAG) {
        memcpy(tag, job.tag, 4);
        ok = true;
        job.st = HORSE_CRYPTO_ST_IDLE;
        wipe_secrets();
    }
    pthread_mutex_unlock(&mu);
    return ok;
}

bool horse_crypto_req_voice_verify(const uint8_t *k_tag, uint8_t dir,
                                   uint16_t fn, const uint8_t *payload,
                                   const uint8_t tag[4])
{
    bool ok;
    pthread_mutex_lock(&mu);
    ok = prepare_op(OP_VVERIFY);
    if (ok) {
        memcpy(job.ktag, k_tag, 32);
        memcpy(job.pt, payload, 12);
        memcpy(job.tag, tag, 4);
        job.dir = dir;
        job.fn = fn;
        launch_op();
    }
    pthread_mutex_unlock(&mu);
    return ok;
}

bool horse_crypto_req_argon2(const char *pass, size_t pass_len,
                             const uint8_t *salt, size_t salt_len)
{
    bool ok;
    if (pass_len > sizeof job.pass || salt_len > sizeof job.salt)
        return false;
    pthread_mutex_lock(&mu);
    ok = prepare_op(OP_ARGON2);
    if (ok) {
        memcpy(job.pass, pass, pass_len);
        job.pass_len = pass_len;
        memcpy(job.salt, salt, salt_len);
        job.salt_len = salt_len;
        launch_op();
    }
    pthread_mutex_unlock(&mu);
    return ok;
}

bool horse_crypto_take_argon2(uint8_t key[32])
{
    bool ok = false;
    pthread_mutex_lock(&mu);
    if (job.st == HORSE_CRYPTO_ST_DONE && job.op == OP_ARGON2) {
        memcpy(key, job.argon, 32);
        ok = true;
        job.st = HORSE_CRYPTO_ST_IDLE;
        wipe_secrets();
    }
    pthread_mutex_unlock(&mu);
    return ok;
}

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Dedicated Horse crypto worker. RTX posts a job and either polls on
 * the next tick (key agreement) or waits on a condition variable
 * (voice frames). Sodium never runs on the 512 B RTX stack.
 */

#ifndef HORSE_CRYPTO_WORKER_H
#define HORSE_CRYPTO_WORKER_H

#include "protocols/horse/horse_crypto.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HORSE_CRYPTO_ST_IDLE = 0,
    HORSE_CRYPTO_ST_PENDING,
    HORSE_CRYPTO_ST_DONE,
    HORSE_CRYPTO_ST_FAILED,
    HORSE_CRYPTO_ST_CANCELLED
} horse_crypto_st;

void horse_crypto_worker_init(void);
void horse_crypto_worker_terminate(void);

void horse_crypto_cancel(void);
horse_crypto_st horse_crypto_poll(void);
horse_crypto_st horse_crypto_wait(void);

bool horse_crypto_req_x25519_keypair(void);
bool horse_crypto_take_keypair(uint8_t pk[32], uint8_t sk[32]);

bool horse_crypto_req_derive(const uint8_t *local_sk, const uint8_t *remote_pk,
                             const uint8_t src[6], const uint8_t dst[6],
                             const uint8_t eph_pk[32], uint8_t flags,
                             uint8_t version);
bool horse_crypto_take_session(uint8_t k_enc[32], uint8_t k_tag[32]);

bool horse_crypto_req_sign(const uint8_t sk[64], const uint8_t *msg,
                           size_t msg_len);
bool horse_crypto_take_sig(uint8_t sig[64]);

bool horse_crypto_req_verify(const uint8_t pk[32], const uint8_t *msg,
                             size_t msg_len, const uint8_t sig[64]);

bool horse_crypto_req_voice_enc(const uint8_t *k_enc, const uint8_t *k_tag,
                                uint8_t dir, uint16_t fn,
                                const uint8_t nonce[12], const uint8_t *pt,
                                size_t pt_len);
bool horse_crypto_take_voice_enc(uint8_t *ct, uint8_t tag[4], size_t pt_len);

bool horse_crypto_req_voice_dec(const uint8_t *k_enc, const uint8_t *k_tag,
                                uint8_t dir, uint16_t fn,
                                const uint8_t nonce[12], const uint8_t *ct,
                                size_t ct_len, const uint8_t tag[4]);
bool horse_crypto_take_voice_dec(uint8_t *pt, size_t ct_len);

bool horse_crypto_req_voice_tag(const uint8_t *k_tag, uint8_t dir, uint16_t fn,
                                const uint8_t *payload);
bool horse_crypto_take_voice_tag(uint8_t tag[4]);

bool horse_crypto_req_voice_verify(const uint8_t *k_tag, uint8_t dir,
                                   uint16_t fn, const uint8_t *payload,
                                   const uint8_t tag[4]);

bool horse_crypto_req_argon2(const char *pass, size_t pass_len,
                             const uint8_t *salt, size_t salt_len);
bool horse_crypto_take_argon2(uint8_t key[32]);

#ifdef __cplusplus
}
#endif

#endif

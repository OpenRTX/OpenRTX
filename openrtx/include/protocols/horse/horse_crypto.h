/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Horse crypto API.
 *
 * Current implementation uses libsodium where available:
 *  - XChaCha20 stream cipher + BLAKE2b-based 32-bit MAC for voice frames
 *  - X25519 ECDH for session keys (LSF-bound KDF)
 *  - Ed25519 for digital signatures
 *  - Argon2id (via crypto_pwhash) for passphrase-based key derivation
 *
 * Without libsodium every primitive returns false. There is no PBKDF2,
 * cleartext, or stub-crypto fallback.
 */

#ifndef HORSE_CRYPTO_H
#define HORSE_CRYPTO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HORSE_SESSION_KEY_BYTES  32
#define HORSE_VOICE_TAG_BYTES    4
#define HORSE_LSF_VERSION        2
#define HORSE_SESSION_MSG_BYTES  46
#define HORSE_PASSPHRASE_MAX     32
#define HORSE_KDF_VERSION        1
#define HORSE_KDF_SALT_BYTES     16
#define HORSE_IDENTITY_STORE_VERSION 2
/*
 * Shared with horse_provision.py. 16384 B is the largest Argon2id memory
 * cost justified for 192 KiB SRAM: equal to CODEC2_THREAD_STKSIZE, used
 * at mode enable when the codec thread is idle. UNVERIFIED on MD-3x0.
 */
#define HORSE_ARGON2ID_OPSLIMIT  2u
#define HORSE_ARGON2ID_MEMLIMIT  16384u
/* Direction byte in the voice tag: 0 = PTT originator to listeners. */
#define HORSE_VOICE_DIR_FORWARD  0

/* Horse identity keys (libsodium-native sizes). */
#define HORSE_ED25519_PUBLICKEY_BYTES  32
#define HORSE_ED25519_SECRETKEY_BYTES  64
#define HORSE_X25519_PUBLICKEY_BYTES   32
#define HORSE_X25519_SECRETKEY_BYTES   32

/* Ed25519 signature sizes. */
#define HORSE_ED25519_SIGNATURE_BYTES    64

/* Identity key bundle stored/provisioned to the radio. */
typedef struct
{
    uint8_t version; /* must be 1 */
    uint8_t reserved[3];

    uint8_t ed25519_pk[HORSE_ED25519_PUBLICKEY_BYTES];
    uint8_t ed25519_sk[HORSE_ED25519_SECRETKEY_BYTES];

    uint8_t x25519_pk[HORSE_X25519_PUBLICKEY_BYTES];
    uint8_t x25519_sk[HORSE_X25519_SECRETKEY_BYTES];
} __attribute__((packed)) horse_identity_keys_t;

/* Voice frame encrypt: XChaCha20 with k_enc. Tag is keyed BLAKE2b with
 * k_tag over dir || FN16 (no last-frame bit) || payload (ciphertext). */
void horse_crypto_voice_encrypt(
    const uint8_t *k_enc,
    const uint8_t *k_tag,
    uint8_t dir,
    uint16_t frame_num,
    const uint8_t *nonce_96bit,
    const uint8_t *plaintext,
    size_t plaintext_len,
    uint8_t *ciphertext_out,
    uint8_t *tag_truncated_32bit);

/* Voice frame decrypt with constant-time tag check. Missing/wrong tag fails. */
bool horse_crypto_voice_decrypt(
    const uint8_t *k_enc,
    const uint8_t *k_tag,
    uint8_t dir,
    uint16_t frame_num,
    const uint8_t *nonce_96bit,
    const uint8_t *ciphertext,
    size_t ciphertext_len,
    const uint8_t *tag_truncated_32bit,
    uint8_t *plaintext_out);

/* Derive key from passphrase (Argon2id via libsodium). Uses
 * HORSE_ARGON2ID_OPSLIMIT / HORSE_ARGON2ID_MEMLIMIT. No PBKDF2 path. */
bool horse_crypto_argon2id_derive(
    const char *passphrase,
    size_t passphrase_len,
    const uint8_t *salt,
    size_t salt_len,
    uint8_t *key_out,
    size_t key_len);

/* True when a real crypto backend (libsodium) is linked in. */
bool horse_crypto_available(void);

/* Generate an X25519 keypair (public, secret). */
bool horse_crypto_x25519_keypair(uint8_t *pk_out, uint8_t *sk_out);

/*
 * ECDH(local_sk, remote_pk) then KDF over shared || src || dst || eph_pk
 * || flags || version to k_enc and k_tag. Any LSF field change in transit
 * changes the keys, so every voice tag fails.
 */
bool horse_crypto_derive_session_keys(const uint8_t *local_x25519_sk,
                                      const uint8_t *remote_x25519_pk,
                                      const uint8_t src[6],
                                      const uint8_t dst[6],
                                      const uint8_t eph_pk[32],
                                      uint8_t flags,
                                      uint8_t version,
                                      uint8_t k_enc_out[HORSE_SESSION_KEY_BYTES],
                                      uint8_t k_tag_out[HORSE_SESSION_KEY_BYTES]);

/* True when LSF protocol version is the one this build speaks. */
bool horse_crypto_lsf_version_ok(uint8_t version);

/* Build a 96-bit voice nonce from the 16-bit frame counter. */
void horse_crypto_voice_nonce_from_fn(uint16_t frame_num,
                                      uint8_t nonce_96bit[12]);

/* Encrypt/decrypt a horse_identity_keys_t blob with a derived wrap key. */
bool horse_crypto_encrypt_identity(const horse_identity_keys_t *identity,
                                   const uint8_t *wrap_key,
                                   size_t wrap_key_len,
                                   uint8_t *blob_out,
                                   size_t blob_cap,
                                   size_t *blob_len);

bool horse_crypto_decrypt_identity(const uint8_t *blob,
                                   size_t blob_len,
                                   const uint8_t *wrap_key,
                                   size_t wrap_key_len,
                                   horse_identity_keys_t *identity_out);

bool horse_crypto_identity_fingerprint(const horse_identity_keys_t *identity,
                                       uint8_t fp_out[32]);

/* Build src||dst||eph_pk||flags||version (HORSE_SESSION_MSG_BYTES). */
void horse_crypto_build_session_message(const uint8_t src[6],
                                        const uint8_t dst[6],
                                        const uint8_t eph_pk[32],
                                        uint8_t flags,
                                        uint8_t version,
                                        uint8_t message_out[HORSE_SESSION_MSG_BYTES]);

/* 32-bit tag over dir || FN (no last-frame bit) || 12-byte payload. */
bool horse_crypto_voice_auth_tag(const uint8_t k_tag[32],
                                 uint8_t dir,
                                 uint16_t frame_num,
                                 const uint8_t *payload12,
                                 uint8_t tag_out[4]);

bool horse_crypto_voice_auth_verify(const uint8_t k_tag[32],
                                    uint8_t dir,
                                    uint16_t frame_num,
                                    const uint8_t *payload12,
                                    const uint8_t tag[4]);

/* Sign data with Ed25519 (without encryption).
 * Input:
 *  - ed25519_secretkey[64]: signing key from horse_identity_keys_t
 *  - message: data to sign
 *  - message_len: length of message
 * Output:
 *  - signature_out[64]: Ed25519 signature
 * Returns true on success, false on failure.
 */
bool horse_crypto_sign(
    const uint8_t *ed25519_secretkey,
    const uint8_t *message,
    size_t message_len,
    uint8_t *signature_out);

/* Verify Ed25519 signature (without decryption).
 * Input:
 *  - ed25519_publickey[32]: verification key from horse_identity_keys_t
 *  - message: signed data
 *  - message_len: length of message
 *  - signature[64]: Ed25519 signature to verify
 * Returns true if signature is valid, false otherwise.
 */
bool horse_crypto_verify(
    const uint8_t *ed25519_publickey,
    const uint8_t *message,
    size_t message_len,
    const uint8_t *signature);

void horse_crypto_memzero(void *buf, size_t len);

/*
 * TX is allowed only when libsodium is present, the keystore is unlocked,
 * and the peer keys required by the selected mode exist. Both flags clear
 * still means encrypt (legacy default) and therefore still requires crypto.
 */
bool horse_tx_allowed(bool encrypt_en, bool sign_en, bool crypto_available,
                      bool keystore_unlocked, bool have_x25519_peer,
                      bool have_ed25519_peer);

/*
 * Voice audio requires a valid ECDH session (k_tag) in every mode.
 * Signed LSF also requires a verified session signature.
 * Channel encrypt_en/sign_en must match the LSF flags (legacy both-clear
 * still means encrypt).
 */
bool horse_rx_may_output_voice(bool lsf_encrypted, bool session_valid,
                               bool lsf_signed, bool signature_ready,
                               bool ch_encrypt, bool ch_sign);

#ifdef __cplusplus
}
#endif

#endif

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Horse crypto API.
 *
 * Current implementation uses libsodium where available:
 *  - XChaCha20 stream cipher + BLAKE2b-based 32-bit MAC for voice frames
 *  - X25519 ECDH + XChaCha20-Poly1305 for session key wrapping (ECIES-style)
 *  - Ed25519 for digital signatures (without encryption)
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

/* Horse identity keys (libsodium-native sizes). */
#define HORSE_ED25519_PUBLICKEY_BYTES  32
#define HORSE_ED25519_SECRETKEY_BYTES  64
#define HORSE_X25519_PUBLICKEY_BYTES   32
#define HORSE_X25519_SECRETKEY_BYTES   32

/* AEAD used for session key wrapping (XChaCha20-Poly1305). */
#define HORSE_AEAD_XCHACHA20_NONCE_BYTES 24
#define HORSE_AEAD_TAG_BYTES             16

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

/* Session key wrap (ECIES-like):
 * - X25519 ECDH (ephemeral_sk * recipient_pk)
 * - Derive AEAD key via BLAKE2b generichash
 * - Encrypt session key with XChaCha20-Poly1305 (detached tag)
 *
 * Out:
 *  - ephemeral_pubkey_out[32]
 *  - ciphertext_out[32]
 *  - tag_out[16]
 */
bool horse_crypto_ecies_encrypt_session_key(
    const uint8_t *recipient_x25519_pubkey,
    const uint8_t *session_key,
    uint8_t *ephemeral_pubkey_out,
    uint8_t *ciphertext_out,
    uint8_t *tag_out);

/* Session key unwrap, see horse_crypto_ecies_encrypt_session_key().
 * Input:
 *  - recipient_x25519_seckey[32]
 */
bool horse_crypto_ecies_decrypt_session_key(
    const uint8_t *ephemeral_pubkey,
    const uint8_t *ciphertext,
    const uint8_t *tag,
    const uint8_t *recipient_x25519_seckey,
    uint8_t *session_key_out);

/* Voice frame encrypt: XChaCha20 stream cipher + 32-bit BLAKE2b MAC. */
void horse_crypto_voice_encrypt(
    const uint8_t *session_key,
    const uint8_t *nonce_96bit,
    const uint8_t *plaintext,
    size_t plaintext_len,
    uint8_t *ciphertext_out,
    uint8_t *tag_truncated_32bit);

/* Voice frame decrypt with MAC verification. */
bool horse_crypto_voice_decrypt(
    const uint8_t *session_key,
    const uint8_t *nonce_96bit,
    const uint8_t *ciphertext,
    size_t ciphertext_len,
    const uint8_t *tag_truncated_32bit,
    uint8_t *plaintext_out);

/* Derive key from passphrase (Argon2id via libsodium). Fails closed without it. */
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

/* Derive a 32-byte session key from X25519 ECDH + BLAKE2b. */
bool horse_crypto_derive_session_key(const uint8_t *local_x25519_sk,
                                     const uint8_t *remote_x25519_pk,
                                     uint8_t *session_key_out);

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

/* Derive per-call frame auth key from a verified session Ed25519 signature. */
bool horse_crypto_derive_frame_auth_key(const uint8_t session_signature[64],
                                        uint8_t auth_key_out[32]);

/* Build the 44-byte session message signed at the start of each transmission. */
void horse_crypto_build_session_message(const uint8_t src[6],
                                        const uint8_t dst[6],
                                        const uint8_t eph_pk[32],
                                        uint8_t message_out[44]);

/* Compute or verify a 32-bit voice authentication tag (cleartext voice). */
bool horse_crypto_voice_auth_tag(const uint8_t auth_key[32],
                                 uint16_t frame_num,
                                 const uint8_t *melpe96bits,
                                 uint8_t tag_out[4]);

bool horse_crypto_voice_auth_verify(const uint8_t auth_key[32],
                                    uint16_t frame_num,
                                    const uint8_t *melpe96bits,
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

/* Voice audio is released only with a valid encrypt session and/or verified
 * signature when the corresponding LSF flags are set.
 */
bool horse_rx_may_output_voice(bool lsf_encrypted, bool session_valid,
                               bool lsf_signed, bool signature_ready);

#ifdef __cplusplus
}
#endif

#endif

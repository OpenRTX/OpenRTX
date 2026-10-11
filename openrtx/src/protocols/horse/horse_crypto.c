/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Horse crypto implementation.
 *
 * When libsodium is available (HAVE_LIBSODIUM), this file provides:
 *  - XChaCha20 stream cipher + BLAKE2b-based 32-bit MAC for voice frames
 *  - Argon2id via crypto_pwhash for passphrase-based key derivation
 *  - X25519 ECDH with an LSF-bound KDF for session keys
 */

#include "protocols/horse/horse_crypto.h"
#include <string.h>

#ifdef HAVE_LIBSODIUM
#include <sodium.h>
#include "protocols/horse/horse_randombytes.h"

static int horse_sodium_init(void)
{
    static int initialized = 0;

    if (initialized)
        return 0;

    if (horse_randombytes_install() != 0)
        return -1;
    if (sodium_init() < 0)
        return -1;

    initialized = 1;
    return 0;
}
#endif

static bool horse_crypto_tag_dir_fn_payload(const uint8_t *k_tag, uint8_t dir,
                                            uint16_t frame_num,
                                            const uint8_t *payload,
                                            size_t payload_len,
                                            uint8_t tag_out[4])
{
#ifdef HAVE_LIBSODIUM
    static uint8_t msg[3 + 12];
    static uint8_t mac[64];
    uint16_t fn = frame_num & 0x7FFF;

    if (k_tag == NULL || payload == NULL || tag_out == NULL
        || payload_len != 12)
        return false;
    if (horse_sodium_init() != 0)
        return false;

    msg[0] = dir;
    msg[1] = (uint8_t)((fn >> 8) & 0xFF);
    msg[2] = (uint8_t)(fn & 0xFF);
    memcpy(msg + 3, payload, 12);
    crypto_generichash(mac, sizeof mac, msg, sizeof msg, k_tag,
                       HORSE_SESSION_KEY_BYTES);
    memcpy(tag_out, mac, HORSE_VOICE_TAG_BYTES);
    return true;
#else
    (void)k_tag;
    (void)dir;
    (void)frame_num;
    (void)payload;
    (void)payload_len;
    (void)tag_out;
    return false;
#endif
}

void horse_crypto_voice_encrypt(const uint8_t *k_enc, const uint8_t *k_tag,
                                uint8_t dir, uint16_t frame_num,
                                const uint8_t *nonce_96bit,
                                const uint8_t *plaintext, size_t plaintext_len,
                                uint8_t *ciphertext_out,
                                uint8_t *tag_truncated_32bit)
#ifdef HAVE_LIBSODIUM
{
    if (k_enc == NULL || plaintext == NULL || ciphertext_out == NULL)
        return;

    if (horse_sodium_init() != 0)
        return;

    static uint8_t nonce[24];
    crypto_generichash(nonce, sizeof nonce, nonce_96bit, 12,
                       (const unsigned char *)"HORSEV1", 7);

    crypto_stream_xchacha20_xor(ciphertext_out, plaintext, plaintext_len, nonce,
                                k_enc);

    if (tag_truncated_32bit)
        horse_crypto_tag_dir_fn_payload(k_tag, dir, frame_num, ciphertext_out,
                                        plaintext_len, tag_truncated_32bit);
#else
{
    (void)k_enc;
    (void)k_tag;
    (void)dir;
    (void)frame_num;
    (void)nonce_96bit;
    (void)plaintext_len;
    (void)ciphertext_out;
    (void)tag_truncated_32bit;
#endif
}

bool horse_crypto_voice_decrypt(const uint8_t *k_enc, const uint8_t *k_tag,
                                uint8_t dir, uint16_t frame_num,
                                const uint8_t *nonce_96bit,
                                const uint8_t *ciphertext,
                                size_t ciphertext_len,
                                const uint8_t *tag_truncated_32bit,
                                uint8_t *plaintext_out)
{
#ifdef HAVE_LIBSODIUM
    static uint8_t expected[HORSE_VOICE_TAG_BYTES];
    static uint8_t nonce[24];

    if (k_enc == NULL || k_tag == NULL || ciphertext == NULL
        || plaintext_out == NULL || tag_truncated_32bit == NULL)
        return false;

    if (horse_sodium_init() != 0)
        return false;

    if (!horse_crypto_tag_dir_fn_payload(k_tag, dir, frame_num, ciphertext,
                                         ciphertext_len, expected))
        return false;

    if (sodium_memcmp(expected, tag_truncated_32bit, HORSE_VOICE_TAG_BYTES)
        != 0)
        return false;

    crypto_generichash(nonce, sizeof nonce, nonce_96bit, 12,
                       (const unsigned char *)"HORSEV1", 7);

    crypto_stream_xchacha20_xor(plaintext_out, ciphertext, ciphertext_len,
                                nonce, k_enc);
    return true;
#else
    (void)k_enc;
    (void)k_tag;
    (void)dir;
    (void)frame_num;
    (void)nonce_96bit;
    (void)ciphertext;
    (void)ciphertext_len;
    (void)tag_truncated_32bit;
    (void)plaintext_out;
    return false;
#endif
}

bool horse_crypto_argon2id_derive(const char *passphrase, size_t passphrase_len,
                                  const uint8_t *salt, size_t salt_len,
                                  uint8_t *key_out, size_t key_len)
{
    if (passphrase == NULL || salt == NULL || key_out == NULL || key_len == 0)
        return false;

#ifdef HAVE_LIBSODIUM
    if (horse_sodium_init() != 0)
        return false;

    /* Use libsodium's Argon2id implementation via crypto_pwhash.
     * Note: crypto_pwhash expects salt to be crypto_pwhash_SALTBYTES (16 bytes);
     * salt_len parameter is ignored in this path.
     */
    (void)salt_len;
    if (crypto_pwhash(key_out, key_len, passphrase, passphrase_len, salt,
                      HORSE_ARGON2ID_OPSLIMIT, HORSE_ARGON2ID_MEMLIMIT,
                      crypto_pwhash_ALG_ARGON2ID13)
        != 0) {
        return false;
    }
    return true;
#else
    (void)passphrase_len;
    (void)salt_len;
    return false;
#endif
}

bool horse_crypto_available(void)
{
#ifdef HAVE_LIBSODIUM
    return horse_sodium_init() == 0 && !horse_randombytes_failed();
#else
    return false;
#endif
}

bool horse_crypto_x25519_keypair(uint8_t *pk_out, uint8_t *sk_out)
{
    if (pk_out == NULL || sk_out == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    if (horse_sodium_init() != 0) {
        horse_crypto_memzero(pk_out, 32);
        horse_crypto_memzero(sk_out, 32);
        return false;
    }

    if (crypto_kx_keypair(pk_out, sk_out) != 0 || horse_randombytes_failed()) {
        horse_crypto_memzero(pk_out, 32);
        horse_crypto_memzero(sk_out, 32);
        return false;
    }
    return true;
#else
    (void)pk_out;
    (void)sk_out;
    return false;
#endif
}

bool horse_crypto_derive_session_keys(
    const uint8_t *local_x25519_sk, const uint8_t *remote_x25519_pk,
    const uint8_t src[6], const uint8_t dst[6], const uint8_t eph_pk[32],
    uint8_t flags, uint8_t version, uint8_t k_enc_out[HORSE_SESSION_KEY_BYTES],
    uint8_t k_tag_out[HORSE_SESSION_KEY_BYTES])
{
    if (local_x25519_sk == NULL || remote_x25519_pk == NULL || src == NULL
        || dst == NULL || eph_pk == NULL || k_enc_out == NULL
        || k_tag_out == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    static unsigned char shared[crypto_scalarmult_BYTES];
    static unsigned char ikm[crypto_scalarmult_BYTES + 6 + 6 + 32 + 2];

    if (horse_sodium_init() != 0)
        return false;

    if (crypto_scalarmult(shared, local_x25519_sk, remote_x25519_pk) != 0)
        return false;

    memcpy(ikm, shared, sizeof shared);
    memcpy(ikm + sizeof shared, src, 6);
    memcpy(ikm + sizeof shared + 6, dst, 6);
    memcpy(ikm + sizeof shared + 12, eph_pk, 32);
    ikm[sizeof shared + 44] = flags;
    ikm[sizeof shared + 45] = version;

    crypto_generichash(k_enc_out, HORSE_SESSION_KEY_BYTES, ikm, sizeof ikm,
                       (const unsigned char *)"HORSE-KENC", 10);
    crypto_generichash(k_tag_out, HORSE_SESSION_KEY_BYTES, ikm, sizeof ikm,
                       (const unsigned char *)"HORSE-KTAG", 10);
    sodium_memzero(shared, sizeof shared);
    sodium_memzero(ikm, sizeof ikm);
    return true;
#else
    (void)local_x25519_sk;
    (void)remote_x25519_pk;
    (void)src;
    (void)dst;
    (void)eph_pk;
    (void)flags;
    (void)version;
    return false;
#endif
}

bool horse_crypto_lsf_version_ok(uint8_t version)
{
    return version == HORSE_LSF_VERSION;
}

void horse_crypto_voice_nonce_from_fn(uint16_t frame_num,
                                      uint8_t nonce_96bit[12])
{
    if (nonce_96bit == NULL)
        return;

    memset(nonce_96bit, 0, 12);
    nonce_96bit[10] = (uint8_t)((frame_num >> 8) & 0xFF);
    nonce_96bit[11] = (uint8_t)(frame_num & 0xFF);
}

bool horse_crypto_encrypt_identity(const horse_identity_keys_t *identity,
                                   const uint8_t *wrap_key, size_t wrap_key_len,
                                   uint8_t *blob_out, size_t blob_cap,
                                   size_t *blob_len)
{
    if (identity == NULL || wrap_key == NULL || blob_out == NULL
        || blob_len == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    const size_t need = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES
                      + crypto_aead_xchacha20poly1305_ietf_ABYTES
                      + sizeof(horse_identity_keys_t);

    if (blob_cap < need)
        return false;

    (void)wrap_key_len;

    if (horse_sodium_init() != 0)
        return false;

    unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
    randombytes_buf(nonce, sizeof nonce);

    unsigned long long clen = 0;
    if (crypto_aead_xchacha20poly1305_ietf_encrypt(
            blob_out + sizeof nonce, &clen, (const unsigned char *)identity,
            sizeof(horse_identity_keys_t), NULL, 0, NULL, nonce, wrap_key)
        != 0)
        return false;

    memcpy(blob_out, nonce, sizeof nonce);
    *blob_len = sizeof nonce + (size_t)clen;
    return true;
#else
    (void)wrap_key_len;
    (void)blob_out;
    (void)blob_cap;
    return false;
#endif
}

bool horse_crypto_decrypt_identity(const uint8_t *blob, size_t blob_len,
                                   const uint8_t *wrap_key, size_t wrap_key_len,
                                   horse_identity_keys_t *identity_out)
{
    if (blob == NULL || wrap_key == NULL || identity_out == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    if (blob_len < crypto_aead_xchacha20poly1305_ietf_NPUBBYTES
                       + crypto_aead_xchacha20poly1305_ietf_ABYTES)
        return false;

    (void)wrap_key_len;

    if (horse_sodium_init() != 0)
        return false;

    const unsigned char *nonce = blob;
    const unsigned char *cipher = blob
                                + crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
    size_t cipher_len = blob_len - crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
    unsigned long long mlen = 0;

    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            (unsigned char *)identity_out, &mlen, NULL, cipher, cipher_len,
            NULL, 0, nonce, wrap_key)
        != 0)
        return false;

    return mlen == sizeof(horse_identity_keys_t);
#else
    (void)blob_len;
    (void)wrap_key_len;
    (void)identity_out;
    return false;
#endif
}

bool horse_crypto_identity_fingerprint(const horse_identity_keys_t *identity,
                                       uint8_t fp_out[32])
{
    if (identity == NULL || fp_out == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    if (horse_sodium_init() != 0)
        return false;

    crypto_generichash(fp_out, 32, (const unsigned char *)identity,
                       sizeof(horse_identity_keys_t),
                       (const unsigned char *)"HORSE-IDFP", 10);
    return true;
#else
    (void)identity;
    (void)fp_out;
    return false;
#endif
}

void horse_crypto_build_session_message(
    const uint8_t src[6], const uint8_t dst[6], const uint8_t eph_pk[32],
    uint8_t flags, uint8_t version,
    uint8_t message_out[HORSE_SESSION_MSG_BYTES])
{
    if (message_out == NULL)
        return;

    memset(message_out, 0, HORSE_SESSION_MSG_BYTES);
    if (src != NULL)
        memcpy(message_out, src, 6);
    if (dst != NULL)
        memcpy(message_out + 6, dst, 6);
    if (eph_pk != NULL)
        memcpy(message_out + 12, eph_pk, 32);
    message_out[44] = flags;
    message_out[45] = version;
}

bool horse_crypto_voice_auth_tag(const uint8_t k_tag[32], uint8_t dir,
                                 uint16_t frame_num, const uint8_t *payload12,
                                 uint8_t tag_out[4])
{
    return horse_crypto_tag_dir_fn_payload(k_tag, dir, frame_num, payload12, 12,
                                           tag_out);
}

bool horse_crypto_voice_auth_verify(const uint8_t k_tag[32], uint8_t dir,
                                    uint16_t frame_num,
                                    const uint8_t *payload12,
                                    const uint8_t tag[4])
{
    uint8_t expected[HORSE_VOICE_TAG_BYTES];

    if (tag == NULL)
        return false;
    if (!horse_crypto_tag_dir_fn_payload(k_tag, dir, frame_num, payload12, 12,
                                         expected))
        return false;
#ifdef HAVE_LIBSODIUM
    return sodium_memcmp(expected, tag, HORSE_VOICE_TAG_BYTES) == 0;
#else
    (void)expected;
    return false;
#endif
}

bool horse_crypto_sign(const uint8_t *ed25519_secretkey, const uint8_t *message,
                       size_t message_len, uint8_t *signature_out)
{
    if (ed25519_secretkey == NULL || message == NULL || signature_out == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    if (horse_sodium_init() != 0)
        return false;

    if (crypto_sign_detached(signature_out, NULL, message, message_len,
                             ed25519_secretkey)
        != 0) {
        return false;
    }
    if (horse_randombytes_failed()) {
        horse_crypto_memzero(signature_out, 64);
        return false;
    }
    return true;
#else
    (void)message_len;
    (void)signature_out;
    return false;
#endif
}

bool horse_crypto_verify(const uint8_t *ed25519_publickey,
                         const uint8_t *message, size_t message_len,
                         const uint8_t *signature)
{
    if (ed25519_publickey == NULL || message == NULL || signature == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    if (horse_sodium_init() != 0)
        return false;

    if (crypto_sign_verify_detached(signature, message, message_len,
                                    ed25519_publickey)
        != 0) {
        return false;
    }
    return true;
#else
    (void)message_len;
    (void)signature;
    return false;
#endif
}

void horse_crypto_memzero(void *buf, size_t len)
{
    if (buf == NULL || len == 0)
        return;
#ifdef HAVE_LIBSODIUM
    sodium_memzero(buf, len);
#else
    volatile uint8_t *p = (volatile uint8_t *)buf;
    while (len--)
        *p++ = 0;
#endif
}

bool horse_tx_allowed(bool encrypt_en, bool sign_en, bool crypto_available,
                      bool keystore_unlocked, bool have_x25519_peer,
                      bool have_ed25519_peer)
{
    bool want_encrypt = encrypt_en;
    bool want_sign = sign_en;

    if (!want_encrypt && !want_sign)
        want_encrypt = true;

    if (!crypto_available || !keystore_unlocked)
        return false;
    if (!have_x25519_peer)
        return false;
    if (want_sign && !have_ed25519_peer)
        return false;
    return true;
}

bool horse_rx_may_output_voice(bool lsf_encrypted, bool session_valid,
                               bool lsf_signed, bool signature_ready,
                               bool ch_encrypt, bool ch_sign)
{
    if (!ch_encrypt && !ch_sign)
        ch_encrypt = true;
    if (ch_encrypt != lsf_encrypted)
        return false;
    if (ch_sign != lsf_signed)
        return false;
    if (!session_valid)
        return false;
    if (lsf_signed && !signature_ready)
        return false;
    return true;
}

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
 *
 * ECIES-style session key operations remain stubs until a concrete curve and
 * key storage format are finalized in this tree.
 */

#include "protocols/horse/horse_crypto.h"
#include <string.h>

#ifdef HAVE_LIBSODIUM
#include <sodium.h>

static int horse_sodium_init(void)
{
    static int initialized = 0;

    if (initialized)
        return 0;

    if (sodium_init() < 0)
        return -1;

    initialized = 1;
    return 0;
}
#else
#include "core/crypto_utils.h"
#endif

bool horse_crypto_ecies_encrypt_session_key(
    const uint8_t *recipient_x25519_pubkey,
    const uint8_t *session_key,
    uint8_t *ephemeral_pubkey_out,
    uint8_t *ciphertext_out,
    uint8_t *tag_out)
{
    if (recipient_x25519_pubkey == NULL || session_key == NULL ||
        ephemeral_pubkey_out == NULL || ciphertext_out == NULL || tag_out == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    if (horse_sodium_init() != 0)
        return false;

    unsigned char eph_pk[HORSE_X25519_PUBLICKEY_BYTES];
    unsigned char eph_sk[HORSE_X25519_SECRETKEY_BYTES];
    crypto_kx_keypair(eph_pk, eph_sk);
    memcpy(ephemeral_pubkey_out, eph_pk, sizeof eph_pk);

    /* ECDH shared secret. */
    unsigned char shared[crypto_scalarmult_BYTES];
    if (crypto_scalarmult(shared, eph_sk, recipient_x25519_pubkey) != 0)
    {
        sodium_memzero(eph_sk, sizeof eph_sk);
        return false;
    }

    /* Derive AEAD key and nonce deterministically from shared secret + context. */
    unsigned char aead_key[crypto_aead_xchacha20poly1305_ietf_KEYBYTES];
    unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];

    crypto_generichash(aead_key, sizeof aead_key,
                       shared, sizeof shared,
                       (const unsigned char *)"HORSE-ECIES-KEY", 14);
    crypto_generichash(nonce, sizeof nonce,
                       shared, sizeof shared,
                       (const unsigned char *)"HORSE-ECIES-NONCE", 16);

    unsigned long long clen = 0;
    if (crypto_aead_xchacha20poly1305_ietf_encrypt_detached(
            ciphertext_out,
            tag_out,
            &clen,
            session_key,
            (unsigned long long)HORSE_SESSION_KEY_BYTES,
            eph_pk,
            (unsigned long long)sizeof eph_pk,
            NULL, /* nsec */
            nonce,
            aead_key) != 0)
    {
        sodium_memzero(eph_sk, sizeof eph_sk);
        sodium_memzero(shared, sizeof shared);
        return false;
    }

    sodium_memzero(eph_sk, sizeof eph_sk);
    sodium_memzero(shared, sizeof shared);
    return clen == HORSE_SESSION_KEY_BYTES;
#else
    (void)recipient_x25519_pubkey;
    (void)ephemeral_pubkey_out;
    (void)ciphertext_out;
    (void)tag_out;
    return false;
#endif
}

bool horse_crypto_ecies_decrypt_session_key(
    const uint8_t *ephemeral_pubkey,
    const uint8_t *ciphertext,
    const uint8_t *tag,
    const uint8_t *recipient_x25519_seckey,
    uint8_t *session_key_out)
{
    if (ephemeral_pubkey == NULL || ciphertext == NULL || tag == NULL ||
        recipient_x25519_seckey == NULL || session_key_out == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    if (horse_sodium_init() != 0)
        return false;

    unsigned char shared[crypto_scalarmult_BYTES];
    if (crypto_scalarmult(shared, recipient_x25519_seckey, ephemeral_pubkey) != 0)
        return false;

    unsigned char aead_key[crypto_aead_xchacha20poly1305_ietf_KEYBYTES];
    unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];

    crypto_generichash(aead_key, sizeof aead_key,
                       shared, sizeof shared,
                       (const unsigned char *)"HORSE-ECIES-KEY", 14);
    crypto_generichash(nonce, sizeof nonce,
                       shared, sizeof shared,
                       (const unsigned char *)"HORSE-ECIES-NONCE", 16);

    if (crypto_aead_xchacha20poly1305_ietf_decrypt_detached(
            session_key_out,
            NULL, /* nsec */
            ciphertext,
            (unsigned long long)HORSE_SESSION_KEY_BYTES,
            tag,
            ephemeral_pubkey,
            (unsigned long long)HORSE_X25519_PUBLICKEY_BYTES,
            nonce,
            aead_key) != 0)
    {
        sodium_memzero(shared, sizeof shared);
        return false;
    }
    sodium_memzero(shared, sizeof shared);
    return true;
#else
    (void)ephemeral_pubkey;
    (void)ciphertext;
    (void)tag;
    (void)recipient_x25519_seckey;
    (void)session_key_out;
    return false;
#endif
}

void horse_crypto_voice_encrypt(
    const uint8_t *session_key,
    const uint8_t *nonce_96bit,
    const uint8_t *plaintext,
    size_t plaintext_len,
    uint8_t *ciphertext_out,
    uint8_t *tag_truncated_32bit)
#ifdef HAVE_LIBSODIUM
{
    if (plaintext == NULL || ciphertext_out == NULL)
        return;

    if (horse_sodium_init() != 0)
        return;

    /* Derive a 192-bit XChaCha20 nonce from the 96-bit input using BLAKE2b. */
    uint8_t nonce[crypto_stream_xchacha20_NONCEBYTES];
    crypto_generichash(nonce, sizeof nonce,
                       nonce_96bit, 12,
                       (const unsigned char *)"HORSEV1", 7);

    /* XChaCha20 stream cipher encryption. */
    crypto_stream_xchacha20_xor(ciphertext_out,
                                plaintext,
                                plaintext_len,
                                nonce,
                                session_key);

    if (tag_truncated_32bit)
    {
        /* Compute a BLAKE2b MAC over the ciphertext and truncate to 32 bits. */
        uint8_t mac[crypto_generichash_BYTES];
        crypto_generichash(mac, sizeof mac,
                           ciphertext_out, plaintext_len,
                           (const unsigned char *)"HVOICETAG", 9);
        memcpy(tag_truncated_32bit, mac, HORSE_VOICE_TAG_BYTES);
    }
#else
{
    (void)session_key;
    (void)nonce_96bit;
    (void)plaintext_len;
    (void)ciphertext_out;
    (void)tag_truncated_32bit;
#endif
}

bool horse_crypto_voice_decrypt(
    const uint8_t *session_key,
    const uint8_t *nonce_96bit,
    const uint8_t *ciphertext,
    size_t ciphertext_len,
    const uint8_t *tag_truncated_32bit,
    uint8_t *plaintext_out)
{
#ifdef HAVE_LIBSODIUM
    if (ciphertext == NULL || plaintext_out == NULL)
        return false;

    if (horse_sodium_init() != 0)
        return false;

    /* Verify 32-bit BLAKE2b MAC over ciphertext, if provided. */
    if (tag_truncated_32bit != NULL)
    {
        uint8_t mac[crypto_generichash_BYTES];
        crypto_generichash(mac, sizeof mac,
                           ciphertext, ciphertext_len,
                           (const unsigned char *)"HVOICETAG", 9);

        if (sodium_memcmp(mac, tag_truncated_32bit,
                          HORSE_VOICE_TAG_BYTES) != 0)
        {
            /* Authentication failed. Do not decrypt into output buffer. */
            return false;
        }
    }

    uint8_t nonce[crypto_stream_xchacha20_NONCEBYTES];
    crypto_generichash(nonce, sizeof nonce,
                       nonce_96bit, 12,
                       (const unsigned char *)"HORSEV1", 7);

    crypto_stream_xchacha20_xor(plaintext_out,
                                ciphertext,
                                ciphertext_len,
                                nonce,
                                session_key);
    return true;
#else
    (void)ciphertext_len;
    (void)plaintext_out;
    return false;
#endif
}

bool horse_crypto_argon2id_derive(
    const char *passphrase,
    size_t passphrase_len,
    const uint8_t *salt,
    size_t salt_len,
    uint8_t *key_out,
    size_t key_len)
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
    if (crypto_pwhash(key_out, key_len,
                      passphrase, passphrase_len,
                      salt,
                      crypto_pwhash_OPSLIMIT_MODERATE,
                      crypto_pwhash_MEMLIMIT_MODERATE,
                      crypto_pwhash_ALG_ARGON2ID13) != 0)
    {
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
    return horse_sodium_init() == 0;
#else
    return false;
#endif
}

bool horse_crypto_x25519_keypair(uint8_t *pk_out, uint8_t *sk_out)
{
    if (pk_out == NULL || sk_out == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    if (horse_sodium_init() != 0)
        return false;

    if (crypto_kx_keypair(pk_out, sk_out) != 0)
        return false;
    return true;
#else
    (void)pk_out;
    (void)sk_out;
    return false;
#endif
}

bool horse_crypto_derive_session_key(const uint8_t *local_x25519_sk,
                                     const uint8_t *remote_x25519_pk,
                                     uint8_t *session_key_out)
{
    if (local_x25519_sk == NULL || remote_x25519_pk == NULL ||
        session_key_out == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    unsigned char shared[crypto_scalarmult_BYTES];

    if (horse_sodium_init() != 0)
        return false;

    if (crypto_scalarmult(shared, local_x25519_sk, remote_x25519_pk) != 0)
        return false;

    crypto_generichash(session_key_out, HORSE_SESSION_KEY_BYTES,
                       shared, sizeof shared,
                       (const unsigned char *)"HORSE-SESSION", 13);
    sodium_memzero(shared, sizeof shared);
    return true;
#else
    (void)local_x25519_sk;
    (void)remote_x25519_pk;
    (void)session_key_out;
    return false;
#endif
}

void horse_crypto_voice_nonce_from_fn(uint16_t frame_num, uint8_t nonce_96bit[12])
{
    if (nonce_96bit == NULL)
        return;

    memset(nonce_96bit, 0, 12);
    nonce_96bit[10] = (uint8_t)((frame_num >> 8) & 0xFF);
    nonce_96bit[11] = (uint8_t)(frame_num & 0xFF);
}

bool horse_crypto_encrypt_identity(const horse_identity_keys_t *identity,
                                   const uint8_t *wrap_key,
                                   size_t wrap_key_len,
                                   uint8_t *blob_out,
                                   size_t blob_cap,
                                   size_t *blob_len)
{
    if (identity == NULL || wrap_key == NULL || blob_out == NULL ||
        blob_len == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    const size_t need = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES +
                        crypto_aead_xchacha20poly1305_ietf_ABYTES +
                        sizeof(horse_identity_keys_t);

    if (blob_cap < need)
        return false;

    (void)wrap_key_len;

    if (horse_sodium_init() != 0)
        return false;

    unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
    randombytes_buf(nonce, sizeof nonce);

    unsigned long long clen = 0;
    if (crypto_aead_xchacha20poly1305_ietf_encrypt(
            blob_out + sizeof nonce,
            &clen,
            (const unsigned char *)identity,
            sizeof(horse_identity_keys_t),
            NULL,
            0,
            NULL,
            nonce,
            wrap_key) != 0)
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

bool horse_crypto_decrypt_identity(const uint8_t *blob,
                                   size_t blob_len,
                                   const uint8_t *wrap_key,
                                   size_t wrap_key_len,
                                   horse_identity_keys_t *identity_out)
{
    if (blob == NULL || wrap_key == NULL || identity_out == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    if (blob_len < crypto_aead_xchacha20poly1305_ietf_NPUBBYTES +
                     crypto_aead_xchacha20poly1305_ietf_ABYTES)
        return false;

    (void)wrap_key_len;

    if (horse_sodium_init() != 0)
        return false;

    const unsigned char *nonce = blob;
    const unsigned char *cipher = blob + crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
    size_t cipher_len = blob_len - crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
    unsigned long long mlen = 0;

    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            (unsigned char *)identity_out,
            &mlen,
            NULL,
            cipher,
            cipher_len,
            NULL,
            0,
            nonce,
            wrap_key) != 0)
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

    crypto_generichash(fp_out, 32,
                       (const unsigned char *)identity,
                       sizeof(horse_identity_keys_t),
                       (const unsigned char *)"HORSE-IDFP", 10);
    return true;
#else
    (void)identity;
    (void)fp_out;
    return false;
#endif
}

void horse_crypto_build_session_message(const uint8_t src[6],
                                        const uint8_t dst[6],
                                        const uint8_t eph_pk[32],
                                        uint8_t message_out[44])
{
    if (message_out == NULL)
        return;

    memset(message_out, 0, 44);
    if (src != NULL)
        memcpy(message_out, src, 6);
    if (dst != NULL)
        memcpy(message_out + 6, dst, 6);
    if (eph_pk != NULL)
        memcpy(message_out + 12, eph_pk, 32);
}

bool horse_crypto_derive_frame_auth_key(const uint8_t session_signature[64],
                                        uint8_t auth_key_out[32])
{
    if (session_signature == NULL || auth_key_out == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    if (horse_sodium_init() != 0)
        return false;

    crypto_generichash(auth_key_out, 32,
                       session_signature, 64,
                       (const unsigned char *)"HORSE-FAUTH", 11);
    return true;
#else
    (void)session_signature;
    (void)auth_key_out;
    return false;
#endif
}

static void horse_crypto_voice_auth_message(uint16_t frame_num,
                                            const uint8_t *melpe96bits,
                                            uint8_t message_out[14])
{
    message_out[0] = (uint8_t)((frame_num >> 8) & 0xFF);
    message_out[1] = (uint8_t)(frame_num & 0xFF);
    if (melpe96bits != NULL)
        memcpy(message_out + 2, melpe96bits, 12);
    else
        memset(message_out + 2, 0, 12);
}

bool horse_crypto_voice_auth_tag(const uint8_t auth_key[32],
                                 uint16_t frame_num,
                                 const uint8_t *melpe96bits,
                                 uint8_t tag_out[4])
{
    if (auth_key == NULL || tag_out == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    uint8_t message[14];
    uint8_t mac[crypto_generichash_BYTES];

    if (horse_sodium_init() != 0)
        return false;

    horse_crypto_voice_auth_message(frame_num, melpe96bits, message);
    crypto_generichash(mac, sizeof mac, message, sizeof message,
                       auth_key, 32);
    memcpy(tag_out, mac, HORSE_VOICE_TAG_BYTES);
    return true;
#else
    (void)frame_num;
    (void)melpe96bits;
    (void)tag_out;
    return false;
#endif
}

bool horse_crypto_voice_auth_verify(const uint8_t auth_key[32],
                                    uint16_t frame_num,
                                    const uint8_t *melpe96bits,
                                    const uint8_t tag[4])
{
    if (auth_key == NULL || melpe96bits == NULL || tag == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    uint8_t expected[HORSE_VOICE_TAG_BYTES];

    if (!horse_crypto_voice_auth_tag(auth_key, frame_num, melpe96bits, expected))
        return false;

    return sodium_memcmp(expected, tag, HORSE_VOICE_TAG_BYTES) == 0;
#else
    (void)frame_num;
    (void)melpe96bits;
    (void)tag;
    return false;
#endif
}

bool horse_crypto_sign(
    const uint8_t *ed25519_secretkey,
    const uint8_t *message,
    size_t message_len,
    uint8_t *signature_out)
{
    if (ed25519_secretkey == NULL || message == NULL || signature_out == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    if (horse_sodium_init() != 0)
        return false;

    if (crypto_sign_detached(signature_out, NULL,
                              message, message_len,
                              ed25519_secretkey) != 0)
    {
        return false;
    }
    return true;
#else
    (void)message_len;
    (void)signature_out;
    return false;
#endif
}

bool horse_crypto_verify(
    const uint8_t *ed25519_publickey,
    const uint8_t *message,
    size_t message_len,
    const uint8_t *signature)
{
    if (ed25519_publickey == NULL || message == NULL || signature == NULL)
        return false;

#ifdef HAVE_LIBSODIUM
    if (horse_sodium_init() != 0)
        return false;

    if (crypto_sign_verify_detached(signature,
                                     message, message_len,
                                     ed25519_publickey) != 0)
    {
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
    if (want_encrypt && !have_x25519_peer)
        return false;
    if (want_sign && !have_ed25519_peer)
        return false;
    return true;
}

bool horse_rx_may_output_voice(bool lsf_encrypted, bool session_valid,
                               bool lsf_signed, bool signature_ready)
{
    if (lsf_encrypted && !session_valid)
        return false;
    if (lsf_signed && !signature_ready)
        return false;
    return true;
}

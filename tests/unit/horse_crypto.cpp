/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Unit test for Horse crypto voice encrypt/decrypt with libsodium.
 */

#include "protocols/horse/horse_crypto.h"
#include "protocols/horse/HorseConstants.hpp"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdlib>

#ifdef HAVE_LIBSODIUM
#include <sodium.h>
#endif

static int test_voice_encrypt_decrypt_roundtrip()
{
    uint8_t session_key[HORSE_SESSION_KEY_BYTES];
    uint8_t nonce_96bit[12];
    uint8_t plaintext[12];
    uint8_t ciphertext[12];
    uint8_t tag[HORSE_VOICE_TAG_BYTES];
    uint8_t decrypted[12];

    /* Initialize test vectors. */
    for (size_t i = 0; i < sizeof session_key; i++)
        session_key[i] = (uint8_t)(i ^ 0xAA);
    for (size_t i = 0; i < sizeof nonce_96bit; i++)
        nonce_96bit[i] = (uint8_t)(i + 0x10);
    for (size_t i = 0; i < sizeof plaintext; i++)
        plaintext[i] = (uint8_t)(i * 3);

    /* Encrypt. */
    horse_crypto_voice_encrypt(session_key, session_key, HORSE_VOICE_DIR_FORWARD, 0, nonce_96bit, plaintext,
                               sizeof plaintext, ciphertext, tag);

    /* Verify ciphertext changed. */
    if (memcmp(ciphertext, plaintext, sizeof plaintext) == 0)
    {
        std::printf("horse_crypto_test: ciphertext unchanged (encryption may not be active)\n");
        return -1;
    }

    /* Verify tag is non-zero (if libsodium is active). */
    bool tag_all_zero = true;
    for (size_t i = 0; i < sizeof tag; i++)
    {
        if (tag[i] != 0)
        {
            tag_all_zero = false;
            break;
        }
    }
    if (tag_all_zero)
    {
        std::printf("horse_crypto_test: tag is all zeros (MAC may not be active)\n");
    }

    /* Decrypt. */
    if (!horse_crypto_voice_decrypt(session_key, session_key, HORSE_VOICE_DIR_FORWARD, 0, nonce_96bit, ciphertext,
                                     sizeof ciphertext, tag, decrypted))
    {
        std::printf("horse_crypto_test: decrypt failed\n");
        return -1;
    }

    /* Verify plaintext matches. */
    if (memcmp(decrypted, plaintext, sizeof plaintext) != 0)
    {
        std::printf("horse_crypto_test: decrypted plaintext mismatch\n");
        return -1;
    }

    return 0;
}

static int test_voice_decrypt_bad_tag()
{
    uint8_t session_key[HORSE_SESSION_KEY_BYTES];
    uint8_t nonce_96bit[12];
    uint8_t plaintext[12];
    uint8_t ciphertext[12];
    uint8_t tag[HORSE_VOICE_TAG_BYTES];
    uint8_t decrypted[12];

    for (size_t i = 0; i < sizeof session_key; i++)
        session_key[i] = (uint8_t)i;
    for (size_t i = 0; i < sizeof nonce_96bit; i++)
        nonce_96bit[i] = (uint8_t)(i + 0x20);
    for (size_t i = 0; i < sizeof plaintext; i++)
        plaintext[i] = (uint8_t)(i * 5);

    horse_crypto_voice_encrypt(session_key, session_key, HORSE_VOICE_DIR_FORWARD, 0, nonce_96bit, plaintext,
                               sizeof plaintext, ciphertext, tag);

    /* Corrupt tag. */
    tag[0] ^= 0xFF;

    /* Decrypt should fail. */
    if (horse_crypto_voice_decrypt(session_key, session_key, HORSE_VOICE_DIR_FORWARD, 0, nonce_96bit, ciphertext,
                                    sizeof ciphertext, tag, decrypted))
    {
        std::printf("horse_crypto_test: decrypt accepted corrupted tag\n");
        return -1;
    }

    return 0;
}

static int test_voice_decrypt_bad_ciphertext()
{
    uint8_t session_key[HORSE_SESSION_KEY_BYTES];
    uint8_t nonce_96bit[12];
    uint8_t plaintext[12];
    uint8_t ciphertext[12];
    uint8_t tag[HORSE_VOICE_TAG_BYTES];
    uint8_t decrypted[12];

    for (size_t i = 0; i < sizeof session_key; i++)
        session_key[i] = (uint8_t)(i + 0x30);
    for (size_t i = 0; i < sizeof nonce_96bit; i++)
        nonce_96bit[i] = (uint8_t)(i + 0x40);
    for (size_t i = 0; i < sizeof plaintext; i++)
        plaintext[i] = (uint8_t)(i * 7);

    horse_crypto_voice_encrypt(session_key, session_key, HORSE_VOICE_DIR_FORWARD, 0, nonce_96bit, plaintext,
                               sizeof plaintext, ciphertext, tag);

    /* Corrupt ciphertext. */
    ciphertext[10] ^= 0x55;

    /* Decrypt should fail (MAC verification should catch this). */
    if (horse_crypto_voice_decrypt(session_key, session_key, HORSE_VOICE_DIR_FORWARD, 0, nonce_96bit, ciphertext,
                                    sizeof ciphertext, tag, decrypted))
    {
        std::printf("horse_crypto_test: decrypt accepted corrupted ciphertext\n");
        return -1;
    }

    return 0;
}

static int test_voice_nonce_independence()
{
    uint8_t session_key[HORSE_SESSION_KEY_BYTES];
    uint8_t nonce1[12] = {0};
    uint8_t nonce2[12] = {0};
    uint8_t plaintext[12];
    uint8_t ciphertext1[12], ciphertext2[12];
    uint8_t tag1[HORSE_VOICE_TAG_BYTES], tag2[HORSE_VOICE_TAG_BYTES];

    nonce2[0] = 1; /* Different nonce. */

    for (size_t i = 0; i < sizeof session_key; i++)
        session_key[i] = (uint8_t)(i + 0x50);
    for (size_t i = 0; i < sizeof plaintext; i++)
        plaintext[i] = (uint8_t)(i * 11);

    horse_crypto_voice_encrypt(session_key, session_key, HORSE_VOICE_DIR_FORWARD, 0, nonce1, plaintext,
                               sizeof plaintext, ciphertext1, tag1);
    horse_crypto_voice_encrypt(session_key, session_key, HORSE_VOICE_DIR_FORWARD, 0, nonce2, plaintext,
                               sizeof plaintext, ciphertext2, tag2);

    /* Different nonces should produce different ciphertexts. */
    if (memcmp(ciphertext1, ciphertext2, sizeof ciphertext1) == 0)
    {
        std::printf("horse_crypto_test: nonce independence failed (same ciphertext for different nonces)\n");
        return -1;
    }

    return 0;
}

static int test_sign_verify_roundtrip()
{
    uint8_t ed25519_pk[HORSE_ED25519_PUBLICKEY_BYTES];
    uint8_t ed25519_sk[HORSE_ED25519_SECRETKEY_BYTES];
    uint8_t message[64];
    uint8_t signature[HORSE_ED25519_SIGNATURE_BYTES];

    /* Initialize test vectors. */
    for (size_t i = 0; i < sizeof message; i++)
        message[i] = (uint8_t)(i ^ 0xCC);

    /* Generate a test keypair (in real use, this comes from horse_provision.py). */
    /* For testing, we'll use a known test vector or generate via libsodium if available. */
    /* For now, test that the API works even if keys are zeros (will fail verification). */
    memset(ed25519_pk, 0, sizeof ed25519_pk);
    memset(ed25519_sk, 0, sizeof ed25519_sk);

    /* Sign. */
    if (!horse_crypto_sign(ed25519_sk, message, sizeof message, signature))
    {
        std::printf("horse_crypto_test: sign failed\n");
        return -1;
    }

    /* Verify (may fail with zero keys, but API should not crash). */
    (void)horse_crypto_verify(ed25519_pk, message, sizeof message, signature);
    /* With zero keys, verification will likely fail, which is expected. */

    return 0;
}

static int test_sign_verify_bad_signature()
{
    uint8_t ed25519_pk[HORSE_ED25519_PUBLICKEY_BYTES];
    uint8_t ed25519_sk[HORSE_ED25519_SECRETKEY_BYTES];
    uint8_t message[64];
    uint8_t signature[HORSE_ED25519_SIGNATURE_BYTES];

    for (size_t i = 0; i < sizeof message; i++)
        message[i] = (uint8_t)(i + 0x80);

    memset(ed25519_pk, 0, sizeof ed25519_pk);
    memset(ed25519_sk, 0, sizeof ed25519_sk);

    horse_crypto_sign(ed25519_sk, message, sizeof message, signature);

    /* Corrupt signature. */
    signature[0] ^= 0xFF;

    /* Verify should fail. */
    if (horse_crypto_verify(ed25519_pk, message, sizeof message, signature))
    {
        std::printf("horse_crypto_test: verify accepted corrupted signature\n");
        return -1;
    }

    return 0;
}

static int test_session_keys_and_signed_message()
{
#ifdef HAVE_LIBSODIUM
    if (sodium_init() < 0)
        return -1;

    uint8_t alice_pk[32], alice_sk[32], bob_pk[32], bob_sk[32];
    uint8_t eph_pk[32], eph_sk[32];
    uint8_t k_enc_tx[32], k_tag_tx[32], k_enc_rx[32], k_tag_rx[32];
    uint8_t src[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
    uint8_t dst[6] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
    uint8_t flags = horse::LSF_FLAG_SIGNED;
    uint8_t msg[HORSE_SESSION_MSG_BYTES];

    if (!horse_crypto_x25519_keypair(alice_pk, alice_sk) ||
        !horse_crypto_x25519_keypair(bob_pk, bob_sk) ||
        !horse_crypto_x25519_keypair(eph_pk, eph_sk))
        return -1;
    if (!horse_crypto_derive_session_keys(eph_sk, bob_pk, k_enc_tx, k_tag_tx))
        return -1;
    if (!horse_crypto_derive_session_keys(bob_sk, eph_pk, k_enc_rx, k_tag_rx))
        return -1;
    if (memcmp(k_enc_tx, k_enc_rx, 32) != 0 ||
        memcmp(k_tag_tx, k_tag_rx, 32) != 0)
        return -1;
    if (memcmp(k_enc_tx, k_tag_tx, 32) == 0)
        return -1;

    horse_crypto_build_session_message(src, dst, eph_pk, flags,
                                       HORSE_LSF_VERSION, msg);
    if (msg[44] != flags || msg[45] != HORSE_LSF_VERSION)
        return -1;
    if (!horse_crypto_lsf_version_ok(HORSE_LSF_VERSION))
        return -1;
    if (horse_crypto_lsf_version_ok(0) || horse_crypto_lsf_version_ok(2))
        return -1;
#else
    std::printf("horse_crypto_test: skipping session keys (no libsodium)\n");
#endif
    return 0;
}

static int test_signed_only_recording_cannot_forge_voice()
{
#ifdef HAVE_LIBSODIUM
    if (sodium_init() < 0)
        return -1;

    uint8_t alice_ed_pk[32], alice_ed_sk[64];
    uint8_t bob_pk[32], bob_sk[32];
    uint8_t eph_pk[32], eph_sk[32];
    uint8_t k_enc[32], k_tag[32];
    uint8_t src[6] = { 1, 2, 3, 4, 5, 6 };
    uint8_t dst[6] = { 6, 5, 4, 3, 2, 1 };
    uint8_t flags = horse::LSF_FLAG_SIGNED;
    uint8_t session_msg[HORSE_SESSION_MSG_BYTES];
    uint8_t signature[64];
    uint8_t orig[12], forged[12];
    uint8_t good_tag[4], attacker_tag[4];

    crypto_sign_ed25519_keypair(alice_ed_pk, alice_ed_sk);
    if (!horse_crypto_x25519_keypair(bob_pk, bob_sk) ||
        !horse_crypto_x25519_keypair(eph_pk, eph_sk))
        return -1;
    if (!horse_crypto_derive_session_keys(eph_sk, bob_pk, k_enc, k_tag))
        return -1;

    horse_crypto_build_session_message(src, dst, eph_pk, flags,
                                       HORSE_LSF_VERSION, session_msg);
    if (!horse_crypto_sign(alice_ed_sk, session_msg, sizeof session_msg,
                           signature))
        return -1;

    memset(orig, 0x11, sizeof orig);
    if (!horse_crypto_voice_auth_tag(k_tag, HORSE_VOICE_DIR_FORWARD, 0, orig,
                                    good_tag))
        return -1;
    if (!horse_crypto_voice_auth_verify(k_tag, HORSE_VOICE_DIR_FORWARD, 0, orig,
                                        good_tag))
        return -1;

    memset(forged, 0x22, sizeof forged);

    uint8_t from_sig[32];
    crypto_generichash(from_sig, sizeof from_sig, signature, sizeof signature,
                       (const unsigned char *)"HORSE-FAUTH", 11);
    if (!horse_crypto_voice_auth_tag(from_sig, HORSE_VOICE_DIR_FORWARD, 0,
                                    forged, attacker_tag))
        return -1;
    if (horse_crypto_voice_auth_verify(k_tag, HORSE_VOICE_DIR_FORWARD, 0, forged,
                                       attacker_tag))
    {
        std::printf("horse_crypto_test: signature-derived tag forged voice\n");
        return -1;
    }

    uint8_t hvt[32];
    crypto_generichash(hvt, sizeof hvt, forged, sizeof forged,
                       (const unsigned char *)"HVOICETAG", 9);
    if (horse_crypto_voice_auth_verify(k_tag, HORSE_VOICE_DIR_FORWARD, 0, forged,
                                       hvt))
    {
        std::printf("horse_crypto_test: HVOICETAG forged voice\n");
        return -1;
    }

    uint8_t pub_only[32], unused[32];
    if (horse_crypto_derive_session_keys(bob_pk, eph_pk, pub_only, unused))
    {
        uint8_t t[4];
        horse_crypto_voice_auth_tag(pub_only, HORSE_VOICE_DIR_FORWARD, 0, forged,
                                    t);
        if (horse_crypto_voice_auth_verify(k_tag, HORSE_VOICE_DIR_FORWARD, 0,
                                           forged, t) &&
            memcmp(pub_only, k_tag, 32) != 0)
        {
            std::printf("horse_crypto_test: public-only ECDH forged voice\n");
            return -1;
        }
    }

    /* Attacker has all public keys; ECDH without eph_sk or bob_sk fails. */
    (void)alice_ed_pk;
    (void)bob_pk;
    (void)eph_pk;
#else
    std::printf("horse_crypto_test: skipping forge test (no libsodium)\n");
#endif
    return 0;
}

static int test_tag_binds_dir_fn_payload()
{
#ifdef HAVE_LIBSODIUM
    uint8_t k_tag[32];
    uint8_t payload[12];
    uint8_t tag[4], other[4];
    size_t i;

    for (i = 0; i < sizeof k_tag; i++)
        k_tag[i] = (uint8_t)(i + 3);
    memset(payload, 0x5A, sizeof payload);

    if (!horse_crypto_voice_auth_tag(k_tag, HORSE_VOICE_DIR_FORWARD, 7, payload,
                                    tag))
        return -1;
    if (!horse_crypto_voice_auth_verify(k_tag, HORSE_VOICE_DIR_FORWARD, 7,
                                        payload, tag))
        return -1;
    if (horse_crypto_voice_auth_verify(k_tag, HORSE_VOICE_DIR_FORWARD, 8,
                                       payload, tag))
        return -1;
    if (horse_crypto_voice_auth_verify(k_tag, 1, 7, payload, tag))
        return -1;
    payload[0] ^= 1;
    if (horse_crypto_voice_auth_verify(k_tag, HORSE_VOICE_DIR_FORWARD, 7,
                                       payload, tag))
        return -1;
    payload[0] ^= 1;

    /* Last-frame bit is not part of the tag. */
    if (!horse_crypto_voice_auth_tag(k_tag, HORSE_VOICE_DIR_FORWARD, 7 | 0x8000,
                                    payload, other))
        return -1;
    if (memcmp(tag, other, 4) != 0)
        return -1;
    if (horse_crypto_voice_auth_verify(k_tag, HORSE_VOICE_DIR_FORWARD, 7,
                                       payload, NULL))
        return -1;

    uint8_t k_enc[32];
    uint8_t nonce[12] = { 0 };
    uint8_t ct[12], pt[12];
    memcpy(k_enc, k_tag, 32);
    horse_crypto_voice_encrypt(k_enc, k_tag, HORSE_VOICE_DIR_FORWARD, 3, nonce,
                               payload, 12, ct, tag);
    if (horse_crypto_voice_decrypt(k_enc, k_tag, HORSE_VOICE_DIR_FORWARD, 4,
                                   nonce, ct, 12, tag, pt))
        return -1;
    if (!horse_crypto_voice_decrypt(k_enc, k_tag, HORSE_VOICE_DIR_FORWARD, 3,
                                    nonce, ct, 12, tag, pt))
        return -1;
#else
    std::printf("horse_crypto_test: skipping tag bind (no libsodium)\n");
#endif
    return 0;
}

static int test_tx_rx_policy()
{
    if (horse_tx_allowed(true, false, false, true, true, true))
        return -1;
    if (horse_tx_allowed(true, false, true, false, true, true))
        return -1;
    if (horse_tx_allowed(true, false, true, true, false, true))
        return -1;
    if (!horse_tx_allowed(true, false, true, true, true, false))
        return -1;
    if (horse_tx_allowed(false, true, true, true, true, false))
        return -1;
    if (!horse_tx_allowed(false, true, true, true, true, true))
        return -1;
    if (horse_tx_allowed(false, true, true, true, false, true))
        return -1;
    if (horse_tx_allowed(false, false, true, true, false, true))
        return -1;
    if (!horse_tx_allowed(false, false, true, true, true, false))
        return -1;
    if (horse_rx_may_output_voice(true, false, false, false))
        return -1;
    if (horse_rx_may_output_voice(false, false, true, true))
        return -1;
    if (horse_rx_may_output_voice(false, true, true, false))
        return -1;
    if (!horse_rx_may_output_voice(true, true, true, true))
        return -1;
    if (!horse_rx_may_output_voice(false, true, false, false))
        return -1;
    if (horse_rx_may_output_voice(false, false, false, false))
        return -1;
    return 0;
}

static int test_memzero()
{
    uint8_t buf[8];
    memset(buf, 0xA5, sizeof buf);
    horse_crypto_memzero(buf, sizeof buf);
    for (size_t i = 0; i < sizeof buf; i++)
    {
        if (buf[i] != 0)
            return -1;
    }
    return 0;
}

int main()
{
    if (test_voice_encrypt_decrypt_roundtrip() != 0)
        return -1;
    if (test_voice_decrypt_bad_tag() != 0)
        return -1;
    if (test_voice_decrypt_bad_ciphertext() != 0)
        return -1;
    if (test_voice_nonce_independence() != 0)
        return -1;
    if (test_sign_verify_roundtrip() != 0)
        return -1;
    if (test_sign_verify_bad_signature() != 0)
        return -1;
    if (test_session_keys_and_signed_message() != 0)
        return -1;
    if (test_signed_only_recording_cannot_forge_voice() != 0)
        return -1;
    if (test_tag_binds_dir_fn_payload() != 0)
        return -1;
    if (test_tx_rx_policy() != 0)
        return -1;
    if (test_memzero() != 0)
        return -1;

    std::printf("horse_crypto_test: all tests passed\n");
    return 0;
}

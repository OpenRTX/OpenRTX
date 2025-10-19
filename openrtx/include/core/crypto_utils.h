/**
 * @file crypto_utils.h
 * @brief Cryptographic security utilities for OpenRTX
 * 
 * This header provides secure cryptographic functions and
 * utilities to prevent cryptographic vulnerabilities.
 */

#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Cryptographic key types
 */
typedef enum {
    CRYPTO_KEY_AES_128 = 0,
    CRYPTO_KEY_AES_256,
    CRYPTO_KEY_M17_ENCRYPTION,
    CRYPTO_KEY_MAX
} crypto_key_type_t;

/**
 * @brief Secure key structure
 */
typedef struct {
    uint8_t key[32];           // Maximum key size (256 bits)
    uint8_t key_size;         // Actual key size in bytes
    crypto_key_type_t type;   // Key type
    bool is_initialized;      // Key initialization status
} secure_key_t;

/**
 * @brief Initialize a secure key
 * @param key Pointer to key structure
 * @param key_data Key data
 * @param key_size Size of key data
 * @param type Key type
 * @return 0 on success, -1 on error
 */
int secure_key_init(secure_key_t *key, const uint8_t *key_data, 
                    uint8_t key_size, crypto_key_type_t type);

/**
 * @brief Clear a secure key from memory
 * @param key Pointer to key structure
 */
void secure_key_clear(secure_key_t *key);

/**
 * @brief Secure key copy
 * @param dest Destination key
 * @param src Source key
 * @return 0 on success, -1 on error
 */
int secure_key_copy(secure_key_t *dest, const secure_key_t *src);

/**
 * @brief Secure random number generation
 * @param buffer Buffer to fill with random data
 * @param size Size of buffer
 * @return 0 on success, -1 on error
 */
int secure_random(uint8_t *buffer, size_t size);

/**
 * @brief Secure random number generation for single byte
 * @return Random byte value
 */
uint8_t secure_random_byte(void);

/**
 * @brief Secure random number generation for 32-bit value
 * @return Random 32-bit value
 */
uint32_t secure_random_uint32(void);

/**
 * @brief Secure memory comparison (constant time)
 * @param a First buffer
 * @param b Second buffer
 * @param size Size of buffers
 * @return true if equal, false if different
 */
bool secure_memcmp(const void *a, const void *b, size_t size);

/**
 * @brief Secure string comparison (constant time)
 * @param a First string
 * @param b Second string
 * @return true if equal, false if different
 */
bool secure_strcmp(const char *a, const char *b);

/**
 * @brief Secure memory clear (prevents compiler optimization)
 * @param ptr Pointer to memory
 * @param size Size of memory to clear
 */
void secure_memclear(void *ptr, size_t size);

/**
 * @brief Secure string clear
 * @param str String to clear
 * @param max_len Maximum length to clear
 */
void secure_strclear(char *str, size_t max_len);

/**
 * @brief M17 encryption key validation
 * @param key Pointer to key
 * @return true if valid, false if invalid
 */
bool m17_key_validate(const secure_key_t *key);

/**
 * @brief M17 encryption key generation
 * @param key Pointer to key structure
 * @return 0 on success, -1 on error
 */
int m17_key_generate(secure_key_t *key);

/**
 * @brief M17 encryption
 * @param key Encryption key
 * @param plaintext Plaintext data
 * @param plaintext_size Size of plaintext
 * @param ciphertext Ciphertext buffer
 * @param ciphertext_size Size of ciphertext buffer
 * @return 0 on success, -1 on error
 */
int m17_encrypt(const secure_key_t *key, const uint8_t *plaintext, 
                size_t plaintext_size, uint8_t *ciphertext, 
                size_t ciphertext_size);

/**
 * @brief M17 decryption
 * @param key Decryption key
 * @param ciphertext Ciphertext data
 * @param ciphertext_size Size of ciphertext
 * @param plaintext Plaintext buffer
 * @param plaintext_size Size of plaintext buffer
 * @return 0 on success, -1 on error
 */
int m17_decrypt(const secure_key_t *key, const uint8_t *ciphertext, 
                size_t ciphertext_size, uint8_t *plaintext, 
                size_t plaintext_size);

/**
 * @brief Cryptographic hash function (SHA-256)
 * @param data Data to hash
 * @param data_size Size of data
 * @param hash Hash output buffer (32 bytes)
 * @return 0 on success, -1 on error
 */
int crypto_hash_sha256(const uint8_t *data, size_t data_size, uint8_t *hash);

/**
 * @brief Key derivation function (PBKDF2)
 * @param password Password
 * @param password_len Password length
 * @param salt Salt
 * @param salt_len Salt length
 * @param iterations Number of iterations
 * @param key Output key
 * @param key_len Key length
 * @return 0 on success, -1 on error
 */
int crypto_pbkdf2(const char *password, size_t password_len,
                  const uint8_t *salt, size_t salt_len,
                  uint32_t iterations, uint8_t *key, size_t key_len);

/**
 * @brief Secure key storage
 * @param key Key to store
 * @param storage_id Storage identifier
 * @return 0 on success, -1 on error
 */
int secure_key_store(const secure_key_t *key, uint32_t storage_id);

/**
 * @brief Secure key retrieval
 * @param key Key structure to fill
 * @param storage_id Storage identifier
 * @return 0 on success, -1 on error
 */
int secure_key_retrieve(secure_key_t *key, uint32_t storage_id);

/**
 * @brief Secure key deletion
 * @param storage_id Storage identifier
 * @return 0 on success, -1 on error
 */
int secure_key_delete(uint32_t storage_id);

/**
 * @brief Cryptographic initialization
 * @return 0 on success, -1 on error
 */
int crypto_init(void);

/**
 * @brief Cryptographic cleanup
 * @return 0 on success, -1 on error
 */
int crypto_cleanup(void);

/**
 * @brief Security audit of cryptographic implementations
 * @return 0 if secure, -1 if vulnerabilities found
 */
int crypto_security_audit(void);

// Security constants
#define CRYPTO_MAX_KEY_SIZE 32
#define CRYPTO_MIN_KEY_SIZE 16
#define CRYPTO_HASH_SIZE 32
#define CRYPTO_SALT_SIZE 16
#define CRYPTO_IV_SIZE 16

// Security macros
#define SECURE_KEY_INIT(key, data, size, type) \
    secure_key_init(key, data, size, type)

#define SECURE_KEY_CLEAR(key) \
    secure_key_clear(key)

#define SECURE_RANDOM(buf, size) \
    secure_random(buf, size)

#define SECURE_MEMCMP(a, b, size) \
    secure_memcmp(a, b, size)

#define SECURE_STRCMP(a, b) \
    secure_strcmp(a, b)

#endif // CRYPTO_UTILS_H

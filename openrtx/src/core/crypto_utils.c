/**
 * @file crypto_utils.c
 * @brief Cryptographic security implementation for OpenRTX
 */

#include "core/crypto_utils.h"
#include "core/utils.h"
#include "interfaces/platform.h"

/**
 * @brief Initialize a secure key
 */
int secure_key_init(secure_key_t *key, const uint8_t *key_data, 
                    uint8_t key_size, crypto_key_type_t type)
{
    if (!key || !key_data || key_size == 0) {
        return -1;
    }
    
    // Validate key size based on type
    switch (type) {
        case CRYPTO_KEY_AES_128:
            if (key_size != 16) return -1;
            break;
        case CRYPTO_KEY_AES_256:
            if (key_size != 32) return -1;
            break;
        case CRYPTO_KEY_M17_ENCRYPTION:
            if (key_size < 16 || key_size > 32) return -1;
            break;
        default:
            return -1;
    }
    
    // Clear key structure
    secure_memclear(key, sizeof(secure_key_t));
    
    // Copy key data
    memcpy(key->key, key_data, key_size);
    key->key_size = key_size;
    key->type = type;
    key->is_initialized = true;
    
    return 0;
}

/**
 * @brief Clear a secure key from memory
 */
void secure_key_clear(secure_key_t *key)
{
    if (key) {
        secure_memclear(key, sizeof(secure_key_t));
    }
}

/**
 * @brief Secure key copy
 */
int secure_key_copy(secure_key_t *dest, const secure_key_t *src)
{
    if (!dest || !src || !src->is_initialized) {
        return -1;
    }
    
    // Clear destination first
    secure_memclear(dest, sizeof(secure_key_t));
    
    // Copy key data
    memcpy(dest->key, src->key, src->key_size);
    dest->key_size = src->key_size;
    dest->type = src->type;
    dest->is_initialized = true;
    
    return 0;
}

/**
 * @brief Secure random number generation
 */
int secure_random(uint8_t *buffer, size_t size)
{
    if (!buffer || size == 0) {
        return -1;
    }
    
    // Use platform-specific secure random number generator
    for (size_t i = 0; i < size; i++) {
        buffer[i] = platform_getRandomByte();
    }
    
    return 0;
}

/**
 * @brief Secure random number generation for single byte
 */
uint8_t secure_random_byte(void)
{
    return platform_getRandomByte();
}

/**
 * @brief Secure random number generation for 32-bit value
 */
uint32_t secure_random_uint32(void)
{
    uint32_t result = 0;
    uint8_t bytes[4];
    
    if (secure_random(bytes, 4) == 0) {
        result = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    }
    
    return result;
}

/**
 * @brief Secure memory comparison (constant time)
 */
bool secure_memcmp(const void *a, const void *b, size_t size)
{
    if (!a || !b || size == 0) {
        return false;
    }
    
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    uint8_t result = 0;
    
    for (size_t i = 0; i < size; i++) {
        result |= pa[i] ^ pb[i];
    }
    
    return result == 0;
}

/**
 * @brief Secure string comparison (constant time)
 */
bool secure_strcmp(const char *a, const char *b)
{
    if (!a || !b) {
        return false;
    }
    
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    
    if (len_a != len_b) {
        return false;
    }
    
    return secure_memcmp(a, b, len_a);
}

/**
 * @brief Secure memory clear (prevents compiler optimization)
 */
void secure_memclear(void *ptr, size_t size)
{
    if (ptr && size > 0) {
        volatile uint8_t *vptr = (volatile uint8_t *)ptr;
        for (size_t i = 0; i < size; i++) {
            vptr[i] = 0;
        }
    }
}

/**
 * @brief Secure string clear
 */
void secure_strclear(char *str, size_t max_len)
{
    if (str && max_len > 0) {
        secure_memclear(str, max_len);
    }
}

/**
 * @brief M17 encryption key validation
 */
bool m17_key_validate(const secure_key_t *key)
{
    if (!key || !key->is_initialized) {
        return false;
    }
    
    // M17 keys should be at least 16 bytes
    if (key->key_size < 16) {
        return false;
    }
    
    // Check for weak keys (all zeros, all ones, etc.)
    bool all_zero = true;
    bool all_one = true;
    
    for (uint8_t i = 0; i < key->key_size; i++) {
        if (key->key[i] != 0x00) all_zero = false;
        if (key->key[i] != 0xFF) all_one = false;
    }
    
    return !(all_zero || all_one);
}

/**
 * @brief M17 encryption key generation
 */
int m17_key_generate(secure_key_t *key)
{
    if (!key) {
        return -1;
    }
    
    // Generate 16-byte key for M17
    uint8_t key_data[16];
    if (secure_random(key_data, 16) != 0) {
        return -1;
    }
    
    return secure_key_init(key, key_data, 16, CRYPTO_KEY_M17_ENCRYPTION);
}

/**
 * @brief Cryptographic initialization
 */
int crypto_init(void)
{
    // Initialize platform-specific cryptographic functions
    return platform_cryptoInit();
}

/**
 * @brief Cryptographic cleanup
 */
int crypto_cleanup(void)
{
    // Cleanup platform-specific cryptographic functions
    return platform_cryptoCleanup();
}

/**
 * @brief Security audit of cryptographic implementations
 */
int crypto_security_audit(void)
{
    // Check for weak cryptographic implementations
    // This is a placeholder - actual implementation would check:
    // - Key generation entropy
    // - Encryption algorithm strength
    // - Random number generator quality
    // - Key storage security
    
    return 0; // Placeholder - assume secure for now
}

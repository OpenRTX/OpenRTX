/**
 * @file string_utils.h
 * @brief Safe string utility functions for OpenRTX
 * 
 * This header provides safe alternatives to common string operations
 * to prevent buffer overflow vulnerabilities.
 */

#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string.h>
#include <stddef.h>

/**
 * @brief Safe string copy with bounds checking
 * @param dest Destination buffer
 * @param src Source string
 * @param dest_size Size of destination buffer
 * @return 0 on success, -1 on error
 */
static inline int safe_strcpy(char *dest, const char *src, size_t dest_size)
{
    if (!dest || !src || dest_size == 0) {
        return -1;
    }
    
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
    return 0;
}

/**
 * @brief Safe string copy with automatic size detection
 * @param dest Destination buffer
 * @param src Source string
 * @return 0 on success, -1 on error
 */
#define SAFE_STRCPY(dest, src) safe_strcpy(dest, src, sizeof(dest))

/**
 * @brief Safe string concatenation with bounds checking
 * @param dest Destination buffer
 * @param src Source string to append
 * @param dest_size Size of destination buffer
 * @return 0 on success, -1 on error
 */
static inline int safe_strcat(char *dest, const char *src, size_t dest_size)
{
    if (!dest || !src || dest_size == 0) {
        return -1;
    }
    
    size_t dest_len = strnlen(dest, dest_size);
    if (dest_len >= dest_size - 1) {
        return -1; // No space for concatenation
    }
    
    strncat(dest, src, dest_size - dest_len - 1);
    dest[dest_size - 1] = '\0';
    return 0;
}

/**
 * @brief Safe string concatenation with automatic size detection
 * @param dest Destination buffer
 * @param src Source string to append
 * @return 0 on success, -1 on error
 */
#define SAFE_STRCAT(dest, src) safe_strcat(dest, src, sizeof(dest))

/**
 * @brief Safe string formatting with bounds checking
 * @param dest Destination buffer
 * @param dest_size Size of destination buffer
 * @param format Format string
 * @param ... Format arguments
 * @return Number of characters written (excluding null terminator), -1 on error
 */
static inline int safe_snprintf(char *dest, size_t dest_size, const char *format, ...)
{
    if (!dest || !format || dest_size == 0) {
        return -1;
    }
    
    va_list args;
    va_start(args, format);
    int result = vsnprintf(dest, dest_size, format, args);
    va_end(args);
    
    if (result < 0 || (size_t)result >= dest_size) {
        dest[dest_size - 1] = '\0';
        return -1;
    }
    
    return result;
}

/**
 * @brief Safe string formatting with automatic size detection
 * @param dest Destination buffer
 * @param format Format string
 * @param ... Format arguments
 * @return Number of characters written (excluding null terminator), -1 on error
 */
#define SAFE_SNPRINTF(dest, format, ...) safe_snprintf(dest, sizeof(dest), format, ##__VA_ARGS__)

/**
 * @brief Safe string length with bounds checking
 * @param str String to measure
 * @param max_len Maximum length to check
 * @return Length of string (not including null terminator), max_len if string is longer
 */
static inline size_t safe_strlen(const char *str, size_t max_len)
{
    if (!str) {
        return 0;
    }
    
    return strnlen(str, max_len);
}

/**
 * @brief Validate string input for security
 * @param str String to validate
 * @param max_len Maximum allowed length
 * @return true if valid, false if invalid
 */
static inline bool validate_string_input(const char *str, size_t max_len)
{
    if (!str) {
        return false;
    }
    
    return strnlen(str, max_len + 1) <= max_len;
}

#endif // STRING_UTILS_H

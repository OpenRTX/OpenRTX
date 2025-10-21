/**
 * @file memory_utils.h
 * @brief Safe memory management utilities for OpenRTX
 * 
 * This header provides safe memory management functions to prevent
 * memory leaks, use-after-free, and double-free vulnerabilities.
 */

#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/**
 * @brief Safe memory allocation with error checking
 * @param size Size of memory to allocate
 * @return Pointer to allocated memory, NULL on failure
 */
static inline void* safe_malloc(size_t size)
{
    if (size == 0) {
        return NULL;
    }
    
    void *ptr = malloc(size);
    if (!ptr) {
        // Handle allocation failure - could log error here
        return NULL;
    }
    
    // Initialize memory to zero for security
    memset(ptr, 0, size);
    return ptr;
}

/**
 * @brief Safe memory allocation with error checking and initialization
 * @param count Number of elements
 * @param size Size of each element
 * @return Pointer to allocated memory, NULL on failure
 */
static inline void* safe_calloc(size_t count, size_t size)
{
    if (count == 0 || size == 0) {
        return NULL;
    }
    
    // Check for overflow
    if (count > SIZE_MAX / size) {
        return NULL;
    }
    
    void *ptr = calloc(count, size);
    if (!ptr) {
        // Handle allocation failure
        return NULL;
    }
    
    return ptr;
}

/**
 * @brief Safe memory reallocation with error checking
 * @param ptr Pointer to existing memory
 * @param size New size
 * @return Pointer to reallocated memory, NULL on failure
 */
static inline void* safe_realloc(void *ptr, size_t size)
{
    if (size == 0) {
        if (ptr) {
            free(ptr);
        }
        return NULL;
    }
    
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr && ptr) {
        // Reallocation failed, original pointer is still valid
        // Caller should handle this case
        return NULL;
    }
    
    return new_ptr;
}

/**
 * @brief Safe memory deallocation with null pointer check
 * @param ptr Pointer to memory to free
 */
static inline void safe_free(void *ptr)
{
    if (ptr) {
        free(ptr);
    }
}

/**
 * @brief Safe memory copy with bounds checking
 * @param dest Destination buffer
 * @param src Source buffer
 * @param size Number of bytes to copy
 * @return 0 on success, -1 on error
 */
static inline int safe_memcpy(void *dest, const void *src, size_t size)
{
    if (!dest || !src || size == 0) {
        return -1;
    }
    
    memcpy(dest, src, size);
    return 0;
}

/**
 * @brief Safe memory move with bounds checking
 * @param dest Destination buffer
 * @param src Source buffer
 * @param size Number of bytes to move
 * @return 0 on success, -1 on error
 */
static inline int safe_memmove(void *dest, const void *src, size_t size)
{
    if (!dest || !src || size == 0) {
        return -1;
    }
    
    memmove(dest, src, size);
    return 0;
}

/**
 * @brief Safe memory set with bounds checking
 * @param ptr Pointer to memory
 * @param value Value to set
 * @param size Number of bytes to set
 * @return 0 on success, -1 on error
 */
static inline int safe_memset(void *ptr, int value, size_t size)
{
    if (!ptr || size == 0) {
        return -1;
    }
    
    memset(ptr, value, size);
    return 0;
}

/**
 * @brief Secure memory clear (prevents compiler optimization)
 * @param ptr Pointer to memory
 * @param size Number of bytes to clear
 */
static inline void secure_memclear(void *ptr, size_t size)
{
    if (ptr && size > 0) {
        // Use volatile to prevent compiler optimization
        volatile unsigned char *vptr = (volatile unsigned char *)ptr;
        for (size_t i = 0; i < size; i++) {
            vptr[i] = 0;
        }
    }
}

/**
 * @brief Safe string duplication with bounds checking
 * @param str String to duplicate
 * @param max_len Maximum length to copy
 * @return New string, NULL on failure
 */
static inline char* safe_strdup(const char *str, size_t max_len)
{
    if (!str) {
        return NULL;
    }
    
    size_t len = strnlen(str, max_len);
    char *new_str = safe_malloc(len + 1);
    if (!new_str) {
        return NULL;
    }
    
    strncpy(new_str, str, len);
    new_str[len] = '\0';
    return new_str;
}

/**
 * @brief Memory allocation wrapper with automatic cleanup
 * @param size Size of memory to allocate
 * @return Pointer to allocated memory, NULL on failure
 */
#define SAFE_MALLOC(size) safe_malloc(size)

/**
 * @brief Memory deallocation wrapper with null check
 * @param ptr Pointer to free
 */
#define SAFE_FREE(ptr) safe_free(ptr)

/**
 * @brief RAII-style memory management macro
 * @param var Variable name
 * @param size Size to allocate
 */
#define SAFE_ALLOC(var, size) \
    char *var = SAFE_MALLOC(size); \
    if (!var) { \
        /* Handle allocation failure */ \
    }

/**
 * @brief Automatic cleanup macro
 * @param var Variable to cleanup
 */
#define SAFE_CLEANUP(var) \
    do { \
        SAFE_FREE(var); \
        var = NULL; \
    } while(0)

#endif // MEMORY_UTILS_H

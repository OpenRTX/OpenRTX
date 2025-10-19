/**
 * @file thread_utils.h
 * @brief Thread safety utilities for OpenRTX
 * 
 * This header provides thread safety mechanisms to prevent
 * race conditions and data corruption in multi-threaded code.
 */

#ifndef THREAD_UTILS_H
#define THREAD_UTILS_H

#include <stdbool.h>
#include <stdint.h>

// Platform-specific includes
#ifdef _MIOSIX
#include <pthread.h>
#elif defined(__ZEPHYR__)
#include <zephyr/kernel.h>
#else
// Fallback for other platforms
#include <pthread.h>
#endif

/**
 * @brief Thread-safe mutex wrapper
 */
typedef struct {
#ifdef _MIOSIX
    pthread_mutex_t mutex;
#elif defined(__ZEPHYR__)
    struct k_mutex mutex;
#else
    pthread_mutex_t mutex;
#endif
    bool initialized;
} safe_mutex_t;

/**
 * @brief Initialize a safe mutex
 * @param mutex Pointer to mutex
 * @return 0 on success, -1 on error
 */
static inline int safe_mutex_init(safe_mutex_t *mutex)
{
    if (!mutex) {
        return -1;
    }
    
#ifdef _MIOSIX
    int result = pthread_mutex_init(&mutex->mutex, NULL);
#elif defined(__ZEPHYR__)
    k_mutex_init(&mutex->mutex);
    int result = 0;
#else
    int result = pthread_mutex_init(&mutex->mutex, NULL);
#endif
    
    if (result == 0) {
        mutex->initialized = true;
    }
    
    return result;
}

/**
 * @brief Lock a safe mutex
 * @param mutex Pointer to mutex
 * @return 0 on success, -1 on error
 */
static inline int safe_mutex_lock(safe_mutex_t *mutex)
{
    if (!mutex || !mutex->initialized) {
        return -1;
    }
    
#ifdef _MIOSIX
    return pthread_mutex_lock(&mutex->mutex);
#elif defined(__ZEPHYR__)
    k_mutex_lock(&mutex->mutex, K_FOREVER);
    return 0;
#else
    return pthread_mutex_lock(&mutex->mutex);
#endif
}

/**
 * @brief Unlock a safe mutex
 * @param mutex Pointer to mutex
 * @return 0 on success, -1 on error
 */
static inline int safe_mutex_unlock(safe_mutex_t *mutex)
{
    if (!mutex || !mutex->initialized) {
        return -1;
    }
    
#ifdef _MIOSIX
    return pthread_mutex_unlock(&mutex->mutex);
#elif defined(__ZEPHYR__)
    k_mutex_unlock(&mutex->mutex);
    return 0;
#else
    return pthread_mutex_unlock(&mutex->mutex);
#endif
}

/**
 * @brief Destroy a safe mutex
 * @param mutex Pointer to mutex
 * @return 0 on success, -1 on error
 */
static inline int safe_mutex_destroy(safe_mutex_t *mutex)
{
    if (!mutex || !mutex->initialized) {
        return -1;
    }
    
#ifdef _MIOSIX
    int result = pthread_mutex_destroy(&mutex->mutex);
#elif defined(__ZEPHYR__)
    // Zephyr mutexes don't need explicit destruction
    int result = 0;
#else
    int result = pthread_mutex_destroy(&mutex->mutex);
#endif
    
    if (result == 0) {
        mutex->initialized = false;
    }
    
    return result;
}

/**
 * @brief Thread-safe atomic operations
 */
typedef struct {
    volatile int32_t value;
} safe_atomic_t;

/**
 * @brief Initialize atomic variable
 * @param atomic Pointer to atomic variable
 * @param initial_value Initial value
 */
static inline void safe_atomic_init(safe_atomic_t *atomic, int32_t initial_value)
{
    if (atomic) {
        atomic->value = initial_value;
    }
}

/**
 * @brief Atomic increment
 * @param atomic Pointer to atomic variable
 * @return New value
 */
static inline int32_t safe_atomic_inc(safe_atomic_t *atomic)
{
    if (!atomic) {
        return 0;
    }
    
    return __sync_add_and_fetch(&atomic->value, 1);
}

/**
 * @brief Atomic decrement
 * @param atomic Pointer to atomic variable
 * @return New value
 */
static inline int32_t safe_atomic_dec(safe_atomic_t *atomic)
{
    if (!atomic) {
        return 0;
    }
    
    return __sync_sub_and_fetch(&atomic->value, 1);
}

/**
 * @brief Atomic load
 * @param atomic Pointer to atomic variable
 * @return Current value
 */
static inline int32_t safe_atomic_load(safe_atomic_t *atomic)
{
    if (!atomic) {
        return 0;
    }
    
    return __sync_fetch_and_add(&atomic->value, 0);
}

/**
 * @brief Atomic store
 * @param atomic Pointer to atomic variable
 * @param value Value to store
 */
static inline void safe_atomic_store(safe_atomic_t *atomic, int32_t value)
{
    if (atomic) {
        __sync_lock_test_and_set(&atomic->value, value);
    }
}

/**
 * @brief Thread-safe critical section macro
 * @param mutex Mutex to use
 * @param code Code to execute in critical section
 */
#define SAFE_CRITICAL_SECTION(mutex, code) \
    do { \
        if (safe_mutex_lock(mutex) == 0) { \
            code; \
            safe_mutex_unlock(mutex); \
        } \
    } while(0)

/**
 * @brief Thread-safe variable access macro
 * @param mutex Mutex to use
 * @param var Variable to access
 * @param operation Operation to perform
 */
#define SAFE_VAR_ACCESS(mutex, var, operation) \
    SAFE_CRITICAL_SECTION(mutex, var operation)

/**
 * @brief Thread-safe function call macro
 * @param mutex Mutex to use
 * @param func Function to call
 * @param args Function arguments
 */
#define SAFE_FUNCTION_CALL(mutex, func, args) \
    SAFE_CRITICAL_SECTION(mutex, func args)

/**
 * @brief Thread-safe global variable access
 * @param var Global variable
 * @param operation Operation to perform
 */
#define SAFE_GLOBAL_ACCESS(var, operation) \
    SAFE_CRITICAL_SECTION(&global_mutex, var operation)

// Global mutex for global variable access
extern safe_mutex_t global_mutex;

/**
 * @brief Initialize thread safety system
 * @return 0 on success, -1 on error
 */
int thread_safety_init(void);

/**
 * @brief Cleanup thread safety system
 * @return 0 on success, -1 on error
 */
int thread_safety_cleanup(void);

#endif // THREAD_UTILS_H

/**
 * @file thread_utils.c
 * @brief Thread safety implementation for OpenRTX
 */

#include "core/thread_utils.h"
#include "core/utils.h"

// Global mutex for global variable access
safe_mutex_t global_mutex = {0};

/**
 * @brief Initialize thread safety system
 */
int thread_safety_init(void)
{
    int result = 0;
    
    // Initialize global mutex
    result = safe_mutex_init(&global_mutex);
    if (result != 0) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief Cleanup thread safety system
 */
int thread_safety_cleanup(void)
{
    int result = 0;
    
    // Destroy global mutex
    result = safe_mutex_destroy(&global_mutex);
    if (result != 0) {
        return -1;
    }
    
    return 0;
}

#ifndef LOG_H
#define LOG_H

#include "mst_config.h"

#define LOG_LEVEL LOG_LEVEL_DEBUG


// \------------------------------------------------------------

#if defined(PLATFORM_PC)

    #include <stdio.h>

    #ifndef LOG_LEVEL
    #define LOG_LEVEL LOG_LEVEL_DEBUG
    #endif

    #define LOG_LEVEL_NONE     0
    #define LOG_LEVEL_ERROR    1
    #define LOG_LEVEL_WARN     2
    #define LOG_LEVEL_INFO     3
    #define LOG_LEVEL_DEBUG    4
    #define LOG_LEVEL_VERBOSE  5

    #define LOG_FUNC(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

    #define LOGE(fmt, ...)                                                  \
        do {                                                                \
            if (LOG_LEVEL >= LOG_LEVEL_DEBUG) LOG_FUNC(fmt,##__VA_ARGS__);  \
        } while(0)

    #define LOGW(fmt, ...)                                                  \
        do {                                                                \
            if (LOG_LEVEL >= LOG_LEVEL_WARN) LOG_FUNC(fmt,##__VA_ARGS__);   \
        } while(0)

    #define LOGI(fmt, ...)                                                  \
        do {                                                                \
            if (LOG_LEVEL >= LOG_LEVEL_INFO) LOG_FUNC(fmt,##__VA_ARGS__);   \
        } while(0)

    #define LOGD(fmt, ...)                                                  \
        do {                                                                \
            if (LOG_LEVEL >= LOG_LEVEL_DEBUG) LOG_FUNC(fmt,##__VA_ARGS__);  \
        } while(0)

    #define LOGV(fmt, ...)                                                  \
        do {                                                                \
            if (LOG_LEVEL >= LOG_LEVEL_VERBOSE) LOG_FUNC(fmt,##__VA_ARGS__);\
        } while(0)



#elif defined(PLATFORM_RTOS)

    #include "cst_log.h"

    #define LOGE CLOGE
    #define LOGW CLOGW
    #define LOGI CLOGI
    #define LOGD CLOGV
    #define LOGV CLOGV

#endif // PLATFORM_XXX

#endif // LOG_H

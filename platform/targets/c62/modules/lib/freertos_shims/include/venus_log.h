#pragma once

#include <zephyr/kernel.h>

#define CLOG_LEVEL_NONE     0
#define CLOG_LEVEL_ERROR    1
#define CLOG_LEVEL_WARN     2
#define CLOG_LEVEL_INFO     3
#define CLOG_LEVEL_DEBUG    4
#define CLOG_LEVEL_VERBOSE  5

#define CLOG_LEVEL CONFIG_VENUS_CLOG_LEVEL

#define CLOG(fmt, ...)   printk(fmt"\n", ##__VA_ARGS__)

#define CLOGE(fmt, ...)     do {if (CLOG_LEVEL >= CLOG_LEVEL_ERROR)   CLOG(fmt,##__VA_ARGS__);} while(0)
#define CLOGW(fmt, ...)     do {if (CLOG_LEVEL >= CLOG_LEVEL_WARN)    CLOG(fmt,##__VA_ARGS__);} while(0)
#define CLOGI(fmt, ...)     do {if (CLOG_LEVEL >= CLOG_LEVEL_INFO)    CLOG(fmt,##__VA_ARGS__);} while(0)
#define CLOGD(fmt, ...)     do {if (CLOG_LEVEL >= CLOG_LEVEL_DEBUG)   CLOG(fmt,##__VA_ARGS__);} while(0)
#define CLOGV(fmt, ...)     do {if (CLOG_LEVEL >= CLOG_LEVEL_VERBOSE) CLOG(fmt,##__VA_ARGS__);} while(0)

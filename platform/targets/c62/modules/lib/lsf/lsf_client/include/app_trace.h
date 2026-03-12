#ifndef __APP_TRACE_H
#define __APP_TRACE_H

#include <stdint.h>

enum {
    Consumer_waitFrame_Get = 1000,
    Consumer_waitFrame_Put,
    Consumer_releaseFrame_TryGet,
    DataFramesIncrement_handler_Put,

    Consumer_isEmpty,
    Consumer_acquireFrame,

    Producer_isFull = 2000,
    Producer_waitFrame_Take,
    Producer_waitFrame_Give,
    Producer_acquireFrame,
    Producer_releaseFrame_Take,
    EmptyFramesIncrement_push_responser_Give,
};

void Trace_init(void);

void Trace_printAll(void);

void write_syslog(uint32_t v1, uint32_t v2);

void Trace_dump(void);

#endif // __APP_TRACE_H

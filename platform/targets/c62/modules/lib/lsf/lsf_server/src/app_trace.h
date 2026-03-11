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

    ScanDevice_ISR_Entry = 3000,
    ScanDevice_ISR_Exit,
    ScanDevice_ISR_IsStopping,

    ScanDevice_stop_Entry,
    ScanDevice_stop_Exit,

    ScanDevice_start_Entry,
    ScanDevice_start_Exit,

    AudioStream_Producer_acquireFrame,
    AudioStream_Producer_releaseFrame,
    AudioStream_Consumer_acquireFrame,
    AudioStream_Consumer_releaseFrame,
};

void Trace_init(void);

void Trace_printAll(void);

void write_syslog(uint32_t v1, uint32_t v2);

void Trace_dump(void);

#endif // __APP_TRACE_H

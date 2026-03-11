#include "app_trace.h"

#include "xos_log.h"

// #include <xtensa/hal.h>

// #include <xtensa/xos.h>
// #include <xtensa/xos_log.h>

#include <stdio.h>
#include <stdlib.h>

#include "lsf_os_port_internal.h"

#include "venus_log.h"

static uint32_t  total_cycles;
static uint32_t  max_time = 0;
static uint32_t  min_time = 1000000;
static uint32_t  avg_time;
static uint32_t  num_writes;


//-----------------------------------------------------------------------------
// Write to syslog and measure time taken.
//-----------------------------------------------------------------------------
void
write_syslog(uint32_t v1, uint32_t v2)
{
    // uint32_t ps;
    uint32_t diff;
    uint32_t now = lsf_systick_time(); //xos_get_ccount();

    xos_log_write((uint32_t) lsf_task_get_current_handle(), v1, v2);
    diff = lsf_systick_time() - now;

    // Ignore large diffs due to possible interrupt or context switch.
    if (diff > 1000) {
        //CLOGD("---- %u\n", diff);
        return;
    }

    // ps = xos_disable_interrupts();
    lsf_task_enter_critcal();
    if (diff > max_time)
        max_time = diff;
    if (diff < min_time)
        min_time = diff;
    total_cycles += diff;
    num_writes++;
    // xos_restore_interrupts(ps);
    lsf_task_exit_critcal();
}


//-----------------------------------------------------------------------------
// Print one syslog entry.
//-----------------------------------------------------------------------------
static void
print_syslog_entry(const XosLogEntry * entry)
{
    switch (entry->param1) {
    case XOS_SE_THREAD_CREATE:
        CLOGD("%u: Thread_Create %p %d\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_THREAD_DELETE:
        CLOGD("%u: Thread_Delete %p\n", entry->timestamp, entry->param2);
        break;
    case XOS_SE_THREAD_SWITCH:
        CLOGD("%u: Thread_Switch %p -> %p\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_THREAD_YIELD:
        CLOGD("%u: Thread_Yield %p\n", entry->timestamp, entry->param2);
        break;
    case XOS_SE_THREAD_BLOCK:
        CLOGD("%u: Thread_Block %p (%s)\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_THREAD_WAKE:
        CLOGD("%u: Thread_Wake %p %d\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_THREAD_PRI_CHANGE:
        CLOGD("%u: Thread_Pri_Change: %p %d\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_THREAD_SUSPEND:
        CLOGD("%u: Thread_Suspend: %p\n", entry->timestamp, entry->param2);
        break;
    case XOS_SE_THREAD_ABORT:
        CLOGD("%u: Thread_Abort: %p %d\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_TIMER_INTR_START:
        CLOGD("%u: Timer_Handler_Start\n", entry->timestamp);
        break;
    case XOS_SE_TIMER_INTR_END:
        CLOGD("%u: Timer_Handler_End\n", entry->timestamp);
        break;
    case XOS_SE_MUTEX_LOCK:
        CLOGD("%u: Mutex_Lock %p %d\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_MUTEX_UNLOCK:
        CLOGD("%u: Mutex_Unlock %p %d\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_SEM_GET:
        CLOGD("%u: Sem_Get %p %d\n", entry->timestamp, entry->param2, entry->param3);
        break;
    // case XOS_SE_SEM_TRYGET:
    //     CLOGD("%u: Sem_TryGet %p %d\n", entry->timestamp, entry->param2, entry->param3);
    //     break;
    case XOS_SE_SEM_PUT:
        CLOGD("%u: Sem_Put %p %d\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_EVENT_SET:
        CLOGD("%u: Event_Set %p 0x%08x\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_EVENT_CLEAR:
        CLOGD("%u: Event_Clear %p 0x%08x\n", entry->timestamp, entry->param2, entry->param3);
        break;
    case XOS_SE_EVENT_CLEAR_SET:
        CLOGD("%u: Event_Clear_Set %p 0x%08x\n", entry->timestamp, entry->param2, entry->param3);
        break;

    case Consumer_waitFrame_Get:
        CLOGD("%u: Consumer_waitFrame_Get %p %d", entry->timestamp, entry->param2, entry->param3);
        break;
    case Consumer_waitFrame_Put:
        CLOGD("%u: Consumer_waitFrame_Put %p %d", entry->timestamp, entry->param2, entry->param3);
        break;
    case Consumer_releaseFrame_TryGet:
        CLOGD("%u: Consumer_releaseFrame_TryGet %p %d", entry->timestamp, entry->param2, entry->param3);
        break;
    case DataFramesIncrement_handler_Put:
        CLOGD("%u: DataFramesIncrement_handler_Put %p %d", entry->timestamp, entry->param2, entry->param3);
        break;

    case Consumer_isEmpty:
        CLOGD("%u: Consumer_isEmpty, bool = %d, frames %d", entry->timestamp, entry->param2, entry->param3);
        break;
    case Consumer_acquireFrame:
        CLOGD("%u: Consumer_acquireFrame, frames %d", entry->timestamp, entry->param2);
        break;

    default:
        CLOGD("%u: Thread %p %d %d\n", entry->timestamp, entry->param1, entry->param2, entry->param3);
        break;
    }
}

void Trace_printAll(void)
{
    const XosLogEntry * entry;
    uint64_t last_ts;

    xos_log_disable();

    // Read through log.
    entry = xos_log_get_first();
    if (entry == NULL) {
        CLOGD("FAIL: error trying to read syslog\n");
        return;
    }
    last_ts = entry->timestamp;

    while (entry != NULL) {
        print_syslog_entry(entry);
//        CLOGD("%d: 0x%08x %02u %03u\n", entry->timestamp, entry->param1, entry->param2, entry->param3);
        entry = xos_log_get_next(entry);
        if (entry != NULL) {
//            if (entry->timestamp <= last_ts)
//                xos_fatal_error(-1, "FAIL: log entry timestamps going backwards\n");
            last_ts = entry->timestamp;
        }
    }

    avg_time = total_cycles / num_writes;
    CLOGD("Cycles per log write : avg %u min %u max %u\n", avg_time, min_time, max_time);
    CLOGD("PASS\n");
}

static void * logmem;
static int logmem_bytes = XOS_LOG_SIZE(400);

XosLog * xos_log;

void Trace_init(void)
{
    // Allocate memory for syslog and init the log.
    logmem = malloc(logmem_bytes);

    CLOGD("trace at: %p, bytes: %d", logmem, logmem_bytes);

    xos_log_init(logmem, 400, 1);

    return;
}

/** @file */

// xos_log.h - XOS Event logging module.

// Copyright (c) 2018-2020 Cadence Design Systems, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// NOTE: Do not include this file directly in your application. Including
// xos.h will automatically include this file.


#ifndef XOS_LOG_H
#define XOS_LOG_H

// #include <xtensa/core-macros.h>

// #include "xos_types.h"

#include <stdint.h>
#include <stdbool.h>

#include <cmsis_gcc.h>

#include "lsf_os_port_internal.h"

#ifdef __cplusplus
extern "C" {
#endif


//-----------------------------------------------------------------------------
//  The XOS event log is an array of fixed size entries. The size of the log
//  is determined by the application, and memory for the log must be provided
//  at init time. Every time the log function is called, an entry is made in
//  the log and the next pointer advanced. When the log is full, it will wrap
//  around and start overwriting the oldest entries.
//  Logging can be done from C/C++ code as well as assembly code, and at any
//  interrupt level, even from high level interrupt handlers.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Defines.
//-----------------------------------------------------------------------------
#define XOS_LOG_ENABLED                 0x0001U


///----------------------------------------------------------------------------
///
/// Use this macro to compute how much memory to allocate for the event log.
///
///----------------------------------------------------------------------------
#define XOS_LOG_SIZE(num_entries) \
    ( sizeof(XosLog) + (((num_entries) - 1) * sizeof(XosLogEntry)) )


///----------------------------------------------------------------------------
///
///  Event log entry structure.
///
///----------------------------------------------------------------------------
typedef struct XosLogEntry {
    uint32_t                timestamp;  ///< Timestamp in clock cycles
    uint32_t                param1;     ///< User defined value 1
    uint32_t                param2;     ///< User defined value 2
    uint32_t                param3;     ///< User defined value 3
} XosLogEntry;


///----------------------------------------------------------------------------
///
///  Event log structure.
///
///----------------------------------------------------------------------------
typedef struct XosLog {
    uint16_t      flags;                ///< Flags
    uint16_t      size;                 ///< Number of entries
    XosLogEntry * next;                 ///< Next write position
    XosLogEntry * last;                 ///< Pointer to end of log
    XosLogEntry   entries[1];           ///< First entry
} XosLog;


//-----------------------------------------------------------------------------
//  Pointer to event log buffer.
//-----------------------------------------------------------------------------
extern XosLog * xos_log;


//----------------------------------------------------------------------------
///
///  Initialize and optionally enable the event log. The log wraps around when
///  full and overwrites the oldest entries.
///
///  \param     log_mem         Pointer to allocated memory for the log.
///
///  \param     num_entries     The number of entries that the log can contain.
///
///  \param     enable          If true, the log is enabled on init.
///
///  \return Returns nothing.
///
//----------------------------------------------------------------------------
static inline void
xos_log_init(void * log_mem, uint16_t num_entries, bool enable)
{
    uint32_t i;

    xos_log = (XosLog *) log_mem;

    xos_log->size = num_entries;
    xos_log->next = (XosLogEntry *) xos_log->entries;
    xos_log->last = xos_log->next + (num_entries - 1U); // parasoft-suppress MISRA2012-RULE-18_4-4 "Checked OK."

    for (i = 0U; i < num_entries; i++) {
        xos_log->entries[i].timestamp = 0U;
    }

    if (enable) {
        // Make sure all fields are updated before log is enabled.
        // synchronization & memory barrier: read_acquire. full system, any-any = 15
        __DSB();
        xos_log->flags = XOS_LOG_ENABLED;
    }
}


///----------------------------------------------------------------------------
///
///  Enable event logging. Assumes log has been initialized already.
///
///  No parameters.
///
///  \return Returns nothing.
///
///----------------------------------------------------------------------------
static inline void
xos_log_enable(void)
{
    xos_log->flags |= XOS_LOG_ENABLED;
}


///----------------------------------------------------------------------------
///
///  Disable event logging. It is sometimes useful to disable logging while
///  the log is being examined or dumped.
///
///  No parameters.
///
///  \return Returns nothing.
///
///----------------------------------------------------------------------------
static inline void
xos_log_disable(void)
{
    xos_log->flags &= ~XOS_LOG_ENABLED;
}


///----------------------------------------------------------------------------
///
///  Reset the event log. All log entries are abandoned and the write pointer
///  is set to the first entry location.
///
///  No parameters.
///
///  \return Returns nothing.
///
///----------------------------------------------------------------------------
static inline void
xos_log_clear(void)
{
    bool enabled = (xos_log->flags & XOS_LOG_ENABLED) != 0U;

    xos_log_disable();
    xos_log_init(xos_log, xos_log->size, enabled);
}


///----------------------------------------------------------------------------
///
///  Write an entry into the log. This function does disable all interrupts
///  since logging can be done from interrupt handlers as well. It will write
///  into the log only if the log exists and is enabled.
///
///  \param     param1              User defined value.
///
///  \param     param2              User defined value.
///
///  \param     param3              User defined value.
///
///  \return Returns nothing.
///
///----------------------------------------------------------------------------
static inline void
xos_log_write(uint32_t param1, uint32_t param2, uint32_t param3)
{
    if (xos_log != NULL) {
        XosLogEntry * next;
        // uint32_t      ps = xthal_disable_interrupts();
        lsf_task_enter_critcal();

        // Reserve an entry and advance the write pointer.
        if ((xos_log->flags & XOS_LOG_ENABLED) != 0U) {
            next = xos_log->next;

            if (xos_log->next == xos_log->last) {
                xos_log->next = xos_log->entries;
            }
            else {
                xos_log->next++;
            }

            // Don't need interrupts disabled for writing the entry.
            // xthal_restore_interrupts(ps);
            lsf_task_exit_critcal();

            next->timestamp = lsf_systick_time(); // xos_get_ccount();
            next->param1    = param1;
            next->param2    = param2;
            next->param3    = param3;
        }
        else {
            // xthal_restore_interrupts(ps);
            lsf_task_exit_critcal();
        }
    }
}


///----------------------------------------------------------------------------
///
///  Return a pointer to the first entry in the log. Call this function first
///  to get the first entry, then call xos_log_get_next() repeatedly to walk
///  through the log in sequence.
///
///  No parameters.
///
///  \return Returns a read-only pointer to the first (oldest) log entry, or
///          NULL if there is no log or the log is empty.
///
///  NOTE: The log should be traversed only when it is disabled. Attempting to
///        traverse the log while it is being written can produce unpredictable
///        behavior.
///----------------------------------------------------------------------------
static inline const XosLogEntry *
xos_log_get_first(void)
{
    XosLogEntry * entry = NULL;

    if (xos_log != NULL) {
        entry = xos_log->next;

        // 'entry' should be pointing to the next entry to be overwritten, if we
        // have wrapped. This means it is the oldest entry. However if this entry
        // has a zero timestamp then we have not wrapped, in which case we must
        // look at the first entry in the list.
        if (entry->timestamp == 0U) {
            entry = xos_log->entries;
            if (entry->timestamp == 0U) {
                entry = NULL;
            }
        }
    }

    return entry;
}


///----------------------------------------------------------------------------
///
///  Get the next sequential entry from the log. This function must be called
///  only after xos_log_get_first() has been called to begin.
///
///  \param     entry       Pointer to last entry returned by xos_log_get_first()
///                         or xos_log_get_next().
///
///  \return Returns a read-only pointer to the next log entry, or NULL
///          if no more entries.
///
///----------------------------------------------------------------------------
static inline const XosLogEntry *
xos_log_get_next(const XosLogEntry * entry)
{
    if (entry != NULL) {
        const XosLogEntry * next =
            (entry == xos_log->last) ? xos_log->entries : (entry + 1); // parasoft-suppress MISRA2012-RULE-18_4-4 "Checked OK."

        // Make sure we stop at the last (newest) entry.
        if ((next != xos_log->next) && (next->timestamp != 0U)) {
            return next;
        }
    }

    return NULL;
}


///----------------------------------------------------------------------------
///  XOS internal event codes.
///----------------------------------------------------------------------------
enum {
    XOS_SE_THREAD_CREATE = 1,
    XOS_SE_THREAD_DELETE,
    XOS_SE_THREAD_SWITCH,
    XOS_SE_THREAD_YIELD,
    XOS_SE_THREAD_BLOCK,
    XOS_SE_THREAD_WAKE,
    XOS_SE_THREAD_PRI_CHANGE,
    XOS_SE_THREAD_SUSPEND,
    XOS_SE_THREAD_ABORT,

    XOS_SE_TIMER_INTR_START,
    XOS_SE_TIMER_INTR_END,

    XOS_SE_MUTEX_LOCK,
    XOS_SE_MUTEX_UNLOCK,

    XOS_SE_SEM_GET,
    XOS_SE_SEM_PUT,

    XOS_SE_EVENT_SET,
    XOS_SE_EVENT_CLEAR,
    XOS_SE_EVENT_CLEAR_SET,
};

#ifdef __cplusplus
}
#endif

#endif // XOS_LOG_H


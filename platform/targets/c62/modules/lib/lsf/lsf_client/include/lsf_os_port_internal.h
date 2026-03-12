#ifndef LSF_CLIENT_OS_PORT_INTERNAL_H
#define LSF_CLIENT_OS_PORT_INTERNAL_H

#ifdef LSF_CLIENT_OS_PORT_PATH
    #define __LSF_TO_STR_AUX(x) #x
    #define __LSF_TO_STR(x) __LSF_TO_STR_AUX(x)
    #include __LSF_TO_STR(LSF_CLIENT_OS_PORT_PATH)
    #undef __LSF_TO_STR_AUX
    #undef __LV_TO_STR
#else
    #ifdef __has_include
        #if __has_include("lsf_os_port.h")
        #include "lsf_os_port.h"
        #endif
    #endif
#endif

// base

#ifndef lsf_base_type
typedef long lsf_base_type;
#endif

#ifndef lsf_u_base_type
typedef unsigned long lsf_u_base_type;
#endif

#ifndef lsf_max_delay
#define lsf_max_delay   ((uint32_t) -1)
#endif

// queue port

#ifndef lsf_queue_t
typedef void *lsf_queue_t;
#endif

#ifndef lsf_queue_create
#define lsf_queue_create(length, item_size) printk("lsf_queue_create not defined\n")
#endif

#ifndef lsf_queue_reset
#define lsf_queue_reset(handle) printk("lsf_queue_reset not defined\n")
#endif

#ifndef lsf_queue_receive
#define lsf_queue_receive(handle, msg, delay) printk("lsf_queue_receive not defined\n")
#endif

#ifndef lsf_queue_send
#define lsf_queue_send(handle, msg, delay) printk("lsf_queue_send not defined\n")
#endif

#ifndef lsf_queue_send_from_isr
#define lsf_queue_send_from_isr(handle, msg, delay) printk("lsf_queue_send_from_isr not defined\n")
#endif

#ifndef lsf_queue_is_full_from_isr
#define lsf_queue_is_full_from_isr(handle) printk("lsf_queue_is_full_from_isr not defined\n")
#endif

#ifndef lsf_queue_messages_waiting_from_isr
#define lsf_queue_messages_waiting_from_isr(handle) printk("lsf_queue_messages_waiting_from_isr not defined\n")
#endif

#ifndef lsf_queue_send_to_back
#define lsf_queue_send_to_back(handle, msg, delay) printk("lsf_queue_send_to_back not defined\n")
#endif

// task port

#ifndef lsf_task_create
#define lsf_task_create(function, name, stack_depth, params, priority, stack_buf, task_buf) printk("lsf_task_create not defined\n")
#endif

#ifndef lsf_task_create_static
#define lsf_task_create_static(function, name, stack_depth, params, priority, stack_buf, task_buf) printk("lsf_task_create_static not defined\n")
#endif

#ifndef lsf_task_delay
// timeout as ms
#define lsf_task_delay(timeout) printk("lsf_task_delay not defined\n")
#endif

#ifndef lsf_task_false
#define lsf_task_false			( 0 )
#endif

#ifndef lsf_task_true
#define lsf_task_true			( 1 )
#endif

#ifndef lsf_task_pass
#define lsf_task_pass			( lsf_task_true )
#endif

#ifndef lsf_task_fail
#define lsf_task_fail			( lsf_task_false )
#endif

#ifndef lsf_task_get_current_handle
#define lsf_task_get_current_handle() printk("lsf_task_get_current_handle not defined\n")
#endif

#ifndef lsf_task_enter_critcal
#define lsf_task_enter_critcal() printk("lsf_task_enter_critcal not defined\n")
#endif

#ifndef lsf_task_exit_critcal
#define lsf_task_exit_critcal() printk("lsf_task_exit_critcal not defined\n")
#endif

// time port
#ifndef lsf_systick_time
// uint32_t
#define lsf_systick_time() printk("lsf_systick_time not defined\n")
#endif

// semaphore port
#ifndef lsf_sem_t
typedef void *lsf_sem_t;
#endif

#ifndef lsf_sem_create_mutex
#define lsf_sem_create_mutex() printk("lsf_sem_create_mutex not defined\n")
#endif

#ifndef lsf_sem_create_counting
#define lsf_sem_create_counting(max_count, init_count) printk("lsf_sem_create_counting not defined\n")
#endif

#ifndef lsf_sem_take
#define lsf_sem_take(max_count, init_count) printk("lsf_sem_take not defined\n")
#endif

#ifndef lsf_sem_give
#define lsf_sem_give(handle) printk("lsf_sem_give not defined\n")
#endif

#ifndef lsf_sem_take_from_isr
#define lsf_sem_take_from_isr(handle) printk("lsf_sem_take_from_isr not defined\n")
#endif

#ifndef lsf_sem_get_count
#define lsf_sem_get_count(handle) printk("lsf_sem_get_count not defined\n")
#endif

// stack port
#ifndef lsf_stack_t
typedef uint32_t lsf_stack_t;
#endif

#ifndef lsf_static_task_t
typedef uint32_t *lsf_static_task_t;
#endif

#endif
#ifndef URPC_CLIENT_OS_PORT_INTERNAL_H
#define URPC_CLIENT_OS_PORT_INTERNAL_H

#ifdef URPC_CLIENT_OS_PORT_PATH
    #define __URPC_TO_STR_AUX(x) #x
    #define __URPC_TO_STR(x) __URPC_TO_STR_AUX(x)
    #include __URPC_TO_STR(URPC_CLIENT_OS_PORT_PATH)
    #undef __URPC_TO_STR_AUX
    #undef __LV_TO_STR
#else
    #ifdef __has_include
        #if __has_include("urpc_os_port.h")
        #include "urpc_os_port.h"
        #endif
    #endif
#endif

// base

#ifndef urpc_base_type
typedef long urpc_base_type;
#endif

#ifndef urpc_max_delay
#define urpc_max_delay   ((uint32_t) -1)
#endif

// queue port

#ifndef urpc_queue_t
typedef void *urpc_queue_t;
#endif

#ifndef urpc_queue_create
#define urpc_queue_create(length, item_size) printk("urpc_queue_create not defined\n")
#endif

#ifndef urpc_queue_reset
#define urpc_queue_reset(handle) printk("urpc_queue_reset not defined\n")
#endif

#ifndef urpc_queue_receive
#define urpc_queue_receive(handle, msg, delay) printk("urpc_queue_receive not defined\n")
#endif

#ifndef urpc_queue_send
#define urpc_queue_send(handle, msg, delay) printk("urpc_queue_send not defined\n")
#endif

#ifndef urpc_queue_send_from_isr
#define urpc_queue_send_from_isr(handle, msg, delay) printk("urpc_queue_send_from_isr not defined\n")
#endif

#ifndef urpc_queue_is_full_from_isr
#define urpc_queue_is_full_from_isr(handle) printk("urpc_queue_is_full_from_isr not defined\n")
#endif

#ifndef urpc_queue_messages_waiting_from_isr
#define urpc_queue_messages_waiting_from_isr(handle) printk("urpc_queue_messages_waiting_from_isr not defined\n")
#endif

#ifndef urpc_queue_send_to_back
#define urpc_queue_send_to_back(handle, msg, delay) printk("urpc_queue_send_to_back not defined\n")
#endif

// task port

#ifndef urpc_task_create_static
#define urpc_task_create_static(function, name, stack_depth, params, priority, stack_buf, task_buf) printk("urpc_task_create_static not defined\n")
#endif

#ifndef urpc_task_delay
// timeout as ms
#define urpc_task_delay(timeout) printk("urpc_task_delay not defined\n")
#endif

#ifndef urpc_task_false
#define urpc_task_false			( 0 )
#endif

#ifndef urpc_task_true
#define urpc_task_true			( 1 )
#endif

#ifndef urpc_task_pass
#define urpc_task_pass			( urpc_task_true )
#endif

#ifndef urpc_task_fail
#define urpc_task_fail			( urpc_task_false )
#endif

#ifndef urpc_task_get_current_handle
#define urpc_task_get_current_handle() printk("urpc_task_get_current_handle not defined\n")
#endif

#ifndef urpc_task_enter_critcal
#define urpc_task_enter_critcal() printk("urpc_task_enter_critcal not defined\n")
#endif

#ifndef urpc_task_exit_critcal
#define urpc_task_exit_critcal() printk("urpc_task_exit_critcal not defined\n")
#endif

// time port
#ifndef urpc_systick_time
// uint32_t
#define urpc_systick_time() printk("urpc_systick_time not defined\n")
#endif

// semaphore port
#ifndef urpc_sem_t
typedef void *urpc_sem_t;
#endif

#ifndef urpc_sem_create_mutex
#define urpc_sem_create_mutex() printk("urpc_sem_create_mutex not defined\n")
#endif

#ifndef urpc_sem_create_counting
#define urpc_sem_create_counting(max_count, init_count) printk("urpc_sem_create_counting not defined\n")
#endif

#ifndef urpc_sem_take
#define urpc_sem_take(max_count, init_count) printk("urpc_sem_take not defined\n")
#endif

#ifndef urpc_sem_give
#define urpc_sem_give(handle) printk("urpc_sem_give not defined\n")
#endif

#ifndef urpc_sem_take_from_isr
#define urpc_sem_take_from_isr(handle) printk("urpc_sem_take_from_isr not defined\n")
#endif

#ifndef urpc_sem_give_from_isr
#define urpc_sem_give_from_isr(handle) printk("urpc_sem_give_from_isr not defined\n")
#endif

#ifndef urpc_sem_get_count
#define urpc_sem_get_count(handle) printk("urpc_sem_get_count not defined\n")
#endif

// stack port
#ifndef urpc_stack_t
typedef uint32_t urpc_stack_t;
#endif

#ifndef urpc_static_task_t
typedef uint32_t *urpc_static_task_t;
#endif

#endif
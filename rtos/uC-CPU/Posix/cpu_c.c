/*
*********************************************************************************************************
*                                               uC/CPU
*                                    CPU CONFIGURATION & PORT LAYER
*
*                    Copyright 2004-2020 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            CPU PORT FILE
*
*                                                POSIX
*
* Filename : cpu_c.c
* Version  : v1.32.00
*********************************************************************************************************
* Notes    : (1) Requires a Single UNIX Specification, Version 3 compliant operating environment.
*                On Linux _XOPEN_SOURCE must be defined to at least 600, generally by passing the
*                -D_XOPEN_SOURCE=600 command line option to GCC.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <stdio.h>
#include  <pthread.h>
#include  <stdlib.h>
#include  <string.h>
#include  <signal.h>
#include  <unistd.h>
#include  <stdlib.h>
#include  <sys/types.h>
#include  <sys/syscall.h>
#include  <sys/resource.h>
#include  <errno.h>

#include  <cpu.h>
#include  <cpu_core.h>

#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
#include  <time.h>
#endif

#ifdef __cplusplus
extern  "C" {
#endif


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define  CPU_TMR_INT_TASK_PRIO         sched_get_priority_max(SCHED_RR)     /* Tmr interrupt task priority.             */
#define  CPU_IRQ_SIG                  (SIGURG)                              /* IRQ trigger signal.                      */

/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/

typedef  struct  cpu_interrupt_node  CPU_INTERRUPT_NODE;

struct  cpu_interrupt_node {
    CPU_INTERRUPT       *InterruptPtr;
    CPU_INTERRUPT_NODE  *NextPtr;
};


/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/

static  pthread_mutex_t       CPU_InterruptQueueMutex = PTHREAD_MUTEX_INITIALIZER;
static  pthread_mutexattr_t   CPU_InterruptQueueMutexAttr;

static  CPU_INTERRUPT_NODE   *CPU_InterruptPendListHeadPtr;
static  CPU_INTERRUPT_NODE   *CPU_InterruptRunningListHeadPtr;

static  sigset_t              CPU_IRQ_SigMask;

/*
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

#if  (_POSIX_C_SOURCE < 199309L)
#error  "_POSIX_C_SOURCE is required to be at least 199309L"
#endif

static  void   CPU_IRQ_Handler       (int  sig);

static  void   CPU_InterruptTriggerInternal (CPU_INTERRUPT  *p_interrupt);

static  void   CPU_InterruptQueue    (CPU_INTERRUPT  *p_isr);

static  void  *CPU_TmrInterruptTask  (void  *p_arg);

static  void   CPU_ISR_Sched         (void);


/*
*********************************************************************************************************
*                                            CPU_IntInit()
*
* Description : This function initializes the critical section.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : 1) CPU_IntInit() MUST be called prior to use any of the CPU_IntEn(), and CPU_IntDis()
*                  functions.
*********************************************************************************************************
*/

void  CPU_IntInit (void)
{
    struct  sigaction  on_isr_trigger_sig_action;
    int     res;


    CPU_InterruptPendListHeadPtr    = DEF_NULL;
    CPU_InterruptRunningListHeadPtr = DEF_NULL;

    sigemptyset(&CPU_IRQ_SigMask);
    sigaddset(&CPU_IRQ_SigMask, CPU_IRQ_SIG);;

    pthread_mutexattr_init(&CPU_InterruptQueueMutexAttr);
    pthread_mutexattr_settype(&CPU_InterruptQueueMutexAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&CPU_InterruptQueueMutex, &CPU_InterruptQueueMutexAttr);

                                                                /* Register interrupt trigger signal handler.           */
    memset(&on_isr_trigger_sig_action, 0, sizeof(on_isr_trigger_sig_action));
    res = sigemptyset(&on_isr_trigger_sig_action.sa_mask);
    if (res != 0u) {
        raise(SIGABRT);
    }
    on_isr_trigger_sig_action.sa_flags = SA_NODEFER;
    on_isr_trigger_sig_action.sa_handler = CPU_IRQ_Handler;
    res = sigaction(CPU_IRQ_SIG, &on_isr_trigger_sig_action, NULL);
    if (res != 0u) {
        raise(SIGABRT);
    }
}


/*
*********************************************************************************************************
*                                            CPU_IntDis()
*
* Description : This function disables interrupts for critical sections of code.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  CPU_IntDis (void)
{
    pthread_sigmask(SIG_BLOCK, &CPU_IRQ_SigMask, DEF_NULL);
}


/*
*********************************************************************************************************
*                                             CPU_IntEn()
*
* Description : This function enables interrupts after critical sections of code.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  CPU_IntEn (void)
{
    pthread_sigmask(SIG_UNBLOCK, &CPU_IRQ_SigMask, DEF_NULL);
}


/*
*********************************************************************************************************
*                                             CPU_ISR_End()
*
* Description : Ends an ISR.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : (1) This function MUST be called at the end of an ISR.
*
*********************************************************************************************************
*/

void  CPU_ISR_End (void)
{
    CPU_INTERRUPT_NODE  *p_interrupt_node;


    CPU_INT_DIS();
    pthread_mutex_lock(&CPU_InterruptQueueMutex);
    if (CPU_InterruptRunningListHeadPtr == DEF_NULL) {
        raise(SIGABRT);
    }
    p_interrupt_node                = CPU_InterruptRunningListHeadPtr;
    CPU_InterruptRunningListHeadPtr = CPU_InterruptRunningListHeadPtr->NextPtr;
    pthread_mutex_unlock(&CPU_InterruptQueueMutex);
    CPU_INT_EN();

    free(p_interrupt_node);

    CPU_ISR_Sched();
}


/*
*********************************************************************************************************
*                                       CPU_TmrInterruptCreate()
*
* Description : Simulated hardware timer instance creation.
*
* Argument(s) : p_tmr_interrupt     Pointer to a timer interrupt descriptor.
*
* Return(s)   : none.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

void  CPU_TmrInterruptCreate (CPU_TMR_INTERRUPT  *p_tmr_interrupt)
{
    pthread_t            thread;
    pthread_attr_t       attr;
    struct  sched_param  param;
    int                  res;


    res = pthread_attr_init(&attr);
    if (res != 0u) {
        raise(SIGABRT);
    }
    res = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (res != 0u) {
        raise(SIGABRT);
    }
    param.__sched_priority = CPU_TMR_INT_TASK_PRIO;
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    if (res != 0u) {
        raise(SIGABRT);
    }
    pthread_attr_setschedparam(&attr, &param);
    if (res != 0u) {
        raise(SIGABRT);
    }

    pthread_create(&thread, &attr, CPU_TmrInterruptTask, p_tmr_interrupt);
}


/*
*********************************************************************************************************
*                                        CPU_InterruptTrigger()
*
* Description : Queue an interrupt and send the IRQ signal.
*
* Argument(s) : p_interrupt     Interrupt to be queued.
*
* Return(s)   : none.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

void  CPU_InterruptTrigger (CPU_INTERRUPT  *p_interrupt)
{
    CPU_INT_DIS();
    CPU_InterruptTriggerInternal(p_interrupt);                  /* Signal are now blocked: rest of Trigger is internal. */
    CPU_INT_EN();
}


/*
*********************************************************************************************************
*                                            CPU_Printf()
*
* Description: This function is analog of printf.
*
* Arguments  : p_str        Pointer to format string output.
*
* Returns    : Number of characters written.
*********************************************************************************************************
*/

#ifdef  CPU_CFG_MSG_TRACE_EN
static  int  CPU_Printf (char  *p_str, ...)
{

}
#endif


/*
*********************************************************************************************************
*                                         CPU_CntLeadZeros()
*
* Description : Count the number of contiguous, most-significant, leading zero bits in a data value.
*
* Argument(s) : val         Data value to count leading zero bits.
*
* Return(s)   : Number of contiguous, most-significant, leading zero bits in 'val', if NO error(s).
*
*               0,                                                                  otherwise.
*
* Note(s)     : (1) (a) Supports the following data value sizes :
*
*                       (1)  8-bits
*                       (2) 16-bits
*                       (3) 32-bits
*
*                       See also 'cpu_def.h  CPU WORD CONFIGURATION  Note #1'.
*
*                   (b) (1) For  8-bit values :
*
*                                  b07  b06  b05  b04  b03  b02  b01  b00    # Leading Zeros
*                                  ---  ---  ---  ---  ---  ---  ---  ---    ---------------
*                                   1    x    x    x    x    x    x    x            0
*                                   0    1    x    x    x    x    x    x            1
*                                   0    0    1    x    x    x    x    x            2
*                                   0    0    0    1    x    x    x    x            3
*                                   0    0    0    0    1    x    x    x            4
*                                   0    0    0    0    0    1    x    x            5
*                                   0    0    0    0    0    0    1    x            6
*                                   0    0    0    0    0    0    0    1            7
*                                   0    0    0    0    0    0    0    0            8
*
*
*                       (2) For 16-bit values :
*
*                             b15  b14  b13  ...  b04  b03  b02  b01  b00    # Leading Zeros
*                             ---  ---  ---       ---  ---  ---  ---  ---    ---------------
*                              1    x    x         x    x    x    x    x            0
*                              0    1    x         x    x    x    x    x            1
*                              0    0    1         x    x    x    x    x            2
*                              :    :    :         :    :    :    :    :            :
*                              :    :    :         :    :    :    :    :            :
*                              0    0    0         1    x    x    x    x           11
*                              0    0    0         0    1    x    x    x           12
*                              0    0    0         0    0    1    x    x           13
*                              0    0    0         0    0    0    1    x           14
*                              0    0    0         0    0    0    0    1           15
*                              0    0    0         0    0    0    0    0           16
*
*
*                       (3) For 32-bit values :
*
*                             b31  b30  b29  ...  b04  b03  b02  b01  b00    # Leading Zeros
*                             ---  ---  ---       ---  ---  ---  ---  ---    ---------------
*                              1    x    x         x    x    x    x    x            0
*                              0    1    x         x    x    x    x    x            1
*                              0    0    1         x    x    x    x    x            2
*                              :    :    :         :    :    :    :    :            :
*                              :    :    :         :    :    :    :    :            :
*                              0    0    0         1    x    x    x    x           27
*                              0    0    0         0    1    x    x    x           28
*                              0    0    0         0    0    1    x    x           29
*                              0    0    0         0    0    0    1    x           30
*                              0    0    0         0    0    0    0    1           31
*                              0    0    0         0    0    0    0    0           32
*
*
*                       See also 'CPU COUNT LEAD ZEROs LOOKUP TABLE  Note #1'.
*
*               (2) MUST be implemented in cpu_a.asm if and only if CPU_CFG_LEAD_ZEROS_ASM_PRESENT
*                   is #define'd in 'cpu_cfg.h' or 'cpu.h'.
*********************************************************************************************************
*/

#ifdef  CPU_CFG_LEAD_ZEROS_ASM_PRESENT
CPU_DATA  CPU_CntLeadZeros (CPU_DATA  val)
{

}
#endif


/*
*********************************************************************************************************
*                                         CPU_CntTrailZeros()
*
* Description : Count the number of contiguous, least-significant, trailing zero bits in a data value.
*
* Argument(s) : val         Data value to count trailing zero bits.
*
* Return(s)   : Number of contiguous, least-significant, trailing zero bits in 'val'.
*
* Note(s)     : (1) (a) Supports the following data value sizes :
*
*                       (1)  8-bits
*                       (2) 16-bits
*                       (3) 32-bits
*                       (4) 64-bits
*
*                       See also 'cpu_def.h  CPU WORD CONFIGURATION  Note #1'.
*
*                   (b) (1) For  8-bit values :
*
*                                  b07  b06  b05  b04  b03  b02  b01  b00    # Trailing Zeros
*                                  ---  ---  ---  ---  ---  ---  ---  ---    ----------------
*                                   x    x    x    x    x    x    x    1            0
*                                   x    x    x    x    x    x    1    0            1
*                                   x    x    x    x    x    1    0    0            2
*                                   x    x    x    x    1    0    0    0            3
*                                   x    x    x    1    0    0    0    0            4
*                                   x    x    1    0    0    0    0    0            5
*                                   x    1    0    0    0    0    0    0            6
*                                   1    0    0    0    0    0    0    0            7
*                                   0    0    0    0    0    0    0    0            8
*
*
*                       (2) For 16-bit values :
*
*                             b15  b14  b13  b12  b11  ...  b02  b01  b00    # Trailing Zeros
*                             ---  ---  ---  ---  ---       ---  ---  ---    ----------------
*                              x    x    x    x    x         x    x    1            0
*                              x    x    x    x    x         x    1    0            1
*                              x    x    x    x    x         1    0    0            2
*                              :    :    :    :    :         :    :    :            :
*                              :    :    :    :    :         :    :    :            :
*                              x    x    x    x    1         0    0    0           11
*                              x    x    x    1    0         0    0    0           12
*                              x    x    1    0    0         0    0    0           13
*                              x    1    0    0    0         0    0    0           14
*                              1    0    0    0    0         0    0    0           15
*                              0    0    0    0    0         0    0    0           16
*
*
*                       (3) For 32-bit values :
*
*                             b31  b30  b29  b28  b27  ...  b02  b01  b00    # Trailing Zeros
*                             ---  ---  ---  ---  ---       ---  ---  ---    ----------------
*                              x    x    x    x    x         x    x    1            0
*                              x    x    x    x    x         x    1    0            1
*                              x    x    x    x    x         1    0    0            2
*                              :    :    :    :    :         :    :    :            :
*                              :    :    :    :    :         :    :    :            :
*                              x    x    x    x    1         0    0    0           27
*                              x    x    x    1    0         0    0    0           28
*                              x    x    1    0    0         0    0    0           29
*                              x    1    0    0    0         0    0    0           30
*                              1    0    0    0    0         0    0    0           31
*                              0    0    0    0    0         0    0    0           32
*
*
*                       (4) For 64-bit values :
*
*                             b63  b62  b61  b60  b59  ...  b02  b01  b00    # Trailing Zeros
*                             ---  ---  ---  ---  ---       ---  ---  ---    ----------------
*                              x    x    x    x    x         x    x    1            0
*                              x    x    x    x    x         x    1    0            1
*                              x    x    x    x    x         1    0    0            2
*                              :    :    :    :    :         :    :    :            :
*                              :    :    :    :    :         :    :    :            :
*                              x    x    x    x    1         0    0    0           59
*                              x    x    x    1    0         0    0    0           60
*                              x    x    1    0    0         0    0    0           61
*                              x    1    0    0    0         0    0    0           62
*                              1    0    0    0    0         0    0    0           63
*                              0    0    0    0    0         0    0    0           64
*
*               (2) For non-zero values, the returned number of contiguous, least-significant, trailing
*                   zero bits is also equivalent to the bit position of the least-significant set bit.
*
*               (3) 'val' SHOULD be validated for non-'0' PRIOR to all other counting zero calculations :
*
*                   (a) CPU_CntTrailZeros()'s final conditional statement calculates 'val's number of
*                       trailing zeros based on its return data size, 'CPU_CFG_DATA_SIZE', & 'val's
*                       calculated number of lead zeros ONLY if the initial 'val' is non-'0' :
*
*                           if (val != 0u) {
*                               nbr_trail_zeros = ((CPU_CFG_DATA_SIZE * DEF_OCTET_NBR_BITS) - 1u) - nbr_lead_zeros;
*                           } else {
*                               nbr_trail_zeros = nbr_lead_zeros;
*                           }
*
*                       Therefore, initially validating all non-'0' values avoids having to conditionally
*                       execute the final statement.
*********************************************************************************************************
*/

#ifdef  CPU_CFG_TRAIL_ZEROS_ASM_PRESENT
CPU_DATA  CPU_CntTrailZeros (CPU_DATA  val)
{

}
#endif

/*
*********************************************************************************************************
*                                          CPU_TS_TmrInit()
*
* Description : Initialize & start CPU timestamp timer.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : (1) CPU_TS_TmrInit() is an application/BSP function that MUST be defined by the developer
*                   if either of the following CPU features is enabled :
*
*                   (a) CPU timestamps
*                   (b) CPU interrupts disabled time measurements
*
*                   See 'cpu_cfg.h  CPU TIMESTAMP CONFIGURATION  Note #1'
*                     & 'cpu_cfg.h  CPU INTERRUPTS DISABLED TIME MEASUREMENT CONFIGURATION  Note #1a'.
*
*               (2) (a) Timer count values MUST be returned via word-size-configurable 'CPU_TS_TMR'
*                       data type.
*
*                       (1) If timer has more bits, truncate timer values' higher-order bits greater
*                           than the configured 'CPU_TS_TMR' timestamp timer data type word size.
*
*                       (2) Since the timer MUST NOT have less bits than the configured 'CPU_TS_TMR'
*                           timestamp timer data type word size; 'CPU_CFG_TS_TMR_SIZE' MUST be
*                           configured so that ALL bits in 'CPU_TS_TMR' data type are significant.
*
*                           In other words, if timer size is not a binary-multiple of 8-bit octets
*                           (e.g. 20-bits or even 24-bits), then the next lower, binary-multiple
*                           octet word size SHOULD be configured (e.g. to 16-bits).  However, the
*                           minimum supported word size for CPU timestamp timers is 8-bits.
*
*                       See also 'cpu_cfg.h   CPU TIMESTAMP CONFIGURATION  Note #2'
*                              & 'cpu_core.h  CPU TIMESTAMP DATA TYPES     Note #1'.
*
*                   (b) Timer SHOULD be an 'up'  counter whose values increase with each time count.
*
*                   (c) When applicable, timer period SHOULD be less than the typical measured time
*                       but MUST be less than the maximum measured time; otherwise, timer resolution
*                       inadequate to measure desired times.
*
*                   See also 'CPU_TS_TmrRd()  Note #2'.
*********************************************************************************************************
*/

#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
void  CPU_TS_TmrInit (void)
{
    struct  timespec   res;


    res.tv_sec  = 0;
    res.tv_nsec = 0;

	(void)clock_settime(CLOCK_MONOTONIC, &res);

    CPU_TS_TmrFreqSet(1000000000);
}
#endif


/*
*********************************************************************************************************
*                                           CPU_TS_TmrRd()
*
* Description : Get current CPU timestamp timer count value.
*
* Argument(s) : none.
*
* Return(s)   : Timestamp timer count (see Notes #2a & #2b).
*
* Note(s)     : (1) CPU_TS_TmrRd() is an application/BSP function that MUST be defined by the developer
*                   if either of the following CPU features is enabled :
*
*                   (a) CPU timestamps
*                   (b) CPU interrupts disabled time measurements
*
*                   See 'cpu_cfg.h  CPU TIMESTAMP CONFIGURATION  Note #1'
*                     & 'cpu_cfg.h  CPU INTERRUPTS DISABLED TIME MEASUREMENT CONFIGURATION  Note #1a'.
*
*               (2) (a) Timer count values MUST be returned via word-size-configurable 'CPU_TS_TMR'
*                       data type.
*
*                       (1) If timer has more bits, truncate timer values' higher-order bits greater
*                           than the configured 'CPU_TS_TMR' timestamp timer data type word size.
*
*                       (2) Since the timer MUST NOT have less bits than the configured 'CPU_TS_TMR'
*                           timestamp timer data type word size; 'CPU_CFG_TS_TMR_SIZE' MUST be
*                           configured so that ALL bits in 'CPU_TS_TMR' data type are significant.
*
*                           In other words, if timer size is not a binary-multiple of 8-bit octets
*                           (e.g. 20-bits or even 24-bits), then the next lower, binary-multiple
*                           octet word size SHOULD be configured (e.g. to 16-bits).  However, the
*                           minimum supported word size for CPU timestamp timers is 8-bits.
*
*                       See also 'cpu_cfg.h   CPU TIMESTAMP CONFIGURATION  Note #2'
*                              & 'cpu_core.h  CPU TIMESTAMP DATA TYPES     Note #1'.
*
*                   (b) Timer SHOULD be an 'up'  counter whose values increase with each time count.
*
*                       (1) If timer is a 'down' counter whose values decrease with each time count,
*                           then the returned timer value MUST be ones-complemented.
*
*                   (c) (1) When applicable, the amount of time measured by CPU timestamps is
*                           calculated by either of the following equations :
*
*                           (A) Time measured  =  Number timer counts  *  Timer period
*
*                                   where
*
*                                       Number timer counts     Number of timer counts measured
*                                       Timer period            Timer's period in some units of
*                                                                   (fractional) seconds
*                                       Time measured           Amount of time measured, in same
*                                                                   units of (fractional) seconds
*                                                                   as the Timer period
*
*                                                  Number timer counts
*                           (B) Time measured  =  ---------------------
*                                                    Timer frequency
*
*                                   where
*
*                                       Number timer counts     Number of timer counts measured
*                                       Timer frequency         Timer's frequency in some units
*                                                                   of counts per second
*                                       Time measured           Amount of time measured, in seconds
*
*                       (2) Timer period SHOULD be less than the typical measured time but MUST be less
*                           than the maximum measured time; otherwise, timer resolution inadequate to
*                           measure desired times.
*********************************************************************************************************
*/

#if (CPU_CFG_TS_TMR_EN == DEF_ENABLED)
CPU_TS_TMR  CPU_TS_TmrRd (void)
{
    struct  timespec    res;
            CPU_TS_TMR  ts;


    (void)clock_gettime(CLOCK_MONOTONIC, &res);

    ts = (CPU_TS_TMR)(res.tv_sec * 1000000000u + res.tv_nsec);

    return (ts);
}
#endif


#ifdef __cplusplus
}
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          CPU_IRQ_Handler()
*
* Description : CPU_IRQ_SIG signal handler.
*
* Argument(s) : sig     Signal that triggered the handler (unused).
*
* Return(s)   : none.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  void  CPU_IRQ_Handler (int sig)
{
    (void)&sig;
    CPU_ISR_Sched();
}


/*
*********************************************************************************************************
*                                    CPU_InterruptTriggerInternal()
*
* Description : Queue an interrupt and send the IRQ signal.
*
* Argument(s) : p_interrupt     Interrupt to be queued.
*
* Return(s)   : none.
*
* Note(s)     : (1) The Interrupt signal must be blocked before calling this function.
*********************************************************************************************************
*/

void  CPU_InterruptTriggerInternal (CPU_INTERRUPT  *p_interrupt)
{
    if (p_interrupt->En == DEF_NO) {
        return;
    }

    CPU_InterruptQueue(p_interrupt);

    kill(getpid(), CPU_IRQ_SIG);

    if (p_interrupt->TraceEn == DEF_ENABLED) {
        struct  timespec  ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        printf("@ %lu:%06lu", ts.tv_sec, ts.tv_nsec / 1000u);
        printf("  %s interrupt fired.\r\n", p_interrupt->NamePtr);
    }
}


/*
*********************************************************************************************************
*                                         CPU_InterruptQueue()
*
* Description : Queue an interrupt.
*
* Argument(s) : p_interrupt     Pointer to the interrupt to be queued.
*
* Return(s)   : none.
*
* Note(s)     : (1) The signals for this thread are already blocked during this function call.
*
*               (2) Since the signal are already blocked, it is safe to lock and release the mutex.
*********************************************************************************************************
*/

static  void  CPU_InterruptQueue (CPU_INTERRUPT  *p_interrupt)
{
    CPU_INTERRUPT_NODE  *p_cur_interrupt_node;
    CPU_INTERRUPT_NODE  *p_prev_interrupt_node;
    CPU_INTERRUPT_NODE  *p_interrupt_node;


    p_interrupt_node = (CPU_INTERRUPT_NODE *)malloc(sizeof(CPU_INTERRUPT_NODE));
    p_interrupt_node->InterruptPtr = p_interrupt;

    pthread_mutex_lock(&CPU_InterruptQueueMutex);
    if ((CPU_InterruptPendListHeadPtr == DEF_NULL) ||
        (CPU_InterruptPendListHeadPtr->InterruptPtr->Prio < p_interrupt->Prio)) {
        p_interrupt_node->NextPtr = CPU_InterruptPendListHeadPtr;
        CPU_InterruptPendListHeadPtr  = p_interrupt_node;
    } else {
        p_cur_interrupt_node = CPU_InterruptPendListHeadPtr;
        while ((p_cur_interrupt_node != DEF_NULL) &&
               (p_cur_interrupt_node->InterruptPtr->Prio >= p_interrupt->Prio)) {
            p_prev_interrupt_node = p_cur_interrupt_node;
            p_cur_interrupt_node  = p_cur_interrupt_node->NextPtr;
        }
        p_prev_interrupt_node->NextPtr = p_interrupt_node;
        p_interrupt_node->NextPtr      = p_cur_interrupt_node;
    }
    pthread_mutex_unlock(&CPU_InterruptQueueMutex);
}


/*
*********************************************************************************************************
*                                            CPU_ISR_Sched()
*
* Description : Schedules the highest priority pending interrupt.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  void  CPU_ISR_Sched (void)
{
    CPU_INTERRUPT_NODE     *p_isr_node;


    CPU_INT_DIS();
    pthread_mutex_lock(&CPU_InterruptQueueMutex);
    p_isr_node = CPU_InterruptPendListHeadPtr;
    if ((p_isr_node != DEF_NULL) &&
        ((CPU_InterruptRunningListHeadPtr == DEF_NULL) ||
        (p_isr_node->InterruptPtr->Prio > CPU_InterruptRunningListHeadPtr->InterruptPtr->Prio))) {
        CPU_InterruptPendListHeadPtr = CPU_InterruptPendListHeadPtr->NextPtr;
        p_isr_node->NextPtr = CPU_InterruptRunningListHeadPtr;
        CPU_InterruptRunningListHeadPtr = p_isr_node;
        pthread_mutex_unlock(&CPU_InterruptQueueMutex);
        CPU_INT_EN();
        p_isr_node->InterruptPtr->ISR_Fnct();
    } else {
        pthread_mutex_unlock(&CPU_InterruptQueueMutex);
        CPU_INT_EN();
    }
}


/*
*********************************************************************************************************
*                                        CPU_TmrInterruptTask()
*
* Description : Hardware timer interrupt simulation function.
*
* Argument(s) : p_arg    Pointer to a timer interrupt descriptor.
*
* Return(s)   : none.
*
* Note(s)     : none.
*
*********************************************************************************************************
*/

static  void  *CPU_TmrInterruptTask (void  *p_arg) {

    struct  timespec    tspec, tspec_rem;
    int                 res;
    CPU_TMR_INTERRUPT  *p_tmr_int;
    CPU_BOOLEAN         one_shot;

    CPU_INT_DIS();

    p_tmr_int = (CPU_TMR_INTERRUPT *)p_arg;

    tspec.tv_nsec = p_tmr_int->PeriodMuSec * 1000u;
    tspec.tv_sec  = p_tmr_int->PeriodSec;

    one_shot = p_tmr_int->OneShot;

    do {
        tspec_rem = tspec;
        do {res = clock_nanosleep(CLOCK_MONOTONIC, 0u, &tspec_rem, &tspec_rem); } while (res == EINTR);
        if (res != 0u) { raise(SIGABRT); }
        CPU_InterruptTriggerInternal(&(p_tmr_int->Interrupt));  /* See Note #2.                                         */
    } while (one_shot != DEF_YES);

    pthread_exit(DEF_NULL);

    return (NULL);
}

@********************************************************************************************************
@                                               uC/CPU
@                                    CPU CONFIGURATION & PORT LAYER
@
@                    Copyright 2004-2020 Silicon Laboratories Inc. www.silabs.com
@
@                                 SPDX-License-Identifier: APACHE-2.0
@
@               This software is subject to an open source license and is distributed by
@                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
@                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
@
@********************************************************************************************************

@********************************************************************************************************
@
@                                            CPU PORT FILE
@
@                                               ARMv7-M
@                                            GNU C Compiler
@
@ Filename : cpu_a.s
@ Version  : v1.32.00
@********************************************************************************************************
@ Note(s)  : This port supports the ARM Cortex-M3, Cortex-M4 and Cortex-M7 architectures.
@********************************************************************************************************


@********************************************************************************************************
@                                           PUBLIC FUNCTIONS
@********************************************************************************************************

        .global  CPU_IntDis
        .global  CPU_IntEn

        .global  CPU_SR_Save
        .global  CPU_SR_Restore

        .global  CPU_WaitForInt
        .global  CPU_WaitForExcept


        .global  CPU_CntLeadZeros
        .global  CPU_CntTrailZeros
        .global  CPU_RevBits


@********************************************************************************************************
@                                      CODE GENERATION DIRECTIVES
@********************************************************************************************************

.text
.align 2
.syntax unified


@********************************************************************************************************
@                                    DISABLE and ENABLE INTERRUPTS
@
@ Description : Disable/Enable interrupts.
@
@ Prototypes  : void  CPU_IntDis(void);
@               void  CPU_IntEn (void);
@********************************************************************************************************

.thumb_func
CPU_IntDis:
        CPSID   I
        BX      LR

.thumb_func
CPU_IntEn:
        CPSIE   I
        BX      LR


@********************************************************************************************************
@                                      CRITICAL SECTION FUNCTIONS
@
@ Description : Disable/Enable Kernel aware interrupts by preserving the state of BASEPRI.  Generally speaking,
@               the state of the BASEPRI interrupt exception processing is stored in the local variable
@               'cpu_sr' & Kernel Aware interrupts are then disabled ('cpu_sr' is allocated in all functions
@               that need to disable Kernel aware interrupts). The previous BASEPRI interrupt state is restored
@               by copying 'cpu_sr' into the BASEPRI register.
@
@ Prototypes  : CPU_SR  CPU_SR_Save   (CPU_SR  new_basepri);
@               void    CPU_SR_Restore(CPU_SR  cpu_sr);
@
@ Note(s)     : (1) These functions are used in general like this :
@
@                       void  Task (void  *p_arg)
@                       {
@                           CPU_SR_ALLOC();                     /* Allocate storage for CPU status register */
@                               :
@                               :
@                           CPU_CRITICAL_ENTER();               /* cpu_sr = CPU_SR_Save();                  */
@                               :
@                               :
@                           CPU_CRITICAL_EXIT();                /* CPU_SR_Restore(cpu_sr);                  */
@                               :
@                       }
@
@               (2) Increasing priority using a write to BASEPRI does not take effect immediately.
@                   (a) IMPLICATION  This erratum means that the instruction after an MSR to boost BASEPRI
@                       might incorrectly be preempted by an insufficient high priority exception.
@
@                   (b) WORKAROUND  The MSR to boost BASEPRI can be replaced by the following code sequence:
@
@                       CPSID i
@                       MSR to BASEPRI
@                       DSB
@                       ISB
@                       CPSIE i
@********************************************************************************************************

.thumb_func
CPU_SR_Save:
        CPSID   I                               @ Cortex-M7 errata notice. See Note #2
        PUSH   {R1}
        MRS     R1, BASEPRI
        MSR     BASEPRI, R0
        DSB
        ISB
        MOV     R0, R1
        POP    {R1}
        CPSIE   I
        BX      LR


.thumb_func
CPU_SR_Restore:
        CPSID   I                               @ Cortex-M7 errata notice. See Note #2
        MSR     BASEPRI, R0
        DSB
        ISB
        CPSIE   I
        BX      LR


@********************************************************************************************************
@                                         WAIT FOR INTERRUPT
@
@ Description : Enters sleep state, which will be exited when an interrupt is received.
@
@ Prototypes  : void  CPU_WaitForInt (void)
@
@ Argument(s) : none.
@********************************************************************************************************

.thumb_func
CPU_WaitForInt:
        WFI                                     @ Wait for interrupt
        BX      LR


@********************************************************************************************************
@                                         WAIT FOR EXCEPTION
@
@ Description : Enters sleep state, which will be exited when an exception is received.
@
@ Prototypes  : void  CPU_WaitForExcept (void)
@
@ Argument(s) : none.
@********************************************************************************************************

.thumb_func
CPU_WaitForExcept:
        WFE                                     @ Wait for exception
        BX      LR


@********************************************************************************************************
@                                         CPU_CntLeadZeros()
@                                        COUNT LEADING ZEROS
@
@ Description : Counts the number of contiguous, most-significant, leading zero bits before the
@                   first binary one bit in a data value.
@
@ Prototype   : CPU_DATA  CPU_CntLeadZeros(CPU_DATA  val);
@
@ Argument(s) : val         Data value to count leading zero bits.
@
@ Return(s)   : Number of contiguous, most-significant, leading zero bits in 'val'.
@
@ Note(s)     : (1) (a) Supports 32-bit data value size as configured by 'CPU_DATA' (see 'cpu.h
@                       CPU WORD CONFIGURATION  Note #1').
@
@                   (b) For 32-bit values :
@
@                             b31  b30  b29  ...  b04  b03  b02  b01  b00    # Leading Zeros
@                             ---  ---  ---       ---  ---  ---  ---  ---    ---------------
@                              1    x    x         x    x    x    x    x            0
@                              0    1    x         x    x    x    x    x            1
@                              0    0    1         x    x    x    x    x            2
@                              :    :    :         :    :    :    :    :            :
@                              :    :    :         :    :    :    :    :            :
@                              0    0    0         1    x    x    x    x           27
@                              0    0    0         0    1    x    x    x           28
@                              0    0    0         0    0    1    x    x           29
@                              0    0    0         0    0    0    1    x           30
@                              0    0    0         0    0    0    0    1           31
@                              0    0    0         0    0    0    0    0           32
@
@
@               (2) MUST be defined in 'cpu_a.asm' (or 'cpu_c.c') if CPU_CFG_LEAD_ZEROS_ASM_PRESENT is
@                   #define'd in 'cpu_cfg.h' or 'cpu.h'.
@********************************************************************************************************

.thumb_func
CPU_CntLeadZeros:
        CLZ     R0, R0                          @ Count leading zeros
        BX      LR


@********************************************************************************************************
@                                         CPU_CntTrailZeros()
@                                        COUNT TRAILING ZEROS
@
@ Description : Counts the number of contiguous, least-significant, trailing zero bits before the
@                   first binary one bit in a data value.
@
@ Prototype   : CPU_DATA  CPU_CntTrailZeros(CPU_DATA  val);
@
@ Argument(s) : val         Data value to count trailing zero bits.
@
@ Return(s)   : Number of contiguous, least-significant, trailing zero bits in 'val'.
@
@ Note(s)     : (1) (a) Supports 32-bit data value size as configured by 'CPU_DATA' (see 'cpu.h
@                       CPU WORD CONFIGURATION  Note #1').
@
@                   (b) For 32-bit values :
@
@                             b31  b30  b29  b28  b27  ...  b02  b01  b00    # Trailing Zeros
@                             ---  ---  ---  ---  ---       ---  ---  ---    ----------------
@                              x    x    x    x    x         x    x    1            0
@                              x    x    x    x    x         x    1    0            1
@                              x    x    x    x    x         1    0    0            2
@                              :    :    :    :    :         :    :    :            :
@                              :    :    :    :    :         :    :    :            :
@                              x    x    x    x    1         0    0    0           27
@                              x    x    x    1    0         0    0    0           28
@                              x    x    1    0    0         0    0    0           29
@                              x    1    0    0    0         0    0    0           30
@                              1    0    0    0    0         0    0    0           31
@                              0    0    0    0    0         0    0    0           32
@
@
@               (2) MUST be defined in 'cpu_a.asm' (or 'cpu_c.c') if CPU_CFG_TRAIL_ZEROS_ASM_PRESENT is
@                   #define'd in 'cpu_cfg.h' or 'cpu.h'.
@********************************************************************************************************

.thumb_func
CPU_CntTrailZeros:
        RBIT    R0, R0                          @ Reverse bits
        CLZ     R0, R0                          @ Count trailing zeros
        BX      LR


@********************************************************************************************************
@                                            CPU_RevBits()
@                                            REVERSE BITS
@
@ Description : Reverses the bits in a data value.
@
@ Prototypes  : CPU_DATA  CPU_RevBits(CPU_DATA  val);
@
@ Argument(s) : val         Data value to reverse bits.
@
@ Return(s)   : Value with all bits in 'val' reversed (see Note #1).
@
@ Note(s)     : (1) The final, reversed data value for 'val' is such that :
@
@                       'val's final bit  0       =  'val's original bit  N
@                       'val's final bit  1       =  'val's original bit (N - 1)
@                       'val's final bit  2       =  'val's original bit (N - 2)
@
@                               ...                           ...
@
@                       'val's final bit (N - 2)  =  'val's original bit  2
@                       'val's final bit (N - 1)  =  'val's original bit  1
@                       'val's final bit  N       =  'val's original bit  0
@********************************************************************************************************

.thumb_func
CPU_RevBits:
        RBIT    R0, R0                          @ Reverse bits
        BX      LR


@********************************************************************************************************
@                                     CPU ASSEMBLY PORT FILE END
@********************************************************************************************************

.end


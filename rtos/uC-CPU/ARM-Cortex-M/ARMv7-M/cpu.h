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
*                                               ARMv7-M
*                                            GNU C Compiler
*
* Filename : cpu.h
* Version  : v1.32.00
*********************************************************************************************************
* Note(s)  : This port supports the ARM Cortex-M3, Cortex-M4 and Cortex-M7 architectures.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) This CPU header file is protected from multiple pre-processor inclusion through use of
*               the  CPU module present pre-processor macro definition.
*********************************************************************************************************
*/

#ifndef  CPU_MODULE_PRESENT                                     /* See Note #1.                                         */
#define  CPU_MODULE_PRESENT


/*
*********************************************************************************************************
*                                          CPU INCLUDE FILES
*
* Note(s) : (1) The following CPU files are located in the following directories :
*
*               (a) \<Your Product Application>\cpu_cfg.h
*
*               (b) (1) \<CPU-Compiler Directory>\cpu_def.h
*                   (2) \<CPU-Compiler Directory>\<cpu>\<compiler>\cpu*.*
*
*                       where
*                               <Your Product Application>      directory path for Your Product's Application
*                               <CPU-Compiler Directory>        directory path for common   CPU-compiler software
*                               <cpu>                           directory name for specific CPU
*                               <compiler>                      directory name for specific compiler
*
*           (2) Compiler MUST be configured to include as additional include path directories :
*
*               (a) '\<Your Product Application>\' directory                            See Note #1a
*
*               (b) (1) '\<CPU-Compiler Directory>\'                  directory         See Note #1b1
*                   (2) '\<CPU-Compiler Directory>\<cpu>\<compiler>\' directory         See Note #1b2
*
*           (3) Since NO custom library modules are included, 'cpu.h' may ONLY use configurations from
*               CPU configuration file 'cpu_cfg.h' that do NOT reference any custom library definitions.
*
*               In other words, 'cpu.h' may use 'cpu_cfg.h' configurations that are #define'd to numeric
*               constants or to NULL (i.e. NULL-valued #define's); but may NOT use configurations to
*               custom library #define's (e.g. DEF_DISABLED or DEF_ENABLED).
*********************************************************************************************************
*/

#include  <cpu_def.h>
#include  <cpu_cfg.h>                                           /* See Note #3.                                         */

#ifdef __cplusplus
extern  "C" {
#endif


/*
*********************************************************************************************************
*                                    CONFIGURE STANDARD DATA TYPES
*
* Note(s) : (1) Configure standard data types according to CPU-/compiler-specifications.
*
*           (2) (a) (1) 'CPU_FNCT_VOID' data type defined to replace the commonly-used function pointer
*                       data type of a pointer to a function which returns void & has no arguments.
*
*                   (2) Example function pointer usage :
*
*                           CPU_FNCT_VOID  FnctName;
*
*                           FnctName();
*
*               (b) (1) 'CPU_FNCT_PTR'  data type defined to replace the commonly-used function pointer
*                       data type of a pointer to a function which returns void & has a single void
*                       pointer argument.
*
*                   (2) Example function pointer usage :
*
*                           CPU_FNCT_PTR   FnctName;
*                           void          *p_obj
*
*                           FnctName(p_obj);
*********************************************************************************************************
*/

typedef            void        CPU_VOID;
typedef            char        CPU_CHAR;                        /*  8-bit character                                     */
typedef  unsigned  char        CPU_BOOLEAN;                     /*  8-bit boolean or logical                            */
typedef  unsigned  char        CPU_INT08U;                      /*  8-bit unsigned integer                              */
typedef    signed  char        CPU_INT08S;                      /*  8-bit   signed integer                              */
typedef  unsigned  short       CPU_INT16U;                      /* 16-bit unsigned integer                              */
typedef    signed  short       CPU_INT16S;                      /* 16-bit   signed integer                              */
typedef  unsigned  int         CPU_INT32U;                      /* 32-bit unsigned integer                              */
typedef    signed  int         CPU_INT32S;                      /* 32-bit   signed integer                              */
typedef  unsigned  long  long  CPU_INT64U;                      /* 64-bit unsigned integer                              */
typedef    signed  long  long  CPU_INT64S;                      /* 64-bit   signed integer                              */

typedef            float       CPU_FP32;                        /* 32-bit floating point                                */
typedef            double      CPU_FP64;                        /* 64-bit floating point                                */


typedef  volatile  CPU_INT08U  CPU_REG08;                       /*  8-bit register                                      */
typedef  volatile  CPU_INT16U  CPU_REG16;                       /* 16-bit register                                      */
typedef  volatile  CPU_INT32U  CPU_REG32;                       /* 32-bit register                                      */
typedef  volatile  CPU_INT64U  CPU_REG64;                       /* 64-bit register                                      */


typedef            void      (*CPU_FNCT_VOID)(void);            /* See Note #2a.                                        */
typedef            void      (*CPU_FNCT_PTR )(void *p_obj);     /* See Note #2b.                                        */


/*
*********************************************************************************************************
*                                       CPU WORD CONFIGURATION
*
* Note(s) : (1) Configure CPU_CFG_ADDR_SIZE, CPU_CFG_DATA_SIZE, & CPU_CFG_DATA_SIZE_MAX with CPU's &/or
*               compiler's word sizes :
*
*                   CPU_WORD_SIZE_08             8-bit word size
*                   CPU_WORD_SIZE_16            16-bit word size
*                   CPU_WORD_SIZE_32            32-bit word size
*                   CPU_WORD_SIZE_64            64-bit word size
*
*           (2) Configure CPU_CFG_ENDIAN_TYPE with CPU's data-word-memory order :
*
*               (a) CPU_ENDIAN_TYPE_BIG         Big-   endian word order (CPU words' most  significant
*                                                                         octet @ lowest memory address)
*               (b) CPU_ENDIAN_TYPE_LITTLE      Little-endian word order (CPU words' least significant
*                                                                         octet @ lowest memory address)
*********************************************************************************************************
*/

                                                                /* Define  CPU         word sizes (see Note #1) :       */
#define  CPU_CFG_ADDR_SIZE              CPU_WORD_SIZE_32        /* Defines CPU address word size  (in octets).          */
#define  CPU_CFG_DATA_SIZE              CPU_WORD_SIZE_32        /* Defines CPU data    word size  (in octets).          */
#define  CPU_CFG_DATA_SIZE_MAX          CPU_WORD_SIZE_64        /* Defines CPU maximum word size  (in octets).          */

#define  CPU_CFG_ENDIAN_TYPE            CPU_ENDIAN_TYPE_LITTLE  /* Defines CPU data    word-memory order (see Note #2). */


/*
*********************************************************************************************************
*                                 CONFIGURE CPU ADDRESS & DATA TYPES
*********************************************************************************************************
*/

                                                                /* CPU address type based on address bus size.          */
#if     (CPU_CFG_ADDR_SIZE == CPU_WORD_SIZE_32)
typedef  CPU_INT32U  CPU_ADDR;
#elif   (CPU_CFG_ADDR_SIZE == CPU_WORD_SIZE_16)
typedef  CPU_INT16U  CPU_ADDR;
#else
typedef  CPU_INT08U  CPU_ADDR;
#endif

                                                                /* CPU data    type based on data    bus size.          */
#if     (CPU_CFG_DATA_SIZE == CPU_WORD_SIZE_32)
typedef  CPU_INT32U  CPU_DATA;
#elif   (CPU_CFG_DATA_SIZE == CPU_WORD_SIZE_16)
typedef  CPU_INT16U  CPU_DATA;
#else
typedef  CPU_INT08U  CPU_DATA;
#endif


typedef  CPU_DATA    CPU_ALIGN;                                 /* Defines CPU data-word-alignment size.                */
typedef  CPU_ADDR    CPU_SIZE_T;                                /* Defines CPU standard 'size_t'   size.                */


/*
*********************************************************************************************************
*                                       CPU STACK CONFIGURATION
*
* Note(s) : (1) Configure CPU_CFG_STK_GROWTH in 'cpu.h' with CPU's stack growth order :
*
*               (a) CPU_STK_GROWTH_LO_TO_HI     CPU stack pointer increments to the next higher  stack
*                                                   memory address after data is pushed onto the stack
*               (b) CPU_STK_GROWTH_HI_TO_LO     CPU stack pointer decrements to the next lower   stack
*                                                   memory address after data is pushed onto the stack
*
*           (2) Configure CPU_CFG_STK_ALIGN_BYTES with the highest minimum alignement required for
*               cpu stacks.
*
*               (a) ARM Procedure Calls Standard requires an 8 bytes stack alignment.
*********************************************************************************************************
*/

#define  CPU_CFG_STK_GROWTH       CPU_STK_GROWTH_HI_TO_LO       /* Defines CPU stack growth order (see Note #1).        */

#define  CPU_CFG_STK_ALIGN_BYTES  (8u)                          /* Defines CPU stack alignment in bytes. (see Note #2). */

typedef  CPU_INT32U               CPU_STK;                      /* Defines CPU stack data type.                         */
typedef  CPU_ADDR                 CPU_STK_SIZE;                 /* Defines CPU stack size data type.                    */


/*
*********************************************************************************************************
*                                   CRITICAL SECTION CONFIGURATION
*
* Note(s) : (1) Configure CPU_CFG_CRITICAL_METHOD with CPU's/compiler's critical section method :
*
*                                                       Enter/Exit critical sections by ...
*
*                   CPU_CRITICAL_METHOD_INT_DIS_EN      Disable/Enable interrupts
*                   CPU_CRITICAL_METHOD_STATUS_STK      Push/Pop       interrupt status onto stack
*                   CPU_CRITICAL_METHOD_STATUS_LOCAL    Save/Restore   interrupt status to local variable
*
*               (a) CPU_CRITICAL_METHOD_INT_DIS_EN  is NOT a preferred method since it does NOT support
*                   multiple levels of interrupts.  However, with some CPUs/compilers, this is the only
*                   available method.
*
*               (b) CPU_CRITICAL_METHOD_STATUS_STK    is one preferred method since it supports multiple
*                   levels of interrupts.  However, this method assumes that the compiler provides C-level
*                   &/or assembly-level functionality for the following :
*
*                     ENTER CRITICAL SECTION :
*                       (1) Push/save   interrupt status onto a local stack
*                       (2) Disable     interrupts
*
*                     EXIT  CRITICAL SECTION :
*                       (3) Pop/restore interrupt status from a local stack
*
*               (c) CPU_CRITICAL_METHOD_STATUS_LOCAL  is one preferred method since it supports multiple
*                   levels of interrupts.  However, this method assumes that the compiler provides C-level
*                   &/or assembly-level functionality for the following :
*
*                     ENTER CRITICAL SECTION :
*                       (1) Save    interrupt status into a local variable
*                       (2) Disable interrupts
*
*                     EXIT  CRITICAL SECTION :
*                       (3) Restore interrupt status from a local variable
*
*           (2) Critical section macro's most likely require inline assembly.  If the compiler does NOT
*               allow inline assembly in C source files, critical section macro's MUST call an assembly
*               subroutine defined in a 'cpu_a.asm' file located in the following software directory :
*
*                   \<CPU-Compiler Directory>\<cpu>\<compiler>\
*
*                       where
*                               <CPU-Compiler Directory>    directory path for common   CPU-compiler software
*                               <cpu>                       directory name for specific CPU
*                               <compiler>                  directory name for specific compiler
*
*           (3) (a) To save/restore interrupt status, a local variable 'cpu_sr' of type 'CPU_SR' MAY need
*                   to be declared (e.g. if 'CPU_CRITICAL_METHOD_STATUS_LOCAL' method is configured).
*
*                   (1) 'cpu_sr' local variable should be declared via the CPU_SR_ALLOC() macro which, if
*                        used, MUST be declared following ALL other local variables.
*
*                        Example :
*
*                           void  Fnct (void)
*                           {
*                               CPU_INT08U  val_08;
*                               CPU_INT16U  val_16;
*                               CPU_INT32U  val_32;
*                               CPU_SR_ALLOC();         MUST be declared after ALL other local variables
*                                   :
*                                   :
*                           }
*
*               (b) Configure 'CPU_SR' data type with the appropriate-sized CPU data type large enough to
*                   completely store the CPU's/compiler's status word.
*********************************************************************************************************
*/
                                                                /* Configure CPU critical method      (see Note #1) :   */
#define  CPU_CFG_CRITICAL_METHOD    CPU_CRITICAL_METHOD_STATUS_LOCAL

typedef  CPU_INT32U                 CPU_SR;                     /* Defines   CPU status register size (see Note #3b).   */

                                                                /* Allocates CPU status register word (see Note #3a).   */
#if     (CPU_CFG_CRITICAL_METHOD == CPU_CRITICAL_METHOD_STATUS_LOCAL)
#define  CPU_SR_ALLOC()             CPU_SR  cpu_sr = (CPU_SR)0
#else
#define  CPU_SR_ALLOC()
#endif
                                                                /* Save CPU current BASEPRI priority lvl for exception. */
#define  CPU_INT_DIS()         do { cpu_sr = CPU_SR_Save(CPU_CFG_KA_IPL_BOUNDARY << (8u - CPU_CFG_NVIC_PRIO_BITS));} while (0)
#define  CPU_INT_EN()          do { CPU_SR_Restore(cpu_sr); } while (0) /* Restore CPU BASEPRI priority level.          */


#ifdef   CPU_CFG_INT_DIS_MEAS_EN
                                                                        /* Disable interrupts, ...                      */
                                                                        /* & start interrupts disabled time measurement.*/
#define  CPU_CRITICAL_ENTER()  do { CPU_INT_DIS();         \
                                    CPU_IntDisMeasStart(); }  while (0)
                                                                        /* Stop & measure   interrupts disabled time,   */
                                                                        /* ...  & re-enable interrupts.                 */
#define  CPU_CRITICAL_EXIT()   do { CPU_IntDisMeasStop();  \
                                    CPU_INT_EN();          }  while (0)

#else

#define  CPU_CRITICAL_ENTER()  do { CPU_INT_DIS(); } while (0)          /* Disable   interrupts.                        */
#define  CPU_CRITICAL_EXIT()   do { CPU_INT_EN();  } while (0)          /* Re-enable interrupts.                        */

#endif


/*
*********************************************************************************************************
*                                    MEMORY BARRIERS CONFIGURATION
*
* Note(s) : (1) (a) Configure memory barriers if required by the architecture.
*
*                   CPU_MB      Full memory barrier.
*                   CPU_RMB     Read (Loads) memory barrier.
*                   CPU_WMB     Write (Stores) memory barrier.
*
*********************************************************************************************************
*/

#define  CPU_MB()       __asm__ __volatile__ ("dsb" : : : "memory")
#define  CPU_RMB()      __asm__ __volatile__ ("dsb" : : : "memory")
#define  CPU_WMB()      __asm__ __volatile__ ("dsb" : : : "memory")


/*
*********************************************************************************************************
*                                    CPU COUNT ZEROS CONFIGURATION
*
* Note(s) : (1) (a) Configure CPU_CFG_LEAD_ZEROS_ASM_PRESENT  to define count leading  zeros bits
*                   function(s) in :
*
*                   (1) 'cpu_a.asm',  if CPU_CFG_LEAD_ZEROS_ASM_PRESENT       #define'd in 'cpu.h'/
*                                         'cpu_cfg.h' to enable assembly-optimized function(s)
*
*                   (2) 'cpu_core.c', if CPU_CFG_LEAD_ZEROS_ASM_PRESENT   NOT #define'd in 'cpu.h'/
*                                         'cpu_cfg.h' to enable C-source-optimized function(s) otherwise
*
*               (b) Configure CPU_CFG_TRAIL_ZEROS_ASM_PRESENT to define count trailing zeros bits
*                   function(s) in :
*
*                   (1) 'cpu_a.asm',  if CPU_CFG_TRAIL_ZEROS_ASM_PRESENT      #define'd in 'cpu.h'/
*                                         'cpu_cfg.h' to enable assembly-optimized function(s)
*
*                   (2) 'cpu_core.c', if CPU_CFG_TRAIL_ZEROS_ASM_PRESENT  NOT #define'd in 'cpu.h'/
*                                         'cpu_cfg.h' to enable C-source-optimized function(s) otherwise
*********************************************************************************************************
*/

                                                                /* Configure CPU count leading  zeros bits ...          */
#define  CPU_CFG_LEAD_ZEROS_ASM_PRESENT                         /* ... assembly-version (see Note #1a).                 */

                                                                /* Configure CPU count trailing zeros bits ...          */
#define  CPU_CFG_TRAIL_ZEROS_ASM_PRESENT                        /* ... assembly-version (see Note #1b).                 */


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void        CPU_IntDis       (void);
void        CPU_IntEn        (void);

void        CPU_IntSrcDis    (CPU_INT08U  pos);
void        CPU_IntSrcEn     (CPU_INT08U  pos);
void        CPU_IntSrcPendClr(CPU_INT08U  pos);
CPU_INT16S  CPU_IntSrcPrioGet(CPU_INT08U  pos);
void        CPU_IntSrcPrioSet(CPU_INT08U  pos,
                              CPU_INT08U  prio,
                              CPU_INT08U  type);


CPU_SR      CPU_SR_Save      (CPU_SR      new_basepri);
void        CPU_SR_Restore   (CPU_SR      cpu_sr);


void        CPU_WaitForInt   (void);
void        CPU_WaitForExcept(void);


CPU_DATA    CPU_RevBits      (CPU_DATA    val);

void        CPU_BitBandClr   (CPU_ADDR    addr,
                              CPU_INT08U  bit_nbr);
void        CPU_BitBandSet   (CPU_ADDR    addr,
                              CPU_INT08U  bit_nbr);


/*
*********************************************************************************************************
*                                          INTERRUPT SOURCES
*********************************************************************************************************
*/

#define  CPU_INT_STK_PTR                                   0u
#define  CPU_INT_RESET                                     1u
#define  CPU_INT_NMI                                       2u
#define  CPU_INT_HFAULT                                    3u
#define  CPU_INT_MEM                                       4u
#define  CPU_INT_BUSFAULT                                  5u
#define  CPU_INT_USAGEFAULT                                6u
#define  CPU_INT_RSVD_07                                   7u
#define  CPU_INT_RSVD_08                                   8u
#define  CPU_INT_RSVD_09                                   9u
#define  CPU_INT_RSVD_10                                  10u
#define  CPU_INT_SVCALL                                   11u
#define  CPU_INT_DBGMON                                   12u
#define  CPU_INT_RSVD_13                                  13u
#define  CPU_INT_PENDSV                                   14u
#define  CPU_INT_SYSTICK                                  15u
#define  CPU_INT_EXT0                                     16u


/*
*********************************************************************************************************
*                                            INTERRUPT TYPE
*********************************************************************************************************
*/

#define  CPU_INT_KA                                        0u   /* Kernel Aware     interrupt request.                  */
#define  CPU_INT_NKA                                       1u   /* Non-Kernel Aware interrupt request.                  */


/*
*********************************************************************************************************
*                                            CPU REGISTERS
*********************************************************************************************************
*/
                                                                                /* -------- SYSTICK REGISTERS --------- */
#define  CPU_REG_SYST_CSR            (*((CPU_REG32 *)(0xE000E010)))             /* SysTick Ctrl & Status Reg.           */
#define  CPU_REG_SYST_RVR            (*((CPU_REG32 *)(0xE000E014)))             /* SysTick Reload      Value Reg.       */
#define  CPU_REG_SYST_CVR            (*((CPU_REG32 *)(0xE000E018)))             /* SysTick Current     Value Reg.       */
#define  CPU_REG_SYST_CALIB          (*((CPU_REG32 *)(0xE000E01C)))             /* SysTick Calibration Value Reg.       */

                                                                                /* ---------- NVIC REGISTERS ---------- */
#define  CPU_REG_NVIC_ISER(n)        (*((CPU_REG32 *)(0xE000E100 + (n) * 4u)))  /* IRQ Set En Reg.                      */
#define  CPU_REG_NVIC_ICER(n)        (*((CPU_REG32 *)(0xE000E180 + (n) * 4u)))  /* IRQ Clr En Reg.                      */
#define  CPU_REG_NVIC_ISPR(n)        (*((CPU_REG32 *)(0xE000E200 + (n) * 4u)))  /* IRQ Set Pending Reg.                 */
#define  CPU_REG_NVIC_ICPR(n)        (*((CPU_REG32 *)(0xE000E280 + (n) * 4u)))  /* IRQ Clr Pending Reg.                 */
#define  CPU_REG_NVIC_IABR(n)        (*((CPU_REG32 *)(0xE000E300 + (n) * 4u)))  /* IRQ Active Reg.                      */
#define  CPU_REG_NVIC_IPR(n)         (*((CPU_REG32 *)(0xE000E400 + (n) * 4u)))  /* IRQ Prio Reg.                        */

                                                                                /* -- SYSTEM CONTROL BLOCK(SCB) REG  -- */
#define  CPU_REG_SCB_CPUID           (*((CPU_REG32 *)(0xE000ED00)))             /* CPUID Base Reg.                      */
#define  CPU_REG_SCB_ICSR            (*((CPU_REG32 *)(0xE000ED04)))             /* Int Ctrl State  Reg.                 */
#define  CPU_REG_SCB_VTOR            (*((CPU_REG32 *)(0xE000ED08)))             /* Vect Tbl Offset Reg.                 */
#define  CPU_REG_SCB_AIRCR           (*((CPU_REG32 *)(0xE000ED0C)))             /* App Int/Reset Ctrl Reg.              */
#define  CPU_REG_SCB_SCR             (*((CPU_REG32 *)(0xE000ED10)))             /* System Ctrl Reg.                     */
#define  CPU_REG_SCB_CCR             (*((CPU_REG32 *)(0xE000ED14)))             /* Cfg    Ctrl Reg.                     */
#define  CPU_REG_SCB_SHPRI1          (*((CPU_REG32 *)(0xE000ED18)))             /* System Handlers  4 to  7 Prio.       */
#define  CPU_REG_SCB_SHPRI2          (*((CPU_REG32 *)(0xE000ED1C)))             /* System Handlers  8 to 11 Prio.       */
#define  CPU_REG_SCB_SHPRI3          (*((CPU_REG32 *)(0xE000ED20)))             /* System Handlers 12 to 15 Prio.       */
#define  CPU_REG_SCB_SHCSR           (*((CPU_REG32 *)(0xE000ED24)))             /* System Handler Ctrl & State Reg.     */
#define  CPU_REG_SCB_CFSR            (*((CPU_REG32 *)(0xE000ED28)))             /* Configurable Fault Status Reg.       */
#define  CPU_REG_SCB_HFSR            (*((CPU_REG32 *)(0xE000ED2C)))             /* Hard  Fault Status Reg.              */
#define  CPU_REG_SCB_DFSR            (*((CPU_REG32 *)(0xE000ED30)))             /* Debug Fault Status Reg.              */
#define  CPU_REG_SCB_MMFAR           (*((CPU_REG32 *)(0xE000ED34)))             /* Mem Manage Addr Reg.                 */
#define  CPU_REG_SCB_BFAR            (*((CPU_REG32 *)(0xE000ED38)))             /* Bus Fault  Addr Reg.                 */
#define  CPU_REG_SCB_AFSR            (*((CPU_REG32 *)(0xE000ED3C)))             /* Aux Fault Status Reg.                */
#define  CPU_REG_SCB_CPACR           (*((CPU_REG32 *)(0xE000ED88)))             /* Coprocessor Access Control Reg.      */

                                                                                /* ----- SCB REG FOR FP EXTENSION ----- */
#define  CPU_REG_SCB_FPCCR           (*((CPU_REG32 *)(0xE000EF34)))             /* Floating-Point Context Control Reg.  */
#define  CPU_REG_SCB_FPCAR           (*((CPU_REG32 *)(0xE000EF38)))             /* Floating-Point Context Address Reg.  */
#define  CPU_REG_SCB_FPDSCR          (*((CPU_REG32 *)(0xE000EF3C)))             /* FP Default Status Control Reg.       */

                                                                                /* ---------- CPUID REGISTERS --------- */
#define  CPU_REG_CPUID_PFR0          (*((CPU_REG32 *)(0xE000ED40)))             /* Processor Feature Reg 0.             */
#define  CPU_REG_CPUID_PFR1          (*((CPU_REG32 *)(0xE000ED44)))             /* Processor Feature Reg 1.             */
#define  CPU_REG_CPUID_DFR0          (*((CPU_REG32 *)(0xE000ED48)))             /* Debug     Feature Reg 0.             */
#define  CPU_REG_CPUID_AFR0          (*((CPU_REG32 *)(0xE000ED4C)))             /* Aux       Feature Reg 0.             */
#define  CPU_REG_CPUID_MMFR0         (*((CPU_REG32 *)(0xE000ED50)))             /* Memory Model Feature Reg 0.          */
#define  CPU_REG_CPUID_MMFR1         (*((CPU_REG32 *)(0xE000ED54)))             /* Memory Model Feature Reg 1.          */
#define  CPU_REG_CPUID_MMFR2         (*((CPU_REG32 *)(0xE000ED58)))             /* Memory Model Feature Reg 2.          */
#define  CPU_REG_CPUID_MMFR3         (*((CPU_REG32 *)(0xE000ED5C)))             /* Memory Model Feature Reg 3.          */
#define  CPU_REG_CPUID_ISAFR0        (*((CPU_REG32 *)(0xE000ED60)))             /* ISA Feature Reg 0.                   */
#define  CPU_REG_CPUID_ISAFR1        (*((CPU_REG32 *)(0xE000ED64)))             /* ISA Feature Reg 1.                   */
#define  CPU_REG_CPUID_ISAFR2        (*((CPU_REG32 *)(0xE000ED68)))             /* ISA Feature Reg 2.                   */
#define  CPU_REG_CPUID_ISAFR3        (*((CPU_REG32 *)(0xE000ED6C)))             /* ISA Feature Reg 3.                   */
#define  CPU_REG_CPUID_ISAFR4        (*((CPU_REG32 *)(0xE000ED70)))             /* ISA Feature Reg 4.                   */

                                                                                /* ----------- MPU REGISTERS ---------- */
#define  CPU_REG_MPU_TYPE            (*((CPU_REG32 *)(0xE000ED90)))             /* MPU Type Reg.                        */
#define  CPU_REG_MPU_CTRL            (*((CPU_REG32 *)(0xE000ED94)))             /* MPU Ctrl Reg.                        */
#define  CPU_REG_MPU_RNR             (*((CPU_REG32 *)(0xE000ED98)))             /* MPU Region Nbr Reg.                  */
#define  CPU_REG_MPU_RBAR            (*((CPU_REG32 *)(0xE000ED9C)))             /* MPU Region Base Addr Reg.            */
#define  CPU_REG_MPU_RASR            (*((CPU_REG32 *)(0xE000EDA0)))             /* MPU Region Attrib & Size Reg.        */

                                                                                /* ----- REGISTERS NOT IN THE SCB ----- */
#define  CPU_REG_ICTR                (*((CPU_REG32 *)(0xE000E004)))             /* Int Ctrl'er Type Reg.                */
#define  CPU_REG_DHCSR               (*((CPU_REG32 *)(0xE000EDF0)))             /* Debug Halting Ctrl & Status Reg.     */
#define  CPU_REG_DCRSR               (*((CPU_REG32 *)(0xE000EDF4)))             /* Debug Core Reg Selector Reg.         */
#define  CPU_REG_DCRDR               (*((CPU_REG32 *)(0xE000EDF8)))             /* Debug Core Reg Data     Reg.         */
#define  CPU_REG_DEMCR               (*((CPU_REG32 *)(0xE000EDFC)))             /* Debug Except & Monitor Ctrl Reg.     */
#define  CPU_REG_STIR                (*((CPU_REG32 *)(0xE000EF00)))             /* Software Trigger Int Reg.            */


/*
*********************************************************************************************************
*                                          CPU REGISTER BITS
*********************************************************************************************************
*/

                                                                /* ---------- SYSTICK CTRL & STATUS REG BITS ---------- */
#define  CPU_REG_SYST_CSR_COUNTFLAG               0x00010000
#define  CPU_REG_SYST_CSR_CLKSOURCE               0x00000004
#define  CPU_REG_SYST_CSR_TICKINT                 0x00000002
#define  CPU_REG_SYST_CSR_ENABLE                  0x00000001

                                                                /* -------- SYSTICK CALIBRATION VALUE REG BITS -------- */
#define  CPU_REG_SYST_CALIB_NOREF                 0x80000000
#define  CPU_REG_SYST_CALIB_SKEW                  0x40000000

                                                                /* -------------- INT CTRL STATE REG BITS ------------- */
#define  CPU_REG_SCB_ICSR_NMIPENDSET              0x80000000
#define  CPU_REG_SCB_ICSR_PENDSVSET               0x10000000
#define  CPU_REG_SCB_ICSR_PENDSVCLR               0x08000000
#define  CPU_REG_SCB_ICSR_PENDSTSET               0x04000000
#define  CPU_REG_SCB_ICSR_PENDSTCLR               0x02000000
#define  CPU_REG_SCB_ICSR_ISRPREEMPT              0x00800000
#define  CPU_REG_SCB_ICSR_ISRPENDING              0x00400000
#define  CPU_REG_SCB_ICSR_RETTOBASE               0x00000800

                                                                /* ------------- VECT TBL OFFSET REG BITS ------------- */
#define  CPU_REG_SCB_VTOR_TBLBASE                 0x20000000

                                                                /* ------------ APP INT/RESET CTRL REG BITS ----------- */
#define  CPU_REG_SCB_AIRCR_ENDIANNESS             0x00008000
#define  CPU_REG_SCB_AIRCR_SYSRESETREQ            0x00000004
#define  CPU_REG_SCB_AIRCR_VECTCLRACTIVE          0x00000002
#define  CPU_REG_SCB_AIRCR_VECTRESET              0x00000001

                                                                /* --------------- SYSTEM CTRL REG BITS --------------- */
#define  CPU_REG_SCB_SCR_SEVONPEND                0x00000010
#define  CPU_REG_SCB_SCR_SLEEPDEEP                0x00000004
#define  CPU_REG_SCB_SCR_SLEEPONEXIT              0x00000002

                                                                /* ----------------- CFG CTRL REG BITS ---------------- */
#define  CPU_REG_SCB_CCR_STKALIGN                 0x00000200
#define  CPU_REG_SCB_CCR_BFHFNMIGN                0x00000100
#define  CPU_REG_SCB_CCR_DIV_0_TRP                0x00000010
#define  CPU_REG_SCB_CCR_UNALIGN_TRP              0x00000008
#define  CPU_REG_SCB_CCR_USERSETMPEND             0x00000002
#define  CPU_REG_SCB_CCR_NONBASETHRDENA           0x00000001

                                                                /* ------- SYSTEM HANDLER CTRL & STATE REG BITS ------- */
#define  CPU_REG_SCB_SHCSR_USGFAULTENA            0x00040000
#define  CPU_REG_SCB_SHCSR_BUSFAULTENA            0x00020000
#define  CPU_REG_SCB_SHCSR_MEMFAULTENA            0x00010000
#define  CPU_REG_SCB_SHCSR_SVCALLPENDED           0x00008000
#define  CPU_REG_SCB_SHCSR_BUSFAULTPENDED         0x00004000
#define  CPU_REG_SCB_SHCSR_MEMFAULTPENDED         0x00002000
#define  CPU_REG_SCB_SHCSR_USGFAULTPENDED         0x00001000
#define  CPU_REG_SCB_SHCSR_SYSTICKACT             0x00000800
#define  CPU_REG_SCB_SHCSR_PENDSVACT              0x00000400
#define  CPU_REG_SCB_SHCSR_MONITORACT             0x00000100
#define  CPU_REG_SCB_SHCSR_SVCALLACT              0x00000080
#define  CPU_REG_SCB_SHCSR_USGFAULTACT            0x00000008
#define  CPU_REG_SCB_SHCSR_BUSFAULTACT            0x00000002
#define  CPU_REG_SCB_SHCSR_MEMFAULTACT            0x00000001

                                                                /* -------- CONFIGURABLE FAULT STATUS REG BITS -------- */
#define  CPU_REG_SCB_CFSR_DIVBYZERO               0x02000000
#define  CPU_REG_SCB_CFSR_UNALIGNED               0x01000000
#define  CPU_REG_SCB_CFSR_NOCP                    0x00080000
#define  CPU_REG_SCB_CFSR_INVPC                   0x00040000
#define  CPU_REG_SCB_CFSR_INVSTATE                0x00020000
#define  CPU_REG_SCB_CFSR_UNDEFINSTR              0x00010000
#define  CPU_REG_SCB_CFSR_BFARVALID               0x00008000
#define  CPU_REG_SCB_CFSR_STKERR                  0x00001000
#define  CPU_REG_SCB_CFSR_UNSTKERR                0x00000800
#define  CPU_REG_SCB_CFSR_IMPRECISERR             0x00000400
#define  CPU_REG_SCB_CFSR_PRECISERR               0x00000200
#define  CPU_REG_SCB_CFSR_IBUSERR                 0x00000100
#define  CPU_REG_SCB_CFSR_MMARVALID               0x00000080
#define  CPU_REG_SCB_CFSR_MSTKERR                 0x00000010
#define  CPU_REG_SCB_CFSR_MUNSTKERR               0x00000008
#define  CPU_REG_SCB_CFSR_DACCVIOL                0x00000002
#define  CPU_REG_SCB_CFSR_IACCVIOL                0x00000001

                                                                /* ------------ HARD FAULT STATUS REG BITS ------------ */
#define  CPU_REG_SCB_HFSR_DEBUGEVT                0x80000000
#define  CPU_REG_SCB_HFSR_FORCED                  0x40000000
#define  CPU_REG_SCB_HFSR_VECTTBL                 0x00000002

                                                                /* ------------ DEBUG FAULT STATUS REG BITS ----------- */
#define  CPU_REG_SCB_DFSR_EXTERNAL                0x00000010
#define  CPU_REG_SCB_DFSR_VCATCH                  0x00000008
#define  CPU_REG_SCB_DFSR_DWTTRAP                 0x00000004
#define  CPU_REG_SCB_DFSR_BKPT                    0x00000002
#define  CPU_REG_SCB_DFSR_HALTED                  0x00000001

                                                                /* -------- COPROCESSOR ACCESS CONTROL REG BITS ------- */
#define  CPU_REG_SCB_CPACR_CP10_FULL_ACCESS       0x00300000
#define  CPU_REG_SCB_CPACR_CP11_FULL_ACCESS       0x00C00000


/*
*********************************************************************************************************
*                                          CPU REGISTER MASK
*********************************************************************************************************
*/

#define  CPU_MSK_SCB_ICSR_VECT_ACTIVE             0x000001FF


/*
*********************************************************************************************************
*                                        CONFIGURATION ERRORS
*********************************************************************************************************
*/

#ifndef  CPU_CFG_ADDR_SIZE
#error  "CPU_CFG_ADDR_SIZE              not #define'd in 'cpu.h'               "
#error  "                         [MUST be  CPU_WORD_SIZE_08   8-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_16  16-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_32  32-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_64  64-bit alignment]"

#elif  ((CPU_CFG_ADDR_SIZE != CPU_WORD_SIZE_08) && \
        (CPU_CFG_ADDR_SIZE != CPU_WORD_SIZE_16) && \
        (CPU_CFG_ADDR_SIZE != CPU_WORD_SIZE_32) && \
        (CPU_CFG_ADDR_SIZE != CPU_WORD_SIZE_64))
#error  "CPU_CFG_ADDR_SIZE        illegally #define'd in 'cpu.h'               "
#error  "                         [MUST be  CPU_WORD_SIZE_08   8-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_16  16-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_32  32-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_64  64-bit alignment]"
#endif


#ifndef  CPU_CFG_DATA_SIZE
#error  "CPU_CFG_DATA_SIZE              not #define'd in 'cpu.h'               "
#error  "                         [MUST be  CPU_WORD_SIZE_08   8-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_16  16-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_32  32-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_64  64-bit alignment]"

#elif  ((CPU_CFG_DATA_SIZE != CPU_WORD_SIZE_08) && \
        (CPU_CFG_DATA_SIZE != CPU_WORD_SIZE_16) && \
        (CPU_CFG_DATA_SIZE != CPU_WORD_SIZE_32) && \
        (CPU_CFG_DATA_SIZE != CPU_WORD_SIZE_64))
#error  "CPU_CFG_DATA_SIZE        illegally #define'd in 'cpu.h'               "
#error  "                         [MUST be  CPU_WORD_SIZE_08   8-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_16  16-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_32  32-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_64  64-bit alignment]"
#endif


#ifndef  CPU_CFG_DATA_SIZE_MAX
#error  "CPU_CFG_DATA_SIZE_MAX          not #define'd in 'cpu.h'               "
#error  "                         [MUST be  CPU_WORD_SIZE_08   8-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_16  16-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_32  32-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_64  64-bit alignment]"

#elif  ((CPU_CFG_DATA_SIZE_MAX != CPU_WORD_SIZE_08) && \
        (CPU_CFG_DATA_SIZE_MAX != CPU_WORD_SIZE_16) && \
        (CPU_CFG_DATA_SIZE_MAX != CPU_WORD_SIZE_32) && \
        (CPU_CFG_DATA_SIZE_MAX != CPU_WORD_SIZE_64))
#error  "CPU_CFG_DATA_SIZE_MAX    illegally #define'd in 'cpu.h'               "
#error  "                         [MUST be  CPU_WORD_SIZE_08   8-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_16  16-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_32  32-bit alignment]"
#error  "                         [     ||  CPU_WORD_SIZE_64  64-bit alignment]"
#endif



#if     (CPU_CFG_DATA_SIZE_MAX < CPU_CFG_DATA_SIZE)
#error  "CPU_CFG_DATA_SIZE_MAX    illegally #define'd in 'cpu.h' "
#error  "                         [MUST be  >= CPU_CFG_DATA_SIZE]"
#endif




#ifndef  CPU_CFG_ENDIAN_TYPE
#error  "CPU_CFG_ENDIAN_TYPE            not #define'd in 'cpu.h'   "
#error  "                         [MUST be  CPU_ENDIAN_TYPE_BIG   ]"
#error  "                         [     ||  CPU_ENDIAN_TYPE_LITTLE]"

#elif  ((CPU_CFG_ENDIAN_TYPE != CPU_ENDIAN_TYPE_BIG   ) && \
        (CPU_CFG_ENDIAN_TYPE != CPU_ENDIAN_TYPE_LITTLE))
#error  "CPU_CFG_ENDIAN_TYPE      illegally #define'd in 'cpu.h'   "
#error  "                         [MUST be  CPU_ENDIAN_TYPE_BIG   ]"
#error  "                         [     ||  CPU_ENDIAN_TYPE_LITTLE]"
#endif




#ifndef  CPU_CFG_STK_GROWTH
#error  "CPU_CFG_STK_GROWTH             not #define'd in 'cpu.h'    "
#error  "                         [MUST be  CPU_STK_GROWTH_LO_TO_HI]"
#error  "                         [     ||  CPU_STK_GROWTH_HI_TO_LO]"

#elif  ((CPU_CFG_STK_GROWTH != CPU_STK_GROWTH_LO_TO_HI) && \
        (CPU_CFG_STK_GROWTH != CPU_STK_GROWTH_HI_TO_LO))
#error  "CPU_CFG_STK_GROWTH       illegally #define'd in 'cpu.h'    "
#error  "                         [MUST be  CPU_STK_GROWTH_LO_TO_HI]"
#error  "                         [     ||  CPU_STK_GROWTH_HI_TO_LO]"
#endif




#ifndef  CPU_CFG_CRITICAL_METHOD
#error  "CPU_CFG_CRITICAL_METHOD        not #define'd in 'cpu.h'             "
#error  "                         [MUST be  CPU_CRITICAL_METHOD_INT_DIS_EN  ]"
#error  "                         [     ||  CPU_CRITICAL_METHOD_STATUS_STK  ]"
#error  "                         [     ||  CPU_CRITICAL_METHOD_STATUS_LOCAL]"

#elif  ((CPU_CFG_CRITICAL_METHOD != CPU_CRITICAL_METHOD_INT_DIS_EN  ) && \
        (CPU_CFG_CRITICAL_METHOD != CPU_CRITICAL_METHOD_STATUS_STK  ) && \
        (CPU_CFG_CRITICAL_METHOD != CPU_CRITICAL_METHOD_STATUS_LOCAL))
#error  "CPU_CFG_CRITICAL_METHOD  illegally #define'd in 'cpu.h'             "
#error  "                         [MUST be  CPU_CRITICAL_METHOD_INT_DIS_EN  ]"
#error  "                         [     ||  CPU_CRITICAL_METHOD_STATUS_STK  ]"
#error  "                         [     ||  CPU_CRITICAL_METHOD_STATUS_LOCAL]"
#endif


/*
*********************************************************************************************************
*                                             MODULE END
*
* Note(s) : (1) See 'cpu.h  MODULE'.
*********************************************************************************************************
*/

#ifdef __cplusplus
}
#endif

#endif                                                          /* End of CPU module include.                           */


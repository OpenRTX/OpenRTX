/*
*********************************************************************************************************
*                                               uC/LIB
*                                       Custom Library Modules
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
*                                     STANDARD MEMORY OPERATIONS
*
* Filename : lib_mem.h
* Version  : V1.39.00
*********************************************************************************************************
* Note(s)  : (1) NO compiler-supplied standard library functions are used in library or product software.
*
*                (a) ALL standard library functions are implemented in the custom library modules :
*
*                    (1) \<Custom Library Directory>\lib_*.*
*
*                    (2) \<Custom Library Directory>\Ports\<cpu>\<compiler>\lib*_a.*
*
*                          where
*                                  <Custom Library Directory>      directory path for custom library software
*                                  <cpu>                           directory name for specific processor (CPU)
*                                  <compiler>                      directory name for specific compiler
*
*                (b) Product-specific library functions are implemented in individual products.
*
*            (2) Assumes the following versions (or more recent) of software modules are included in
*                the project build :
*
*                (a) uC/CPU V1.27
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) This memory library header file is protected from multiple pre-processor inclusion through
*               use of the memory library module present pre-processor macro definition.
*********************************************************************************************************
*/

#ifndef  LIB_MEM_MODULE_PRESENT                                 /* See Note #1.                                         */
#define  LIB_MEM_MODULE_PRESENT


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*
* Note(s) : (1) The custom library software files are located in the following directories :
*
*               (a) \<Your Product Application>\lib_cfg.h
*
*               (b) \<Custom Library Directory>\lib_*.*
*
*                       where
*                               <Your Product Application>      directory path for Your Product's Application
*                               <Custom Library Directory>      directory path for custom library software
*
*           (2) CPU-configuration  software files are located in the following directories :
*
*               (a) \<CPU-Compiler Directory>\cpu_*.*
*               (b) \<CPU-Compiler Directory>\<cpu>\<compiler>\cpu*.*
*
*                       where
*                               <CPU-Compiler Directory>        directory path for common CPU-compiler software
*                               <cpu>                           directory name for specific processor (CPU)
*                               <compiler>                      directory name for specific compiler
*
*           (3) Compiler MUST be configured to include as additional include path directories :
*
*               (a) '\<Your Product Application>\' directory                            See Note #1a
*
*               (b) '\<Custom Library Directory>\' directory                            See Note #1b
*
*               (c) (1) '\<CPU-Compiler Directory>\'                  directory         See Note #2a
*                   (2) '\<CPU-Compiler Directory>\<cpu>\<compiler>\' directory         See Note #2b
*
*           (4) NO compiler-supplied standard library functions SHOULD be used.
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <cpu_core.h>

#include  <lib_def.h>
#include  <lib_cfg.h>


/*
*********************************************************************************************************
*                                               EXTERNS
*********************************************************************************************************
*/

#ifdef   LIB_MEM_MODULE
#define  LIB_MEM_EXT
#else
#define  LIB_MEM_EXT  extern
#endif


/*
*********************************************************************************************************
*                                               DEFINES
*********************************************************************************************************
*/

#define  LIB_MEM_PADDING_ALIGN_NONE                       1u

#define  LIB_MEM_BLK_QTY_UNLIMITED                        0u


/*
*********************************************************************************************************
*                                        DEFAULT CONFIGURATION
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                             MEMORY LIBRARY ARGUMENT CHECK CONFIGURATION
*
* Note(s) : (1) Configure LIB_MEM_CFG_ARG_CHK_EXT_EN to enable/disable the memory library suite external
*               argument check feature :
*
*               (a) When ENABLED,      arguments received from any port interface provided by the developer
*                   or application are checked/validated.
*
*               (b) When DISABLED, NO  arguments received from any port interface provided by the developer
*                   or application are checked/validated.
*********************************************************************************************************
*/

                                                                /* Cfg external argument check feature (see Note #1) :  */
#ifndef  LIB_MEM_CFG_ARG_CHK_EXT_EN
#define  LIB_MEM_CFG_ARG_CHK_EXT_EN     DEF_DISABLED
                                                                /* DEF_DISABLED     Argument check DISABLED             */
                                                                /* DEF_ENABLED      Argument check ENABLED              */
#endif


/*
*********************************************************************************************************
*                         MEMORY LIBRARY ASSEMBLY OPTIMIZATION CONFIGURATION
*
* Note(s) : (1) Configure LIB_MEM_CFG_OPTIMIZE_ASM_EN to enable/disable assembly-optimized memory
*               functions.
*********************************************************************************************************
*/

                                                                /* Cfg assembly-optimized function(s) [see Note #1] :   */
#ifndef  LIB_MEM_CFG_OPTIMIZE_ASM_EN
#define  LIB_MEM_CFG_OPTIMIZE_ASM_EN    DEF_DISABLED
                                                                /* DEF_DISABLED     Assembly-optimized fnct(s) DISABLED */
                                                                /* DEF_ENABLED      Assembly-optimized fnct(s) ENABLED  */
#endif


/*
*********************************************************************************************************
*                          MEMORY ALLOCATION DEBUG INFORMATION CONFIGURATION
*
* Note(s) : (1) Configure LIB_MEM_CFG_DBG_INFO_EN to enable/disable debug information associated to each
*               segment allocation.
*********************************************************************************************************
*/

#ifndef  LIB_MEM_CFG_DBG_INFO_EN
#define  LIB_MEM_CFG_DBG_INFO_EN         DEF_DISABLED
#endif


/*
*********************************************************************************************************
*                                  HEAP PADDING ALIGN CONFIGURATION
*
* Note(s) : (1) Configure LIB_MEM_CFG_HEAP_PADDING_ALIGN to set the padding alignment of any buffer
*               allocated from the heap.
*********************************************************************************************************
*/

#ifndef  LIB_MEM_CFG_HEAP_PADDING_ALIGN
#define  LIB_MEM_CFG_HEAP_PADDING_ALIGN  LIB_MEM_PADDING_ALIGN_NONE
#endif


/*
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            LIB MEM TYPE
*
* Note(s) : (1) 'LIB_MEM_TYPE' declared as 'CPU_INT32U' & all 'LIB_MEM_TYPE's #define'd with large, non-trivial
*               values to trap & discard invalid/corrupted library memory objects based on 'LIB_MEM_TYPE'.
*********************************************************************************************************
*/

typedef  CPU_INT32U  LIB_MEM_TYPE;


/*
*********************************************************************************************************
*                                MEMORY POOL BLOCK QUANTITY DATA TYPE
*********************************************************************************************************
*/

typedef  CPU_SIZE_T  MEM_POOL_BLK_QTY;


/*
*********************************************************************************************************
*                                      MEMORY POOL TABLE IX TYPE
*********************************************************************************************************
*/

typedef  MEM_POOL_BLK_QTY  MEM_POOL_IX;


/*
*********************************************************************************************************
*                              MEMORY ALLOCATION TRACKING INFO DATA TYPE
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_DBG_INFO_EN  == DEF_ENABLED)
typedef  struct  mem_alloc_info  MEM_ALLOC_INFO;

struct  mem_alloc_info  {                                       /* ------------------ MEM ALLOC INFO ------------------ */
    const  CPU_CHAR        *NamePtr;                            /* Ptr to name.                                         */
           CPU_SIZE_T       Size;                               /* Total alloc'd size, in bytes.                        */
           MEM_ALLOC_INFO  *NextPtr;                            /* Ptr to next alloc info in list.                      */
};
#endif


/*
*********************************************************************************************************
*                                     MEMORY SEGMENTS DATA TYPES
*********************************************************************************************************
*/

typedef  struct  mem_seg  MEM_SEG;                              /* --------------------- SEG DATA --------------------- */

struct mem_seg {
           CPU_ADDR         AddrBase;                           /* Seg start addr.                                      */
           CPU_ADDR         AddrEnd;                            /* Seg end addr (last addr).                            */
           CPU_ADDR         AddrNext;                           /* Next free addr.                                      */

           MEM_SEG         *NextPtr;                            /* Ptr to next seg.                                     */

           CPU_SIZE_T       PaddingAlign;                       /* Padding alignment in byte.                           */

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
    const  CPU_CHAR        *NamePtr;                            /* Ptr to seg name.                                     */
           MEM_ALLOC_INFO  *AllocInfoHeadPtr;                   /* Ptr to head of alloc info struct list.               */
#endif
};

typedef  struct  mem_seg_info {                                 /* --------------------- SEG INFO --------------------- */
    CPU_SIZE_T  UsedSize;                                       /* Used size, independently of alignment.               */
    CPU_SIZE_T  TotalSize;                                      /* Total seg capacity, in octets.                       */

    CPU_ADDR    AddrBase;
    CPU_ADDR    AddrNextAlloc;                                  /* Next aligned address, 0 if none available.           */
} MEM_SEG_INFO;


/*
*********************************************************************************************************
*                                    (STATIC) MEMORY POOL DATA TYPES
*
* Note(s) : (1) Free static memory pool blocks are indexed in the 'BlkFreeTbl' table. Newly freed blocks
*               are added at the first available position in the table and blocks are retrieved from the
*               last occupied position, in a LIFO fashion.
*
*                                 /-------------------------------\
*                                 |/------------\                 |
*                    BlkFreeTbl   ||  Start     v                 v              End
*                    /--------\   ||  /--------------------------------------------\
*                    |p_free_1|---/|  |        |        |        |        |        |
*                    |--------|    |  \--------------------------------------------/
*                    |p_free_2|----/   ^                                  |        |
*                    |--------|        |                                  |__Blk___|
*                    |p_free_3|--------/ (Next block to be retrieved.)       Size
*                    |--------|
*                    |        |<-------- (Next block to be freed.)
*                    \--------/
*
*********************************************************************************************************
*/

                                                                /* --------------------- MEM POOL --------------------- */
typedef  struct  mem_pool {
    void               *PoolAddrStart;                          /* Ptr   to start of mem seg for mem pool blks.         */
    void               *PoolAddrEnd;                            /* Ptr   to end   of mem seg for mem pool blks.         */
    MEM_POOL_BLK_QTY    BlkNbr;                                 /* Nbr   of mem pool   blks.                            */
    CPU_SIZE_T          BlkSize;                                /* Size  of mem pool   blks (in octets).                */
    void              **BlkFreeTbl;                             /* Tbl of free mem pool blks.                           */
    CPU_SIZE_T          BlkFreeTblIx;                           /* Ix of next free blk free tbl entry.                  */
} MEM_POOL;


/*
*********************************************************************************************************
*                                     DYNAMIC MEMORY POOL DATA TYPE
*
* Note(s) : (1) Dynamic memory pool blocks are not indexed in a table. Only freed blocks are linked using
*               a singly linked list, in a LIFO fashion; newly freed blocks are inserted at the head of the
*               list and blocks are also retrieved from the head of the list.
*
*           (2) Pointers to the next block are only present when a block is free, using the first location
*               in the allocated memory block. The user of dynamic memory pool must not assume his data
*               will not be overwritten when a block is freed.
*
*                                   /----------------\
*                    /----------\   |  /----------\  |    /----------\   /----------\
*       BlkFreePtr-->|(NextPtr) |---/  |          |  \--->|(NextPtr) |-->|(NextPtr) |--> DEF_NULL
*                    |----------|      |  Blk in  |       |----------|   |----------|
*                    |          |      |   use    |       |          |   |          |
*                    |          |      |          |       |          |   |          |
*                    \----------/      \----------/       \----------/   \----------/
*
*********************************************************************************************************
*/

typedef  struct  mem_dyn_pool {                                 /* ---------------- DYN MEM POOL DATA ----------------- */
           MEM_SEG     *PoolSegPtr;                             /* Mem pool from which blks are alloc'd.                */
           CPU_SIZE_T   BlkSize;                                /* Size of pool blks, in octets.                        */
           CPU_SIZE_T   BlkAlign;                               /* Align req'd for blks, in octets.                     */
           CPU_SIZE_T   BlkPaddingAlign;                        /* Padding alignment in bytes for this mem seg.         */
           void        *BlkFreePtr;                             /* Ptr to first free blk.                               */

           CPU_SIZE_T   BlkQtyMax;                              /* Max qty of blk in dyn mem pool. 0 = unlimited.       */
           CPU_SIZE_T   BlkAllocCnt;                            /* Cnt of alloc blk.                                    */

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
    const  CPU_CHAR    *NamePtr;                                /* Ptr to mem pool name.                                */
#endif
} MEM_DYN_POOL;


/*
*********************************************************************************************************
*                                          GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                              MACRO'S
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      MEMORY DATA VALUE MACRO'S
*
* Note(s) : (1) (a) Some variables & variable buffers to pass & receive data values MUST start on appropriate
*                   CPU word-aligned addresses.  This is required because most word-aligned processors are more
*                   efficient & may even REQUIRE that multi-octet words start on CPU word-aligned addresses.
*
*                   (1) For 16-bit word-aligned processors, this means that
*
*                           all 16- & 32-bit words MUST start on addresses that are multiples of 2 octets
*
*                   (2) For 32-bit word-aligned processors, this means that
*
*                           all 16-bit       words MUST start on addresses that are multiples of 2 octets
*                           all 32-bit       words MUST start on addresses that are multiples of 4 octets
*
*               (b) However, some data values macro's appropriately access data values from any CPU addresses,
*                   word-aligned or not.  Thus for processors that require data word alignment, data words can
*                   be accessed to/from any CPU address, word-aligned or not, without generating data-word-
*                   alignment exceptions/faults.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      ENDIAN WORD ORDER MACRO'S
*
* Description : Convert data values to & from big-, little, or host-endian CPU word order.
*
* Argument(s) : val       Data value to convert (see Notes #1 & #2).
*
* Return(s)   : Converted data value (see Notes #1 & #2).
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Convert data values to the desired data-word order :
*
*                       MEM_VAL_BIG_TO_LITTLE_xx()      Convert big-        endian data values
*                                                            to little-     endian data values
*                       MEM_VAL_LITTLE_TO_BIG_xx()      Convert little-     endian data values
*                                                            to big-        endian data values
*                       MEM_VAL_xxx_TO_HOST_xx()        Convert big-/little-endian data values
*                                                            to host-       endian data values
*                       MEM_VAL_HOST_TO_xxx_xx()        Convert host-       endian data values
*                                                            to big-/little-endian data values
*
*                   See also 'cpu.h  CPU WORD CONFIGURATION  Note #2'.
*
*               (2) 'val' data value to convert & any variable to receive the returned conversion MUST
*                   start on appropriate CPU word-aligned addresses.
*
*                   See also 'MEMORY DATA VALUE MACRO'S  Note #1a'.
*
*               (3) MEM_VAL_COPY_xxx() macro's are more efficient than generic endian word order macro's &
*                   are also independent of CPU data-word-alignment & SHOULD be used whenever possible.
*
*                   See also 'MEM_VAL_COPY_GET_xxx()  Note #4'
*                          & 'MEM_VAL_COPY_SET_xxx()  Note #4'.
*
*               (4) Generic endian word order macro's are NOT atomic operations & MUST NOT be used on any
*                   non-static (i.e. volatile) variables, registers, hardware, etc.; without the caller of
*                   the macro's providing some form of additional protection (e.g. mutual exclusion).
*
*               (5) The 'CPU_CFG_ENDIAN_TYPE' pre-processor 'else'-conditional code SHOULD never be compiled/
*                   linked since each 'cpu.h' SHOULD ensure that the CPU data-word-memory order configuration
*                   constant (CPU_CFG_ENDIAN_TYPE) is configured with an appropriate data-word-memory order
*                   value (see 'cpu.h  CPU WORD CONFIGURATION  Note #2').  The 'else'-conditional code is
*                   included as an extra precaution in case 'cpu.h' is incorrectly configured.
*********************************************************************************************************
*/

#if    ((CPU_CFG_DATA_SIZE == CPU_WORD_SIZE_64) || \
        (CPU_CFG_DATA_SIZE == CPU_WORD_SIZE_32))

#define  MEM_VAL_BIG_TO_LITTLE_16(val)        ((CPU_INT16U)(((CPU_INT16U)((((CPU_INT16U)(val)) & (CPU_INT16U)    0xFF00u) >> (1u * DEF_OCTET_NBR_BITS))) | \
                                                            ((CPU_INT16U)((((CPU_INT16U)(val)) & (CPU_INT16U)    0x00FFu) << (1u * DEF_OCTET_NBR_BITS)))))

#define  MEM_VAL_BIG_TO_LITTLE_32(val)        ((CPU_INT32U)(((CPU_INT32U)((((CPU_INT32U)(val)) & (CPU_INT32U)0xFF000000u) >> (3u * DEF_OCTET_NBR_BITS))) | \
                                                            ((CPU_INT32U)((((CPU_INT32U)(val)) & (CPU_INT32U)0x00FF0000u) >> (1u * DEF_OCTET_NBR_BITS))) | \
                                                            ((CPU_INT32U)((((CPU_INT32U)(val)) & (CPU_INT32U)0x0000FF00u) << (1u * DEF_OCTET_NBR_BITS))) | \
                                                            ((CPU_INT32U)((((CPU_INT32U)(val)) & (CPU_INT32U)0x000000FFu) << (3u * DEF_OCTET_NBR_BITS)))))

#elif   (CPU_CFG_DATA_SIZE == CPU_WORD_SIZE_16)

#define  MEM_VAL_BIG_TO_LITTLE_16(val)        ((CPU_INT16U)(((CPU_INT16U)((((CPU_INT16U)(val)) & (CPU_INT16U)    0xFF00u) >> (1u * DEF_OCTET_NBR_BITS))) | \
                                                            ((CPU_INT16U)((((CPU_INT16U)(val)) & (CPU_INT16U)    0x00FFu) << (1u * DEF_OCTET_NBR_BITS)))))

#define  MEM_VAL_BIG_TO_LITTLE_32(val)        ((CPU_INT32U)(((CPU_INT32U)((((CPU_INT32U)(val)) & (CPU_INT32U)0xFF000000u) >> (1u * DEF_OCTET_NBR_BITS))) | \
                                                            ((CPU_INT32U)((((CPU_INT32U)(val)) & (CPU_INT32U)0x00FF0000u) << (1u * DEF_OCTET_NBR_BITS))) | \
                                                            ((CPU_INT32U)((((CPU_INT32U)(val)) & (CPU_INT32U)0x0000FF00u) >> (1u * DEF_OCTET_NBR_BITS))) | \
                                                            ((CPU_INT32U)((((CPU_INT32U)(val)) & (CPU_INT32U)0x000000FFu) << (1u * DEF_OCTET_NBR_BITS)))))

#else

#define  MEM_VAL_BIG_TO_LITTLE_16(val)                                                  (val)
#define  MEM_VAL_BIG_TO_LITTLE_32(val)                                                  (val)

#endif


#define  MEM_VAL_LITTLE_TO_BIG_16(val)                          MEM_VAL_BIG_TO_LITTLE_16(val)
#define  MEM_VAL_LITTLE_TO_BIG_32(val)                          MEM_VAL_BIG_TO_LITTLE_32(val)



#if     (CPU_CFG_ENDIAN_TYPE == CPU_ENDIAN_TYPE_BIG)

#define  MEM_VAL_BIG_TO_HOST_16(val)                                                    (val)
#define  MEM_VAL_BIG_TO_HOST_32(val)                                                    (val)
#define  MEM_VAL_LITTLE_TO_HOST_16(val)                         MEM_VAL_LITTLE_TO_BIG_16(val)
#define  MEM_VAL_LITTLE_TO_HOST_32(val)                         MEM_VAL_LITTLE_TO_BIG_32(val)

#elif   (CPU_CFG_ENDIAN_TYPE == CPU_ENDIAN_TYPE_LITTLE)

#define  MEM_VAL_BIG_TO_HOST_16(val)                            MEM_VAL_BIG_TO_LITTLE_16(val)
#define  MEM_VAL_BIG_TO_HOST_32(val)                            MEM_VAL_BIG_TO_LITTLE_32(val)
#define  MEM_VAL_LITTLE_TO_HOST_16(val)                                                 (val)
#define  MEM_VAL_LITTLE_TO_HOST_32(val)                                                 (val)

#else                                                           /* See Note #5.                                         */

#error  "CPU_CFG_ENDIAN_TYPE  illegally #defined in 'cpu.h'      "
#error  "                     [See 'cpu.h  CONFIGURATION ERRORS']"

#endif


#define  MEM_VAL_HOST_TO_BIG_16(val)                            MEM_VAL_BIG_TO_HOST_16(val)
#define  MEM_VAL_HOST_TO_BIG_32(val)                            MEM_VAL_BIG_TO_HOST_32(val)
#define  MEM_VAL_HOST_TO_LITTLE_16(val)                         MEM_VAL_LITTLE_TO_HOST_16(val)
#define  MEM_VAL_HOST_TO_LITTLE_32(val)                         MEM_VAL_LITTLE_TO_HOST_32(val)


/*
*********************************************************************************************************
*                                          MEM_VAL_GET_xxx()
*
* Description : Decode data values from any CPU memory address.
*
* Argument(s) : addr        Lowest CPU memory address of data value to decode (see Notes #2 & #3a).
*
* Return(s)   : Decoded data value from CPU memory address (see Notes #1 & #3b).
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Decode data values based on the values' data-word order in CPU memory :
*
*                       MEM_VAL_GET_xxx_BIG()           Decode big-   endian data values -- data words' most
*                                                           significant octet @ lowest memory address
*                       MEM_VAL_GET_xxx_LITTLE()        Decode little-endian data values -- data words' least
*                                                           significant octet @ lowest memory address
*                       MEM_VAL_GET_xxx()               Decode data values using CPU's native or configured
*                                                           data-word order
*
*                   See also 'cpu.h  CPU WORD CONFIGURATION  Note #2'.
*
*               (2) CPU memory addresses/pointers NOT checked for NULL.
*
*               (3) (a) MEM_VAL_GET_xxx() macro's decode data values without regard to CPU word-aligned addresses.
*                       Thus for processors that require data word alignment, data words can be decoded from any
*                       CPU address, word-aligned or not, without generating data-word-alignment exceptions/faults.
*
*                   (b) However, any variable to receive the returned data value MUST start on an appropriate CPU
*                       word-aligned address.
*
*                   See also 'MEMORY DATA VALUE MACRO'S  Note #1'.
*
*               (4) MEM_VAL_COPY_GET_xxx() macro's are more efficient than MEM_VAL_GET_xxx() macro's & are
*                   also independent of CPU data-word-alignment & SHOULD be used whenever possible.
*
*                   See also 'MEM_VAL_COPY_GET_xxx()  Note #4'.
*
*               (5) MEM_VAL_GET_xxx() macro's are NOT atomic operations & MUST NOT be used on any non-static
*                   (i.e. volatile) variables, registers, hardware, etc.; without the caller of the macro's
*                   providing some form of additional protection (e.g. mutual exclusion).
*
*               (6) The 'CPU_CFG_ENDIAN_TYPE' pre-processor 'else'-conditional code SHOULD never be compiled/
*                   linked since each 'cpu.h' SHOULD ensure that the CPU data-word-memory order configuration
*                   constant (CPU_CFG_ENDIAN_TYPE) is configured with an appropriate data-word-memory order
*                   value (see 'cpu.h  CPU WORD CONFIGURATION  Note #2').  The 'else'-conditional code is
*                   included as an extra precaution in case 'cpu.h' is incorrectly configured.
*********************************************************************************************************
*/

#define  MEM_VAL_GET_INT08U_BIG(addr)           ((CPU_INT08U) ((CPU_INT08U)(((CPU_INT08U)(*(((CPU_INT08U *)(addr)) + 0))) << (0u * DEF_OCTET_NBR_BITS))))

#define  MEM_VAL_GET_INT16U_BIG(addr)           ((CPU_INT16U)(((CPU_INT16U)(((CPU_INT16U)(*(((CPU_INT08U *)(addr)) + 0))) << (1u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT16U)(((CPU_INT16U)(*(((CPU_INT08U *)(addr)) + 1))) << (0u * DEF_OCTET_NBR_BITS)))))

#define  MEM_VAL_GET_INT24U_BIG(addr)           ((CPU_INT32U)(((CPU_INT32U)(((CPU_INT32U)(*(((CPU_INT08U *)(addr)) + 0))) << (2u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT32U)(((CPU_INT32U)(*(((CPU_INT08U *)(addr)) + 1))) << (1u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT32U)(((CPU_INT32U)(*(((CPU_INT08U *)(addr)) + 2))) << (0u * DEF_OCTET_NBR_BITS)))))

#define  MEM_VAL_GET_INT32U_BIG(addr)           ((CPU_INT32U)(((CPU_INT32U)(((CPU_INT32U)(*(((CPU_INT08U *)(addr)) + 0))) << (3u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT32U)(((CPU_INT32U)(*(((CPU_INT08U *)(addr)) + 1))) << (2u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT32U)(((CPU_INT32U)(*(((CPU_INT08U *)(addr)) + 2))) << (1u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT32U)(((CPU_INT32U)(*(((CPU_INT08U *)(addr)) + 3))) << (0u * DEF_OCTET_NBR_BITS)))))



#define  MEM_VAL_GET_INT08U_LITTLE(addr)        ((CPU_INT08U) ((CPU_INT08U)(((CPU_INT08U)(*(((CPU_INT08U *)(addr)) + 0))) << (0u * DEF_OCTET_NBR_BITS))))

#define  MEM_VAL_GET_INT16U_LITTLE(addr)        ((CPU_INT16U)(((CPU_INT16U)(((CPU_INT16U)(*(((CPU_INT08U *)(addr)) + 0))) << (0u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT16U)(((CPU_INT16U)(*(((CPU_INT08U *)(addr)) + 1))) << (1u * DEF_OCTET_NBR_BITS)))))

#define  MEM_VAL_GET_INT24U_LITTLE(addr)        ((CPU_INT32U)(((CPU_INT32U)(((CPU_INT32U)(*(((CPU_INT08U *)(addr)) + 0))) << (0u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT32U)(((CPU_INT32U)(*(((CPU_INT08U *)(addr)) + 1))) << (1u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT32U)(((CPU_INT32U)(*(((CPU_INT08U *)(addr)) + 2))) << (2u * DEF_OCTET_NBR_BITS)))))

#define  MEM_VAL_GET_INT32U_LITTLE(addr)        ((CPU_INT32U)(((CPU_INT32U)(((CPU_INT32U)(*(((CPU_INT08U *)(addr)) + 0))) << (0u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT32U)(((CPU_INT32U)(*(((CPU_INT08U *)(addr)) + 1))) << (1u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT32U)(((CPU_INT32U)(*(((CPU_INT08U *)(addr)) + 2))) << (2u * DEF_OCTET_NBR_BITS))) + \
                                                              ((CPU_INT32U)(((CPU_INT32U)(*(((CPU_INT08U *)(addr)) + 3))) << (3u * DEF_OCTET_NBR_BITS)))))



#if     (CPU_CFG_ENDIAN_TYPE == CPU_ENDIAN_TYPE_BIG)

#define  MEM_VAL_GET_INT08U(addr)                               MEM_VAL_GET_INT08U_BIG(addr)
#define  MEM_VAL_GET_INT16U(addr)                               MEM_VAL_GET_INT16U_BIG(addr)
#define  MEM_VAL_GET_INT24U(addr)                               MEM_VAL_GET_INT24U_BIG(addr)
#define  MEM_VAL_GET_INT32U(addr)                               MEM_VAL_GET_INT32U_BIG(addr)

#elif   (CPU_CFG_ENDIAN_TYPE == CPU_ENDIAN_TYPE_LITTLE)

#define  MEM_VAL_GET_INT08U(addr)                               MEM_VAL_GET_INT08U_LITTLE(addr)
#define  MEM_VAL_GET_INT16U(addr)                               MEM_VAL_GET_INT16U_LITTLE(addr)
#define  MEM_VAL_GET_INT24U(addr)                               MEM_VAL_GET_INT24U_LITTLE(addr)
#define  MEM_VAL_GET_INT32U(addr)                               MEM_VAL_GET_INT32U_LITTLE(addr)

#else                                                           /* See Note #6.                                         */

#error  "CPU_CFG_ENDIAN_TYPE  illegally #defined in 'cpu.h'      "
#error  "                     [See 'cpu.h  CONFIGURATION ERRORS']"

#endif


/*
*********************************************************************************************************
*                                          MEM_VAL_SET_xxx()
*
* Description : Encode data values to any CPU memory address.
*
* Argument(s) : addr        Lowest CPU memory address to encode data value (see Notes #2 & #3a).
*
*               val         Data value to encode (see Notes #1 & #3b).
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Encode data values into CPU memory based on the values' data-word order :
*
*                       MEM_VAL_SET_xxx_BIG()           Encode big-   endian data values -- data words' most
*                                                           significant octet @ lowest memory address
*                       MEM_VAL_SET_xxx_LITTLE()        Encode little-endian data values -- data words' least
*                                                           significant octet @ lowest memory address
*                       MEM_VAL_SET_xxx()               Encode data values using CPU's native or configured
*                                                           data-word order
*
*                   See also 'cpu.h  CPU WORD CONFIGURATION  Note #2'.
*
*               (2) CPU memory addresses/pointers NOT checked for NULL.
*
*               (3) (a) MEM_VAL_SET_xxx() macro's encode data values without regard to CPU word-aligned addresses.
*                       Thus for processors that require data word alignment, data words can be encoded to any
*                       CPU address, word-aligned or not, without generating data-word-alignment exceptions/faults.
*
*                   (b) However, 'val' data value to encode MUST start on an appropriate CPU word-aligned address.
*
*                   See also 'MEMORY DATA VALUE MACRO'S  Note #1'.
*
*               (4) MEM_VAL_COPY_SET_xxx() macro's are more efficient than MEM_VAL_SET_xxx() macro's & are
*                   also independent of CPU data-word-alignment & SHOULD be used whenever possible.
*
*                   See also 'MEM_VAL_COPY_SET_xxx()  Note #4'.
*
*               (5) MEM_VAL_SET_xxx() macro's are NOT atomic operations & MUST NOT be used on any non-static
*                   (i.e. volatile) variables, registers, hardware, etc.; without the caller of the macro's
*                   providing some form of additional protection (e.g. mutual exclusion).
*
*               (6) The 'CPU_CFG_ENDIAN_TYPE' pre-processor 'else'-conditional code SHOULD never be compiled/
*                   linked since each 'cpu.h' SHOULD ensure that the CPU data-word-memory order configuration
*                   constant (CPU_CFG_ENDIAN_TYPE) is configured with an appropriate data-word-memory order
*                   value (see 'cpu.h  CPU WORD CONFIGURATION  Note #2').  The 'else'-conditional code is
*                   included as an extra precaution in case 'cpu.h' is incorrectly configured.
*********************************************************************************************************
*/

#define  MEM_VAL_SET_INT08U_BIG(addr, val)                     do { (*(((CPU_INT08U *)(addr)) + 0)) = ((CPU_INT08U)((((CPU_INT08U)(val)) & (CPU_INT08U)      0xFFu) >> (0u * DEF_OCTET_NBR_BITS))); } while (0)

#define  MEM_VAL_SET_INT16U_BIG(addr, val)                     do { (*(((CPU_INT08U *)(addr)) + 0)) = ((CPU_INT08U)((((CPU_INT16U)(val)) & (CPU_INT16U)    0xFF00u) >> (1u * DEF_OCTET_NBR_BITS))); \
                                                                    (*(((CPU_INT08U *)(addr)) + 1)) = ((CPU_INT08U)((((CPU_INT16U)(val)) & (CPU_INT16U)    0x00FFu) >> (0u * DEF_OCTET_NBR_BITS))); } while (0)

#define  MEM_VAL_SET_INT24U_BIG(addr, val)                     do { (*(((CPU_INT08U *)(addr)) + 0)) = ((CPU_INT08U)((((CPU_INT32U)(val)) & (CPU_INT32U)  0xFF0000u) >> (2u * DEF_OCTET_NBR_BITS))); \
                                                                    (*(((CPU_INT08U *)(addr)) + 1)) = ((CPU_INT08U)((((CPU_INT32U)(val)) & (CPU_INT32U)  0x00FF00u) >> (1u * DEF_OCTET_NBR_BITS))); \
                                                                    (*(((CPU_INT08U *)(addr)) + 2)) = ((CPU_INT08U)((((CPU_INT32U)(val)) & (CPU_INT32U)  0x0000FFu) >> (0u * DEF_OCTET_NBR_BITS))); } while (0)

#define  MEM_VAL_SET_INT32U_BIG(addr, val)                     do { (*(((CPU_INT08U *)(addr)) + 0)) = ((CPU_INT08U)((((CPU_INT32U)(val)) & (CPU_INT32U)0xFF000000u) >> (3u * DEF_OCTET_NBR_BITS))); \
                                                                    (*(((CPU_INT08U *)(addr)) + 1)) = ((CPU_INT08U)((((CPU_INT32U)(val)) & (CPU_INT32U)0x00FF0000u) >> (2u * DEF_OCTET_NBR_BITS))); \
                                                                    (*(((CPU_INT08U *)(addr)) + 2)) = ((CPU_INT08U)((((CPU_INT32U)(val)) & (CPU_INT32U)0x0000FF00u) >> (1u * DEF_OCTET_NBR_BITS))); \
                                                                    (*(((CPU_INT08U *)(addr)) + 3)) = ((CPU_INT08U)((((CPU_INT32U)(val)) & (CPU_INT32U)0x000000FFu) >> (0u * DEF_OCTET_NBR_BITS))); } while (0)



#define  MEM_VAL_SET_INT08U_LITTLE(addr, val)                  do { (*(((CPU_INT08U *)(addr)) + 0)) = ((CPU_INT08U)((((CPU_INT08U)(val)) & (CPU_INT08U)      0xFFu) >> (0u * DEF_OCTET_NBR_BITS))); } while (0)

#define  MEM_VAL_SET_INT16U_LITTLE(addr, val)                  do { (*(((CPU_INT08U *)(addr)) + 0)) = ((CPU_INT08U)((((CPU_INT16U)(val)) & (CPU_INT16U)    0x00FFu) >> (0u * DEF_OCTET_NBR_BITS))); \
                                                                    (*(((CPU_INT08U *)(addr)) + 1)) = ((CPU_INT08U)((((CPU_INT16U)(val)) & (CPU_INT16U)    0xFF00u) >> (1u * DEF_OCTET_NBR_BITS))); } while (0)

#define  MEM_VAL_SET_INT24U_LITTLE(addr, val)                  do { (*(((CPU_INT08U *)(addr)) + 0)) = ((CPU_INT08U)((((CPU_INT32U)(val)) & (CPU_INT32U)  0x0000FFu) >> (0u * DEF_OCTET_NBR_BITS))); \
                                                                    (*(((CPU_INT08U *)(addr)) + 1)) = ((CPU_INT08U)((((CPU_INT32U)(val)) & (CPU_INT32U)  0x00FF00u) >> (1u * DEF_OCTET_NBR_BITS))); \
                                                                    (*(((CPU_INT08U *)(addr)) + 2)) = ((CPU_INT08U)((((CPU_INT32U)(val)) & (CPU_INT32U)  0xFF0000u) >> (2u * DEF_OCTET_NBR_BITS))); } while (0)

#define  MEM_VAL_SET_INT32U_LITTLE(addr, val)                  do { (*(((CPU_INT08U *)(addr)) + 0)) = ((CPU_INT08U)((((CPU_INT32U)(val)) & (CPU_INT32U)0x000000FFu) >> (0u * DEF_OCTET_NBR_BITS))); \
                                                                    (*(((CPU_INT08U *)(addr)) + 1)) = ((CPU_INT08U)((((CPU_INT32U)(val)) & (CPU_INT32U)0x0000FF00u) >> (1u * DEF_OCTET_NBR_BITS))); \
                                                                    (*(((CPU_INT08U *)(addr)) + 2)) = ((CPU_INT08U)((((CPU_INT32U)(val)) & (CPU_INT32U)0x00FF0000u) >> (2u * DEF_OCTET_NBR_BITS))); \
                                                                    (*(((CPU_INT08U *)(addr)) + 3)) = ((CPU_INT08U)((((CPU_INT32U)(val)) & (CPU_INT32U)0xFF000000u) >> (3u * DEF_OCTET_NBR_BITS))); } while (0)



#if     (CPU_CFG_ENDIAN_TYPE == CPU_ENDIAN_TYPE_BIG)

#define  MEM_VAL_SET_INT08U(addr, val)                          MEM_VAL_SET_INT08U_BIG((addr), (val))
#define  MEM_VAL_SET_INT16U(addr, val)                          MEM_VAL_SET_INT16U_BIG((addr), (val))
#define  MEM_VAL_SET_INT24U(addr, val)                          MEM_VAL_SET_INT24U_BIG((addr), (val))
#define  MEM_VAL_SET_INT32U(addr, val)                          MEM_VAL_SET_INT32U_BIG((addr), (val))

#elif   (CPU_CFG_ENDIAN_TYPE == CPU_ENDIAN_TYPE_LITTLE)

#define  MEM_VAL_SET_INT08U(addr, val)                          MEM_VAL_SET_INT08U_LITTLE((addr), (val))
#define  MEM_VAL_SET_INT16U(addr, val)                          MEM_VAL_SET_INT16U_LITTLE((addr), (val))
#define  MEM_VAL_SET_INT24U(addr, val)                          MEM_VAL_SET_INT24U_LITTLE((addr), (val))
#define  MEM_VAL_SET_INT32U(addr, val)                          MEM_VAL_SET_INT32U_LITTLE((addr), (val))

#else                                                           /* See Note #6.                                         */

#error  "CPU_CFG_ENDIAN_TYPE  illegally #defined in 'cpu.h'      "
#error  "                     [See 'cpu.h  CONFIGURATION ERRORS']"

#endif


/*
*********************************************************************************************************
*                                       MEM_VAL_COPY_GET_xxx()
*
* Description : Copy & decode data values from any CPU memory address to any CPU memory address.
*
* Argument(s) : addr_dest       Lowest CPU memory address to copy/decode source address's data value
*                                   (see Notes #2 & #3).
*
*               addr_src        Lowest CPU memory address of data value to copy/decode
*                                   (see Notes #2 & #3).
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Copy/decode data values based on the values' data-word order :
*
*                       MEM_VAL_COPY_GET_xxx_BIG()      Decode big-   endian data values -- data words' most
*                                                           significant octet @ lowest memory address
*                       MEM_VAL_COPY_GET_xxx_LITTLE()   Decode little-endian data values -- data words' least
*                                                           significant octet @ lowest memory address
*                       MEM_VAL_COPY_GET_xxx()          Decode data values using CPU's native or configured
*                                                           data-word order
*
*                   See also 'cpu.h  CPU WORD CONFIGURATION  Note #2'.
*
*               (2) (a) CPU memory addresses/pointers NOT checked for NULL.
*
*                   (b) CPU memory addresses/buffers  NOT checked for overlapping.
*
*                       (1) IEEE Std 1003.1, 2004 Edition, Section 'memcpy() : DESCRIPTION' states that
*                           "copying ... between objects that overlap ... is undefined".
*
*               (3) MEM_VAL_COPY_GET_xxx() macro's copy/decode data values without regard to CPU word-aligned
*                   addresses.  Thus for processors that require data word alignment, data words can be copied/
*                   decoded to/from any CPU address, word-aligned or not, without generating data-word-alignment
*                   exceptions/faults.
*
*               (4) MEM_VAL_COPY_GET_xxx() macro's are more efficient than MEM_VAL_GET_xxx() macro's & are
*                   also independent of CPU data-word-alignment & SHOULD be used whenever possible.
*
*                   See also 'MEM_VAL_GET_xxx()  Note #4'.
*
*               (5) Since octet-order copy/conversion are inverse operations, MEM_VAL_COPY_GET_xxx() &
*                   MEM_VAL_COPY_SET_xxx() macros are inverse, but identical, operations & are provided
*                   in both forms for semantics & consistency.
*
*                   See also 'MEM_VAL_COPY_SET_xxx()  Note #5'.
*
*               (6) MEM_VAL_COPY_GET_xxx() macro's are NOT atomic operations & MUST NOT be used on any non-
*                   static (i.e. volatile) variables, registers, hardware, etc.; without the caller of the
*                   macro's providing some form of additional protection (e.g. mutual exclusion).
*
*               (7) The 'CPU_CFG_ENDIAN_TYPE' pre-processor 'else'-conditional code SHOULD never be compiled/
*                   linked since each 'cpu.h' SHOULD ensure that the CPU data-word-memory order configuration
*                   constant (CPU_CFG_ENDIAN_TYPE) is configured with an appropriate data-word-memory order
*                   value (see 'cpu.h  CPU WORD CONFIGURATION  Note #2').  The 'else'-conditional code is
*                   included as an extra precaution in case 'cpu.h' is incorrectly configured.
*********************************************************************************************************
*/

#if     (CPU_CFG_ENDIAN_TYPE == CPU_ENDIAN_TYPE_BIG)


#define  MEM_VAL_COPY_GET_INT08U_BIG(addr_dest, addr_src)      do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 0)); } while (0)

#define  MEM_VAL_COPY_GET_INT16U_BIG(addr_dest, addr_src)      do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 0)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 1)) = (*(((CPU_INT08U *)(addr_src)) + 1)); } while (0)

#define  MEM_VAL_COPY_GET_INT24U_BIG(addr_dest, addr_src)      do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 0)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 1)) = (*(((CPU_INT08U *)(addr_src)) + 1)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 2)) = (*(((CPU_INT08U *)(addr_src)) + 2)); } while (0)

#define  MEM_VAL_COPY_GET_INT32U_BIG(addr_dest, addr_src)      do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 0)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 1)) = (*(((CPU_INT08U *)(addr_src)) + 1)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 2)) = (*(((CPU_INT08U *)(addr_src)) + 2)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 3)) = (*(((CPU_INT08U *)(addr_src)) + 3)); } while (0)



#define  MEM_VAL_COPY_GET_INT08U_LITTLE(addr_dest, addr_src)   do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 0)); } while (0)

#define  MEM_VAL_COPY_GET_INT16U_LITTLE(addr_dest, addr_src)   do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 1)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 1)) = (*(((CPU_INT08U *)(addr_src)) + 0)); } while (0)

#define  MEM_VAL_COPY_GET_INT24U_LITTLE(addr_dest, addr_src)   do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 2)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 1)) = (*(((CPU_INT08U *)(addr_src)) + 1)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 2)) = (*(((CPU_INT08U *)(addr_src)) + 0)); } while (0)

#define  MEM_VAL_COPY_GET_INT32U_LITTLE(addr_dest, addr_src)   do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 3)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 1)) = (*(((CPU_INT08U *)(addr_src)) + 2)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 2)) = (*(((CPU_INT08U *)(addr_src)) + 1)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 3)) = (*(((CPU_INT08U *)(addr_src)) + 0)); } while (0)


#define  MEM_VAL_COPY_GET_INT08U(addr_dest, addr_src)               MEM_VAL_COPY_GET_INT08U_BIG((addr_dest), (addr_src))
#define  MEM_VAL_COPY_GET_INT16U(addr_dest, addr_src)               MEM_VAL_COPY_GET_INT16U_BIG((addr_dest), (addr_src))
#define  MEM_VAL_COPY_GET_INT24U(addr_dest, addr_src)               MEM_VAL_COPY_GET_INT24U_BIG((addr_dest), (addr_src))
#define  MEM_VAL_COPY_GET_INT32U(addr_dest, addr_src)               MEM_VAL_COPY_GET_INT32U_BIG((addr_dest), (addr_src))


#elif   (CPU_CFG_ENDIAN_TYPE == CPU_ENDIAN_TYPE_LITTLE)


#define  MEM_VAL_COPY_GET_INT08U_BIG(addr_dest, addr_src)      do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 0)); } while (0)

#define  MEM_VAL_COPY_GET_INT16U_BIG(addr_dest, addr_src)      do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 1)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 1)) = (*(((CPU_INT08U *)(addr_src)) + 0)); } while (0)

#define  MEM_VAL_COPY_GET_INT24U_BIG(addr_dest, addr_src)      do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 2)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 1)) = (*(((CPU_INT08U *)(addr_src)) + 1)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 2)) = (*(((CPU_INT08U *)(addr_src)) + 0)); } while (0)

#define  MEM_VAL_COPY_GET_INT32U_BIG(addr_dest, addr_src)      do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 3)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 1)) = (*(((CPU_INT08U *)(addr_src)) + 2)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 2)) = (*(((CPU_INT08U *)(addr_src)) + 1)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 3)) = (*(((CPU_INT08U *)(addr_src)) + 0)); } while (0)



#define  MEM_VAL_COPY_GET_INT08U_LITTLE(addr_dest, addr_src)   do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 0)); } while (0)

#define  MEM_VAL_COPY_GET_INT16U_LITTLE(addr_dest, addr_src)   do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 0)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 1)) = (*(((CPU_INT08U *)(addr_src)) + 1)); } while (0)

#define  MEM_VAL_COPY_GET_INT24U_LITTLE(addr_dest, addr_src)   do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 0)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 1)) = (*(((CPU_INT08U *)(addr_src)) + 1)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 2)) = (*(((CPU_INT08U *)(addr_src)) + 2)); } while (0)

#define  MEM_VAL_COPY_GET_INT32U_LITTLE(addr_dest, addr_src)   do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 0)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 1)) = (*(((CPU_INT08U *)(addr_src)) + 1)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 2)) = (*(((CPU_INT08U *)(addr_src)) + 2)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 3)) = (*(((CPU_INT08U *)(addr_src)) + 3)); } while (0)


#define  MEM_VAL_COPY_GET_INT08U(addr_dest, addr_src)               MEM_VAL_COPY_GET_INT08U_LITTLE((addr_dest), (addr_src))
#define  MEM_VAL_COPY_GET_INT16U(addr_dest, addr_src)               MEM_VAL_COPY_GET_INT16U_LITTLE((addr_dest), (addr_src))
#define  MEM_VAL_COPY_GET_INT24U(addr_dest, addr_src)               MEM_VAL_COPY_GET_INT24U_LITTLE((addr_dest), (addr_src))
#define  MEM_VAL_COPY_GET_INT32U(addr_dest, addr_src)               MEM_VAL_COPY_GET_INT32U_LITTLE((addr_dest), (addr_src))


#else                                                           /* See Note #7.                                         */

#error  "CPU_CFG_ENDIAN_TYPE  illegally #defined in 'cpu.h'      "
#error  "                     [See 'cpu.h  CONFIGURATION ERRORS']"

#endif



/*
*********************************************************************************************************
*                                     MEM_VAL_COPY_GET_INTU_xxx()
*
* Description : Copy & decode data values from any CPU memory address to any CPU memory address for
*                   any sized data values.
*
* Argument(s) : addr_dest       Lowest CPU memory address to copy/decode source address's data value
*                                   (see Notes #2 & #3).
*
*               addr_src        Lowest CPU memory address of data value to copy/decode
*                                   (see Notes #2 & #3).
*
*               val_size        Number of data value octets to copy/decode.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Copy/decode data values based on the values' data-word order :
*
*                       MEM_VAL_COPY_GET_INTU_BIG()     Decode big-   endian data values -- data words' most
*                                                           significant octet @ lowest memory address
*                       MEM_VAL_COPY_GET_INTU_LITTLE()  Decode little-endian data values -- data words' least
*                                                           significant octet @ lowest memory address
*                       MEM_VAL_COPY_GET_INTU()         Decode data values using CPU's native or configured
*                                                           data-word order
*
*                   See also 'cpu.h  CPU WORD CONFIGURATION  Note #2'.
*
*               (2) (a) CPU memory addresses/pointers NOT checked for NULL.
*
*                   (b) CPU memory addresses/buffers  NOT checked for overlapping.
*
*                       (1) IEEE Std 1003.1, 2004 Edition, Section 'memcpy() : DESCRIPTION' states that
*                           "copying ... between objects that overlap ... is undefined".
*
*               (3) MEM_VAL_COPY_GET_INTU_xxx() macro's copy/decode data values without regard to CPU word-
*                   aligned addresses.  Thus for processors that require data word alignment, data words
*                   can be copied/decoded to/from any CPU address, word-aligned or not, without generating
*                   data-word-alignment exceptions/faults.
*
*               (4) MEM_VAL_COPY_GET_xxx() macro's are more efficient than MEM_VAL_COPY_GET_INTU_xxx()
*                   macro's & SHOULD be used whenever possible.
*
*                   See also 'MEM_VAL_COPY_GET_xxx()  Note #4'.
*
*               (5) Since octet-order copy/conversion are inverse operations, MEM_VAL_COPY_GET_INTU_xxx() &
*                   MEM_VAL_COPY_SET_INTU_xxx() macros are inverse, but identical, operations & are provided
*                   in both forms for semantics & consistency.
*
*                   See also 'MEM_VAL_COPY_SET_INTU_xxx()  Note #5'.
*
*               (6) MEM_VAL_COPY_GET_INTU_xxx() macro's are NOT atomic operations & MUST NOT be used on any
*                   non-static (i.e. volatile) variables, registers, hardware, etc.; without the caller of
*                   the macro's providing some form of additional protection (e.g. mutual exclusion).
*
*               (7) MISRA-C 2004 Rule 5.2 states that "identifiers in an inner scope shall not use the same
*                   name as an indentifier in an outer scope, and therefore hide that identifier".
*
*                   Therefore, to avoid possible redeclaration of commonly-used loop counter identifier names,
*                   'i' & 'j', MEM_VAL_COPY_GET_INTU_xxx() loop counter identifier names are prefixed with a
*                   single underscore.
*
*               (8) The 'CPU_CFG_ENDIAN_TYPE' pre-processor 'else'-conditional code SHOULD never be compiled/
*                   linked since each 'cpu.h' SHOULD ensure that the CPU data-word-memory order configuration
*                   constant (CPU_CFG_ENDIAN_TYPE) is configured with an appropriate data-word-memory order
*                   value (see 'cpu.h  CPU WORD CONFIGURATION  Note #2').  The 'else'-conditional code is
*                   included as an extra precaution in case 'cpu.h' is incorrectly configured.
*********************************************************************************************************
*/

#if     (CPU_CFG_ENDIAN_TYPE == CPU_ENDIAN_TYPE_BIG)


#define  MEM_VAL_COPY_GET_INTU_BIG(addr_dest, addr_src, val_size)       do {                                                                                  \
                                                                            CPU_SIZE_T  _i;                                                                   \
                                                                                                                                                              \
                                                                            for (_i = 0; _i < (val_size); _i++) {                                             \
                                                                                (*(((CPU_INT08U *)(addr_dest)) + _i)) = (*(((CPU_INT08U *)(addr_src)) + _i)); \
                                                                            }                                                                                 \
                                                                        } while (0)


#define  MEM_VAL_COPY_GET_INTU_LITTLE(addr_dest, addr_src, val_size)    do {                                                                                  \
                                                                            CPU_SIZE_T  _i;                                                                   \
                                                                            CPU_SIZE_T  _j;                                                                   \
                                                                                                                                                              \
                                                                                                                                                              \
                                                                            _j = (val_size) - 1;                                                              \
                                                                                                                                                              \
                                                                            for (_i = 0; _i < (val_size); _i++) {                                             \
                                                                                (*(((CPU_INT08U *)(addr_dest)) + _i)) = (*(((CPU_INT08U *)(addr_src)) + _j)); \
                                                                                _j--;                                                                         \
                                                                            }                                                                                 \
                                                                        } while (0)


#define  MEM_VAL_COPY_GET_INTU(addr_dest, addr_src, val_size)           MEM_VAL_COPY_GET_INTU_BIG((addr_dest), (addr_src), (val_size))




#elif   (CPU_CFG_ENDIAN_TYPE == CPU_ENDIAN_TYPE_LITTLE)


#define  MEM_VAL_COPY_GET_INTU_BIG(addr_dest, addr_src, val_size)       do {                                                                                  \
                                                                            CPU_SIZE_T  _i;                                                                   \
                                                                            CPU_SIZE_T  _j;                                                                   \
                                                                                                                                                              \
                                                                                                                                                              \
                                                                            _j = (val_size) - 1;                                                              \
                                                                                                                                                              \
                                                                            for (_i = 0; _i < (val_size); _i++) {                                             \
                                                                                (*(((CPU_INT08U *)(addr_dest)) + _i)) = (*(((CPU_INT08U *)(addr_src)) + _j)); \
                                                                                _j--;                                                                         \
                                                                            }                                                                                 \
                                                                        } while (0)


#define  MEM_VAL_COPY_GET_INTU_LITTLE(addr_dest, addr_src, val_size)    do {                                                                                  \
                                                                            CPU_SIZE_T  _i;                                                                   \
                                                                                                                                                              \
                                                                            for (_i = 0; _i < (val_size); _i++) {                                             \
                                                                                (*(((CPU_INT08U *)(addr_dest)) + _i)) = (*(((CPU_INT08U *)(addr_src)) + _i)); \
                                                                            }                                                                                 \
                                                                        } while (0)


#define  MEM_VAL_COPY_GET_INTU(addr_dest, addr_src, val_size)           MEM_VAL_COPY_GET_INTU_LITTLE((addr_dest), (addr_src), (val_size))




#else                                                           /* See Note #8.                                         */

#error  "CPU_CFG_ENDIAN_TYPE  illegally #defined in 'cpu.h'      "
#error  "                     [See 'cpu.h  CONFIGURATION ERRORS']"

#endif


/*
*********************************************************************************************************
*                                       MEM_VAL_COPY_SET_xxx()
*
* Description : Copy & encode data values from any CPU memory address to any CPU memory address.
*
* Argument(s) : addr_dest       Lowest CPU memory address to copy/encode source address's data value
*                                   (see Notes #2 & #3).
*
*               addr_src        Lowest CPU memory address of data value to copy/encode
*                                   (see Notes #2 & #3).
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Copy/encode data values based on the values' data-word order :
*
*                       MEM_VAL_COPY_SET_xxx_BIG()      Encode big-   endian data values -- data words' most
*                                                           significant octet @ lowest memory address
*                       MEM_VAL_COPY_SET_xxx_LITTLE()   Encode little-endian data values -- data words' least
*                                                           significant octet @ lowest memory address
*                       MEM_VAL_COPY_SET_xxx()          Encode data values using CPU's native or configured
*                                                           data-word order
*
*                   See also 'cpu.h  CPU WORD CONFIGURATION  Note #2'.
*
*               (2) (a) CPU memory addresses/pointers NOT checked for NULL.
*
*                   (b) CPU memory addresses/buffers  NOT checked for overlapping.
*
*                       (1) IEEE Std 1003.1, 2004 Edition, Section 'memcpy() : DESCRIPTION' states that
*                           "copying ... between objects that overlap ... is undefined".
*
*               (3) MEM_VAL_COPY_SET_xxx() macro's copy/encode data values without regard to CPU word-aligned
*                   addresses.  Thus for processors that require data word alignment, data words can be copied/
*                   encoded to/from any CPU address, word-aligned or not, without generating data-word-alignment
*                   exceptions/faults.
*
*               (4) MEM_VAL_COPY_SET_xxx() macro's are more efficient than MEM_VAL_SET_xxx() macro's & are
*                   also independent of CPU data-word-alignment & SHOULD be used whenever possible.
*
*                   See also 'MEM_VAL_SET_xxx()  Note #4'.
*
*               (5) Since octet-order copy/conversion are inverse operations, MEM_VAL_COPY_GET_xxx() &
*                   MEM_VAL_COPY_SET_xxx() macros are inverse, but identical, operations & are provided
*                   in both forms for semantics & consistency.
*
*                   See also 'MEM_VAL_COPY_GET_xxx()  Note #5'.
*
*               (6) MEM_VAL_COPY_SET_xxx() macro's are NOT atomic operations & MUST NOT be used on any
*                   non-static (i.e. volatile) variables, registers, hardware, etc.; without the caller
*                   of the  macro's providing some form of additional protection (e.g. mutual exclusion).
*********************************************************************************************************
*/

                                                                /* See Note #5.                                         */
#define  MEM_VAL_COPY_SET_INT08U_BIG(addr_dest, addr_src)               MEM_VAL_COPY_GET_INT08U_BIG((addr_dest), (addr_src))
#define  MEM_VAL_COPY_SET_INT16U_BIG(addr_dest, addr_src)               MEM_VAL_COPY_GET_INT16U_BIG((addr_dest), (addr_src))
#define  MEM_VAL_COPY_SET_INT24U_BIG(addr_dest, addr_src)               MEM_VAL_COPY_GET_INT24U_BIG((addr_dest), (addr_src))
#define  MEM_VAL_COPY_SET_INT32U_BIG(addr_dest, addr_src)               MEM_VAL_COPY_GET_INT32U_BIG((addr_dest), (addr_src))

#define  MEM_VAL_COPY_SET_INT08U_LITTLE(addr_dest, addr_src)            MEM_VAL_COPY_GET_INT08U_LITTLE((addr_dest), (addr_src))
#define  MEM_VAL_COPY_SET_INT16U_LITTLE(addr_dest, addr_src)            MEM_VAL_COPY_GET_INT16U_LITTLE((addr_dest), (addr_src))
#define  MEM_VAL_COPY_SET_INT24U_LITTLE(addr_dest, addr_src)            MEM_VAL_COPY_GET_INT24U_LITTLE((addr_dest), (addr_src))
#define  MEM_VAL_COPY_SET_INT32U_LITTLE(addr_dest, addr_src)            MEM_VAL_COPY_GET_INT32U_LITTLE((addr_dest), (addr_src))


#define  MEM_VAL_COPY_SET_INT08U(addr_dest, addr_src)                   MEM_VAL_COPY_GET_INT08U((addr_dest), (addr_src))
#define  MEM_VAL_COPY_SET_INT16U(addr_dest, addr_src)                   MEM_VAL_COPY_GET_INT16U((addr_dest), (addr_src))
#define  MEM_VAL_COPY_SET_INT24U(addr_dest, addr_src)                   MEM_VAL_COPY_GET_INT24U((addr_dest), (addr_src))
#define  MEM_VAL_COPY_SET_INT32U(addr_dest, addr_src)                   MEM_VAL_COPY_GET_INT32U((addr_dest), (addr_src))


/*
*********************************************************************************************************
*                                     MEM_VAL_COPY_SET_INTU_xxx()
*
* Description : Copy & encode data values from any CPU memory address to any CPU memory address for
*                   any sized data values.
*
* Argument(s) : addr_dest       Lowest CPU memory address to copy/encode source address's data value
*                                   (see Notes #2 & #3).
*
*               addr_src        Lowest CPU memory address of data value to copy/encode
*                                   (see Notes #2 & #3).
*
*               val_size        Number of data value octets to copy/encode.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Copy/encode data values based on the values' data-word order :
*
*                       MEM_VAL_COPY_SET_INTU_BIG()     Encode big-   endian data values -- data words' most
*                                                           significant octet @ lowest memory address
*                       MEM_VAL_COPY_SET_INTU_LITTLE()  Encode little-endian data values -- data words' least
*                                                           significant octet @ lowest memory address
*                       MEM_VAL_COPY_SET_INTU()         Encode data values using CPU's native or configured
*                                                           data-word order
*
*                   See also 'cpu.h  CPU WORD CONFIGURATION  Note #2'.
*
*               (2) (a) CPU memory addresses/pointers NOT checked for NULL.
*
*                   (b) CPU memory addresses/buffers  NOT checked for overlapping.
*
*                       (1) IEEE Std 1003.1, 2004 Edition, Section 'memcpy() : DESCRIPTION' states that
*                           "copying ... between objects that overlap ... is undefined".
*
*               (3) MEM_VAL_COPY_SET_INTU_xxx() macro's copy/encode data values without regard to CPU word-
*                   aligned addresses.  Thus for processors that require data word alignment, data words
*                   can be copied/encoded to/from any CPU address, word-aligned or not, without generating
*                   data-word-alignment exceptions/faults.
*
*               (4) MEM_VAL_COPY_SET_xxx() macro's are more efficient than MEM_VAL_COPY_SET_INTU_xxx()
*                   macro's & SHOULD be used whenever possible.
*
*                   See also 'MEM_VAL_COPY_SET_xxx()  Note #4'.
*
*               (5) Since octet-order copy/conversion are inverse operations, MEM_VAL_COPY_GET_INTU_xxx() &
*                   MEM_VAL_COPY_SET_INTU_xxx() macros are inverse, but identical, operations & are provided
*                   in both forms for semantics & consistency.
*
*                   See also 'MEM_VAL_COPY_GET_INTU_xxx()  Note #5'.
*
*               (6) MEM_VAL_COPY_SET_INTU_xxx() macro's are NOT atomic operations & MUST NOT be used on any
*                   non-static (i.e. volatile) variables, registers, hardware, etc.; without the caller of
*                   the macro's providing some form of additional protection (e.g. mutual exclusion).
*********************************************************************************************************
*/

                                                                /* See Note #5.                                         */
#define  MEM_VAL_COPY_SET_INTU_BIG(addr_dest, addr_src, val_size)       MEM_VAL_COPY_GET_INTU_BIG((addr_dest), (addr_src), (val_size))
#define  MEM_VAL_COPY_SET_INTU_LITTLE(addr_dest, addr_src, val_size)    MEM_VAL_COPY_GET_INTU_LITTLE((addr_dest), (addr_src), (val_size))
#define  MEM_VAL_COPY_SET_INTU(addr_dest, addr_src, val_size)           MEM_VAL_COPY_GET_INTU((addr_dest), (addr_src), (val_size))


/*
*********************************************************************************************************
*                                         MEM_VAL_COPY_xxx()
*
* Description : Copy data values from any CPU memory address to any CPU memory address.
*
* Argument(s) : addr_dest       Lowest CPU memory address to copy source address's data value
*                                   (see Notes #2 & #3).
*
*               addr_src        Lowest CPU memory address of data value to copy
*                                   (see Notes #2 & #3).
*
*               val_size        Number of data value octets to copy.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) MEM_VAL_COPY_xxx() macro's copy data values based on CPU's native data-word order.
*
*                   See also 'cpu.h  CPU WORD CONFIGURATION  Note #2'.
*
*               (2) (a) CPU memory addresses/pointers NOT checked for NULL.
*
*                   (b) CPU memory addresses/buffers  NOT checked for overlapping.
*
*                       (1) IEEE Std 1003.1, 2004 Edition, Section 'memcpy() : DESCRIPTION' states that
*                           "copying ... between objects that overlap ... is undefined".
*
*               (3) MEM_VAL_COPY_xxx() macro's copy data values without regard to CPU word-aligned addresses.
*                   Thus for processors that require data word alignment, data words can be copied to/from any
*                   CPU address, word-aligned or not, without generating data-word-alignment exceptions/faults.
*
*               (4) MEM_VAL_COPY_xxx() macro's are more efficient than MEM_VAL_COPY() macro & SHOULD be
*                   used whenever possible.
*
*               (5) MEM_VAL_COPY_xxx() macro's are NOT atomic operations & MUST NOT be used on any non-static
*                   (i.e. volatile) variables, registers, hardware, etc.; without the caller of the macro's
*                   providing some form of additional protection (e.g. mutual exclusion).
*
*               (6) MISRA-C 2004 Rule 5.2 states that "identifiers in an inner scope shall not use the same
*                   name as an indentifier in an outer scope, and therefore hide that identifier".
*
*                   Therefore, to avoid possible redeclaration of commonly-used loop counter identifier name,
*                   'i', MEM_VAL_COPY() loop counter identifier name is prefixed with a single underscore.
*********************************************************************************************************
*/

#define  MEM_VAL_COPY_08(addr_dest, addr_src)                  do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 0)); } while (0)

#define  MEM_VAL_COPY_16(addr_dest, addr_src)                  do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 0)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 1)) = (*(((CPU_INT08U *)(addr_src)) + 1)); } while (0)

#define  MEM_VAL_COPY_24(addr_dest, addr_src)                  do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 0)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 1)) = (*(((CPU_INT08U *)(addr_src)) + 1)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 2)) = (*(((CPU_INT08U *)(addr_src)) + 2)); } while (0)

#define  MEM_VAL_COPY_32(addr_dest, addr_src)                  do { (*(((CPU_INT08U *)(addr_dest)) + 0)) = (*(((CPU_INT08U *)(addr_src)) + 0)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 1)) = (*(((CPU_INT08U *)(addr_src)) + 1)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 2)) = (*(((CPU_INT08U *)(addr_src)) + 2)); \
                                                                    (*(((CPU_INT08U *)(addr_dest)) + 3)) = (*(((CPU_INT08U *)(addr_src)) + 3)); } while (0)


#define  MEM_VAL_COPY(addr_dest, addr_src, val_size)        do {                                                                                \
                                                                CPU_SIZE_T  _i;                                                                 \
                                                                                                                                                \
                                                                for (_i = 0; _i < (val_size); _i++) {                                           \
                                                                    (*(((CPU_INT08U *)(addr_dest)) +_i)) = (*(((CPU_INT08U *)(addr_src)) +_i)); \
                                                                }                                                                               \
                                                            } while (0)


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void               Mem_Init                 (       void);

                                                                /* ------------------ MEM API  FNCTS ------------------ */
void               Mem_Clr                  (       void              *pmem,
                                                    CPU_SIZE_T         size);

void               Mem_Set                  (       void              *pmem,
                                                    CPU_INT08U         data_val,
                                                    CPU_SIZE_T         size);

void               Mem_Copy                 (       void              *pdest,
                                             const  void              *psrc,
                                                    CPU_SIZE_T         size);

void               Mem_Move                 (       void              *pdest,
                                             const  void              *psrc,
                                                    CPU_SIZE_T         size);

CPU_BOOLEAN        Mem_Cmp                  (const  void              *p1_mem,
                                             const  void              *p2_mem,
                                                    CPU_SIZE_T         size);


                                                                /* ----------- MEM HEAP FNCTS (DEPRECATED) ------------ */
#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
void              *Mem_HeapAlloc            (       CPU_SIZE_T         size,
                                                    CPU_SIZE_T         align,
                                                    CPU_SIZE_T        *p_bytes_reqd,
                                                    LIB_ERR           *p_err);

CPU_SIZE_T         Mem_HeapGetSizeRem       (       CPU_SIZE_T         align,
                                                    LIB_ERR           *p_err);
#endif

                                                                /* ------------------ MEM SEG FNCTS ------------------- */
void               Mem_SegCreate            (const  CPU_CHAR          *p_name,
                                                    MEM_SEG           *p_seg,
                                                    CPU_ADDR           seg_base_addr,
                                                    CPU_SIZE_T         size,
                                                    CPU_SIZE_T         padding_align,
                                                    LIB_ERR           *p_err);

void               Mem_SegClr               (       MEM_SEG           *p_seg,
                                                    LIB_ERR           *p_err);

void              *Mem_SegAlloc             (const  CPU_CHAR          *p_name,
                                                    MEM_SEG           *p_seg,
                                                    CPU_SIZE_T         size,
                                                    LIB_ERR           *p_err);

void              *Mem_SegAllocExt          (const  CPU_CHAR          *p_name,
                                                    MEM_SEG           *p_seg,
                                                    CPU_SIZE_T         size,
                                                    CPU_SIZE_T         align,
                                                    CPU_SIZE_T        *p_bytes_reqd,
                                                    LIB_ERR           *p_err);

void              *Mem_SegAllocHW           (const  CPU_CHAR          *p_name,
                                                    MEM_SEG           *p_seg,
                                                    CPU_SIZE_T         size,
                                                    CPU_SIZE_T         align,
                                                    CPU_SIZE_T        *p_bytes_reqd,
                                                    LIB_ERR           *p_err);

CPU_SIZE_T         Mem_SegRemSizeGet        (       MEM_SEG           *p_seg,
                                                    CPU_SIZE_T         align,
                                                    MEM_SEG_INFO      *p_seg_info,
                                                    LIB_ERR           *p_err);

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
void               Mem_OutputUsage          (       void             (*out_fnct) (CPU_CHAR *),
                                                    LIB_ERR           *p_err);
#endif

                                                                /* -------- STATIC MEM POOL FNCTS (DEPRECATED) -------- */
void               Mem_PoolCreate           (       MEM_POOL          *p_pool,
                                                    void              *p_mem_base,
                                                    CPU_SIZE_T         mem_size,
                                                    MEM_POOL_BLK_QTY   blk_nbr,
                                                    CPU_SIZE_T         blk_size,
                                                    CPU_SIZE_T         blk_align,
                                                    CPU_SIZE_T        *p_bytes_reqd,
                                                    LIB_ERR           *p_err);

void               Mem_PoolClr              (       MEM_POOL          *p_pool,
                                                    LIB_ERR           *p_err);

void              *Mem_PoolBlkGet           (       MEM_POOL          *p_pool,
                                                    CPU_SIZE_T         size,
                                                    LIB_ERR           *p_err);

void               Mem_PoolBlkFree          (       MEM_POOL          *p_pool,
                                                    void              *p_blk,
                                                    LIB_ERR           *p_err);

MEM_POOL_BLK_QTY   Mem_PoolBlkGetNbrAvail   (       MEM_POOL          *p_pool,
                                                    LIB_ERR           *p_err);

                                                                /* -------------- DYNAMIC MEM POOL FNCTS -------------- */
void               Mem_DynPoolCreate        (const  CPU_CHAR          *p_name,
                                                    MEM_DYN_POOL      *p_pool,
                                                    MEM_SEG           *p_seg,
                                                    CPU_SIZE_T         blk_size,
                                                    CPU_SIZE_T         blk_align,
                                                    CPU_SIZE_T         blk_qty_init,
                                                    CPU_SIZE_T         blk_qty_max,
                                                    LIB_ERR           *p_err);

void               Mem_DynPoolCreateHW      (const  CPU_CHAR          *p_name,
                                                    MEM_DYN_POOL      *p_pool,
                                                    MEM_SEG           *p_seg,
                                                    CPU_SIZE_T         blk_size,
                                                    CPU_SIZE_T         blk_align,
                                                    CPU_SIZE_T         blk_qty_init,
                                                    CPU_SIZE_T         blk_qty_max,
                                                    LIB_ERR           *p_err);

void              *Mem_DynPoolBlkGet        (       MEM_DYN_POOL      *p_pool,
                                                    LIB_ERR           *p_err);

void               Mem_DynPoolBlkFree       (       MEM_DYN_POOL      *p_pool,
                                                    void              *p_blk,
                                                    LIB_ERR           *p_err);

CPU_SIZE_T         Mem_DynPoolBlkNbrAvailGet(       MEM_DYN_POOL      *p_pool,
                                                    LIB_ERR           *p_err);


/*
*********************************************************************************************************
*                                        CONFIGURATION ERRORS
*********************************************************************************************************
*/

#ifndef  LIB_MEM_CFG_ARG_CHK_EXT_EN
#error  "LIB_MEM_CFG_ARG_CHK_EXT_EN         not #define'd in 'lib_cfg.h'"
#error  "                             [MUST be  DEF_DISABLED]           "
#error  "                             [     ||  DEF_ENABLED ]           "

#elif  ((LIB_MEM_CFG_ARG_CHK_EXT_EN != DEF_DISABLED) && \
        (LIB_MEM_CFG_ARG_CHK_EXT_EN != DEF_ENABLED ))
#error  "LIB_MEM_CFG_ARG_CHK_EXT_EN   illegally #define'd in 'lib_cfg.h'"
#error  "                             [MUST be  DEF_DISABLED]           "
#error  "                             [     ||  DEF_ENABLED ]           "
#endif



#ifndef  LIB_MEM_CFG_OPTIMIZE_ASM_EN
#error  "LIB_MEM_CFG_OPTIMIZE_ASM_EN        not #define'd in 'lib_cfg.h'"
#error  "                             [MUST be  DEF_DISABLED]           "
#error  "                             [     ||  DEF_ENABLED ]           "

#elif  ((LIB_MEM_CFG_OPTIMIZE_ASM_EN != DEF_DISABLED) && \
        (LIB_MEM_CFG_OPTIMIZE_ASM_EN != DEF_ENABLED ))
#error  "LIB_MEM_CFG_OPTIMIZE_ASM_EN  illegally #define'd in 'lib_cfg.h'"
#error  "                             [MUST be  DEF_DISABLED]           "
#error  "                             [     ||  DEF_ENABLED ]           "
#endif


#ifndef  LIB_MEM_CFG_HEAP_SIZE
#error  "LIB_MEM_CFG_HEAP_SIZE              not #define'd in 'lib_cfg.h'"
#error  "                                   [MUST be  >= 0]             "
#endif


#ifdef   LIB_MEM_CFG_HEAP_BASE_ADDR
#if     (LIB_MEM_CFG_HEAP_BASE_ADDR == 0x0)
#error  "LIB_MEM_CFG_HEAP_BASE_ADDR   illegally #define'd in 'lib_cfg.h'"
#error  "                             [MUST be  > 0x0]                  "
#endif
#endif


#if    ((LIB_MEM_CFG_DBG_INFO_EN != DEF_DISABLED) && \
        (LIB_MEM_CFG_DBG_INFO_EN != DEF_ENABLED ))
#error  "LIB_MEM_CFG_DBG_INFO_EN illegally defined in 'lib_cfg.h'"
#error  "                        [MUST be  DEF_DISABLED]         "
#error  "                        [     ||  DEF_ENABLED ]         "

#elif  ((LIB_MEM_CFG_HEAP_SIZE   == 0u) &&           \
        (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED))
#error  "LIB_MEM_CFG_HEAP_SIZE illegally defined in 'lib_cfg.h'                         "
#error  "                      [MUST be > 0 when LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED]"
#endif


/*
*********************************************************************************************************
*                                    LIBRARY CONFIGURATION ERRORS
*********************************************************************************************************
*/

                                                                /* See 'lib_mem.h  Note #2a'.                           */
#if     (CPU_CORE_VERSION < 127u)
#error  "CPU_CORE_VERSION  [SHOULD be >= V1.27]"
#endif


/*
*********************************************************************************************************
*                                             MODULE END
*
* Note(s) : (1) See 'lib_mem.h  MODULE'.
*********************************************************************************************************
*/

#endif                                                          /* End of lib mem module include.                       */

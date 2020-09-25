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
* Filename : lib_mem.c
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
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    LIB_MEM_MODULE
#include  "lib_mem.h"
#include  "lib_math.h"
#include  "lib_str.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
#ifndef  LIB_MEM_CFG_HEAP_BASE_ADDR
CPU_INT08U   Mem_Heap[LIB_MEM_CFG_HEAP_SIZE];                   /* Mem heap.                                            */
#endif

MEM_SEG      Mem_SegHeap;                                       /* Heap mem seg.                                        */
#endif

MEM_SEG     *Mem_SegHeadPtr;                                    /* Ptr to head of seg list.                             */


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void          Mem_SegCreateCritical    (const  CPU_CHAR      *p_name,
                                                       MEM_SEG       *p_seg,
                                                       CPU_ADDR       seg_base_addr,
                                                       CPU_SIZE_T     padding_align,
                                                       CPU_SIZE_T     size);

#if  (LIB_MEM_CFG_HEAP_SIZE > 0u)
static  MEM_SEG      *Mem_SegOverlapChkCritical(       CPU_ADDR       seg_base_addr,
                                                       CPU_SIZE_T     size,
                                                       LIB_ERR       *p_err);
#endif

static  void         *Mem_SegAllocInternal     (const  CPU_CHAR      *p_name,
                                                       MEM_SEG       *p_seg,
                                                       CPU_SIZE_T     size,
                                                       CPU_SIZE_T     align,
                                                       CPU_SIZE_T     padding_align,
                                                       CPU_SIZE_T    *p_bytes_reqd,
                                                       LIB_ERR       *p_err);

static  void         *Mem_SegAllocExtCritical  (       MEM_SEG       *p_seg,
                                                       CPU_SIZE_T     size,
                                                       CPU_SIZE_T     align,
                                                       CPU_SIZE_T     padding_align,
                                                       CPU_SIZE_T    *p_bytes_reqd,
                                                       LIB_ERR       *p_err);

static  void          Mem_DynPoolCreateInternal(const  CPU_CHAR      *p_name,
                                                       MEM_DYN_POOL  *p_pool,
                                                       MEM_SEG       *p_seg,
                                                       CPU_SIZE_T     blk_size,
                                                       CPU_SIZE_T     blk_align,
                                                       CPU_SIZE_T     blk_padding_align,
                                                       CPU_SIZE_T     blk_qty_init,
                                                       CPU_SIZE_T     blk_qty_max,
                                                       LIB_ERR       *p_err);

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
static  void          Mem_SegAllocTrackCritical(const  CPU_CHAR      *p_name,
                                                       MEM_SEG       *p_seg,
                                                       CPU_SIZE_T     size,
                                                       LIB_ERR       *p_err);
#endif

#if ((LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED) && \
     (LIB_MEM_CFG_HEAP_SIZE      >  0u))
static  CPU_BOOLEAN   Mem_PoolBlkIsValidAddr   (       MEM_POOL      *p_pool,
                                                       void          *p_mem);
#endif


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                            GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             Mem_Init()
*
* Description : (1) Initializes Memory Management Module :
*
*                   (a) Initialize heap memory pool
*                   (b) Initialize      memory pool table
*
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (2) Mem_Init() MUST be called ... :
*
*                   (a) ONLY ONCE from a product's application; ...
*                   (b) BEFORE product's application calls any memory library module function(s)
*********************************************************************************************************
*/

void  Mem_Init (void)
{

                                                                /* ------------------ INIT SEG LIST ------------------- */
    Mem_SegHeadPtr = DEF_NULL;

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
    {
        LIB_ERR   err;
        CPU_ADDR  heap_base_addr;


                                                                /* ------------------ INIT HEAP SEG ------------------- */
#ifdef  LIB_MEM_CFG_HEAP_BASE_ADDR
        heap_base_addr = LIB_MEM_CFG_HEAP_BASE_ADDR;
#else
        heap_base_addr = (CPU_ADDR)&Mem_Heap[0u];
#endif

        Mem_SegCreate("Heap",
                      &Mem_SegHeap,                             /* Create heap seg.                                     */
                       heap_base_addr,
                       LIB_MEM_CFG_HEAP_SIZE,
                       LIB_MEM_PADDING_ALIGN_NONE,
                      &err);
        if (err != LIB_MEM_ERR_NONE) {
            CPU_SW_EXCEPTION(;);
        }
    }
#endif
}


/*
*********************************************************************************************************
*                                              Mem_Clr()
*
* Description : Clears data buffer (see Note #2).
*
* Argument(s) : pmem        Pointer to memory buffer to clear.
*
*               size        Number of data buffer octets to clear (see Note #1).
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Null clears allowed (i.e. zero-length clears).
*
*                   See also 'Mem_Set()  Note #1'.
*
*               (2) Clear data by setting each data octet to 0.
*********************************************************************************************************
*/

void  Mem_Clr (void        *pmem,
               CPU_SIZE_T   size)
{
    Mem_Set(pmem,
            0u,                                                 /* See Note #2.                                         */
            size);
}


/*
*********************************************************************************************************
*                                              Mem_Set()
*
* Description : Fills data buffer with specified data octet.
*
* Argument(s) : pmem        Pointer to memory buffer to fill with specified data octet.
*
*               data_val    Data fill octet value.
*
*               size        Number of data buffer octets to fill (see Note #1).
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Null sets allowed (i.e. zero-length sets).
*
*               (2) For best CPU performance, optimized to fill data buffer using 'CPU_ALIGN'-sized data
*                   words. Since many word-aligned processors REQUIRE that multi-octet words be accessed on
*                   word-aligned addresses, 'CPU_ALIGN'-sized words MUST be accessed on 'CPU_ALIGN'd
*                   addresses.
*
*               (3) Modulo arithmetic is used to determine whether a memory buffer starts on a 'CPU_ALIGN'
*                   address boundary.
*
*                   Modulo arithmetic in ANSI-C REQUIREs operations performed on integer values.  Thus
*                   address values MUST be cast to an appropriately-sized integer value PRIOR to any
*                  'mem_align_mod' arithmetic operation.
*********************************************************************************************************
*/

void  Mem_Set (void        *pmem,
               CPU_INT08U   data_val,
               CPU_SIZE_T   size)
{
    CPU_SIZE_T   size_rem;
    CPU_ALIGN    data_align;
    CPU_ALIGN   *pmem_align;
    CPU_INT08U  *pmem_08;
    CPU_DATA     mem_align_mod;
    CPU_DATA     i;


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (size < 1) {                                             /* See Note #1.                                         */
        return;
    }
    if (pmem == (void *)0) {
        return;
    }
#endif


    data_align = 0u;
    for (i = 0u; i < sizeof(CPU_ALIGN); i++) {                  /* Fill each data_align octet with data val.            */
        data_align <<=  DEF_OCTET_NBR_BITS;
        data_align  |= (CPU_ALIGN)data_val;
    }

    size_rem      =  size;
    mem_align_mod = (CPU_INT08U)((CPU_ADDR)pmem % sizeof(CPU_ALIGN));   /* See Note #3.                                 */

    pmem_08 = (CPU_INT08U *)pmem;
    if (mem_align_mod != 0u) {                                  /* If leading octets avail,                   ...       */
        i = mem_align_mod;
        while ((size_rem > 0) &&                                /* ... start mem buf fill with leading octets ...       */
               (i        < sizeof(CPU_ALIGN ))) {               /* ... until next CPU_ALIGN word boundary.              */
           *pmem_08++ = data_val;
            size_rem -= sizeof(CPU_INT08U);
            i++;
        }
    }

    pmem_align = (CPU_ALIGN *)pmem_08;                          /* See Note #2.                                         */
    while (size_rem >= sizeof(CPU_ALIGN)) {                     /* While mem buf aligned on CPU_ALIGN word boundaries,  */
       *pmem_align++ = data_align;                              /* ... fill mem buf with    CPU_ALIGN-sized data.       */
        size_rem    -= sizeof(CPU_ALIGN);
    }

    pmem_08 = (CPU_INT08U *)pmem_align;
    while (size_rem > 0) {                                      /* Finish mem buf fill with trailing octets.            */
       *pmem_08++   = data_val;
        size_rem   -= sizeof(CPU_INT08U);
    }
}


/*
*********************************************************************************************************
*                                             Mem_Copy()
*
* Description : Copies data octets from one memory buffer to another memory buffer.
*
* Argument(s) : pdest       Pointer to destination memory buffer.
*
*               psrc        Pointer to source      memory buffer.
*
*               size        Number of octets to copy (see Note #1).
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Null copies allowed (i.e. zero-length copies).
*
*               (2) Memory buffers NOT checked for overlapping.
*
*                   (a) IEEE Std 1003.1, 2004 Edition, Section 'memcpy() : DESCRIPTION' states that "if
*                       copying takes place between objects that overlap, the behavior is undefined".
*
*                   (b) However, data octets from a source memory buffer at a higher address value SHOULD
*                       successfully copy to a destination memory buffer at a lower  address value even
*                       if any octets of the memory buffers overlap as long as no individual, atomic CPU
*                       word copy overlaps.
*
*                       Since Mem_Copy() performs the data octet copy via 'CPU_ALIGN'-sized words &/or
*                       octets; & since 'CPU_ALIGN'-sized words MUST be accessed on word-aligned addresses
*                       (see Note #3b), neither 'CPU_ALIGN'-sized words nor octets at unique addresses can
*                       ever overlap.
*
*                       Therefore, Mem_Copy() SHOULD be able to successfully copy overlapping memory
*                       buffers as long as the source memory buffer is at a higher address value than the
*                       destination memory buffer.
*
*               (3) For best CPU performance, optimized to copy data buffer using 'CPU_ALIGN'-sized data
*                   words. Since many word-aligned processors REQUIRE that multi-octet words be accessed on
*                   word-aligned addresses, 'CPU_ALIGN'-sized words MUST be accessed on 'CPU_ALIGN'd
*                   addresses.
*
*               (4) Modulo arithmetic is used to determine whether a memory buffer starts on a 'CPU_ALIGN'
*                   address boundary.
*
*                   Modulo arithmetic in ANSI-C REQUIREs operations performed on integer values.  Thus
*                   address values MUST be cast to an appropriately-sized integer value PRIOR to any
*                  'mem_align_mod' arithmetic operation.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_OPTIMIZE_ASM_EN != DEF_ENABLED)
void  Mem_Copy (       void        *pdest,
                const  void        *psrc,
                       CPU_SIZE_T   size)
{
           CPU_SIZE_T    size_rem;
           CPU_SIZE_T    mem_gap_octets;
           CPU_ALIGN    *pmem_align_dest;
    const  CPU_ALIGN    *pmem_align_src;
           CPU_INT08U   *pmem_08_dest;
    const  CPU_INT08U   *pmem_08_src;
           CPU_DATA      i;
           CPU_DATA      mem_align_mod_dest;
           CPU_DATA      mem_align_mod_src;
           CPU_BOOLEAN   mem_aligned;


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (size < 1) {                                             /* See Note #1.                                         */
        return;
    }
    if (pdest == (void *)0) {
        return;
    }
    if (psrc  == (void *)0) {
        return;
    }
#endif


    size_rem           =  size;

    pmem_08_dest       = (      CPU_INT08U *)pdest;
    pmem_08_src        = (const CPU_INT08U *)psrc;

    mem_gap_octets     = (CPU_SIZE_T)(pmem_08_src - pmem_08_dest);


    if (mem_gap_octets >= sizeof(CPU_ALIGN)) {                  /* Avoid bufs overlap.                                  */
                                                                /* See Note #4.                                         */
        mem_align_mod_dest = (CPU_INT08U)((CPU_ADDR)pmem_08_dest % sizeof(CPU_ALIGN));
        mem_align_mod_src  = (CPU_INT08U)((CPU_ADDR)pmem_08_src  % sizeof(CPU_ALIGN));

        mem_aligned        = (mem_align_mod_dest == mem_align_mod_src) ? DEF_YES : DEF_NO;

        if (mem_aligned == DEF_YES) {                           /* If mem bufs' alignment offset equal, ...             */
                                                                /* ... optimize copy for mem buf alignment.             */
            if (mem_align_mod_dest != 0u) {                     /* If leading octets avail,                   ...       */
                i = mem_align_mod_dest;
                while ((size_rem   >  0) &&                     /* ... start mem buf copy with leading octets ...       */
                       (i          <  sizeof(CPU_ALIGN ))) {    /* ... until next CPU_ALIGN word boundary.              */
                   *pmem_08_dest++ = *pmem_08_src++;
                    size_rem      -=  sizeof(CPU_INT08U);
                    i++;
                }
            }

            pmem_align_dest = (      CPU_ALIGN *)pmem_08_dest;  /* See Note #3.                                         */
            pmem_align_src  = (const CPU_ALIGN *)pmem_08_src;
            while (size_rem      >=  sizeof(CPU_ALIGN)) {       /* While mem bufs aligned on CPU_ALIGN word boundaries, */
               *pmem_align_dest++ = *pmem_align_src++;          /* ... copy psrc to pdest with CPU_ALIGN-sized words.   */
                size_rem         -=  sizeof(CPU_ALIGN);
            }

            pmem_08_dest = (      CPU_INT08U *)pmem_align_dest;
            pmem_08_src  = (const CPU_INT08U *)pmem_align_src;
        }
    }

    while (size_rem > 0) {                                      /* For unaligned mem bufs or trailing octets, ...       */
       *pmem_08_dest++ = *pmem_08_src++;                        /* ... copy psrc to pdest by octets.                    */
        size_rem      -=  sizeof(CPU_INT08U);
    }
}
#endif


/*
*********************************************************************************************************
*                                             Mem_Move()
*
* Description : Moves data octets from one memory buffer to another memory buffer, or within the same
*               memory buffer. Overlapping is correctly handled for all move operations.
*
* Argument(s) : pdest       Pointer to destination memory buffer.
*
*               psrc        Pointer to source      memory buffer.
*
*               size        Number of octets to move (see Note #1).
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Null move operations allowed (i.e. zero-length).
*
*               (2) Memory buffers checked for overlapping.
*
*               (3) For best CPU performance, optimized to copy data buffer using 'CPU_ALIGN'-sized data
*                   words. Since many word-aligned processors REQUIRE that multi-octet words be accessed on
*                   word-aligned addresses, 'CPU_ALIGN'-sized words MUST be accessed on 'CPU_ALIGN'd
*                   addresses.
*
*               (4) Modulo arithmetic is used to determine whether a memory buffer starts on a 'CPU_ALIGN'
*                   address boundary.
*
*                   Modulo arithmetic in ANSI-C REQUIREs operations performed on integer values.  Thus
*                   address values MUST be cast to an appropriately-sized integer value PRIOR to any
*                  'mem_align_mod' arithmetic operation.
*********************************************************************************************************
*/

void  Mem_Move (       void        *pdest,
                const  void        *psrc,
                       CPU_SIZE_T   size)
{
           CPU_SIZE_T    size_rem;
           CPU_SIZE_T    mem_gap_octets;
           CPU_ALIGN    *pmem_align_dest;
    const  CPU_ALIGN    *pmem_align_src;
           CPU_INT08U   *pmem_08_dest;
    const  CPU_INT08U   *pmem_08_src;
           CPU_INT08S    i;
           CPU_DATA      mem_align_mod_dest;
           CPU_DATA      mem_align_mod_src;
           CPU_BOOLEAN   mem_aligned;


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (size < 1) {
        return;
    }
    if (pdest == (void *)0) {
        return;
    }
    if (psrc  == (void *)0) {
        return;
    }
#endif

    pmem_08_src  = (const CPU_INT08U *)psrc;
    pmem_08_dest = (      CPU_INT08U *)pdest;
    if (pmem_08_src > pmem_08_dest) {
        Mem_Copy(pdest, psrc, size);
        return;
    }

    size_rem           =  size;

    pmem_08_dest       = (      CPU_INT08U *)pdest + size - 1;
    pmem_08_src        = (const CPU_INT08U *)psrc  + size - 1;

    mem_gap_octets     = (CPU_SIZE_T)(pmem_08_dest - pmem_08_src);


    if (mem_gap_octets >= sizeof(CPU_ALIGN)) {                  /* Avoid bufs overlap.                                  */

                                                                /* See Note #4.                                         */
        mem_align_mod_dest = (CPU_INT08U)((CPU_ADDR)pmem_08_dest % sizeof(CPU_ALIGN));
        mem_align_mod_src  = (CPU_INT08U)((CPU_ADDR)pmem_08_src  % sizeof(CPU_ALIGN));

        mem_aligned        = (mem_align_mod_dest == mem_align_mod_src) ? DEF_YES : DEF_NO;

        if (mem_aligned == DEF_YES) {                           /* If mem bufs' alignment offset equal, ...             */
                                                                /* ... optimize copy for mem buf alignment.             */
            if (mem_align_mod_dest != (sizeof(CPU_ALIGN) - 1)) {/* If leading octets avail,                   ...       */
                i = (CPU_INT08S)mem_align_mod_dest;
                while ((size_rem   >  0) &&                     /* ... start mem buf copy with leading octets ...       */
                       (i          >= 0)) {                     /* ... until next CPU_ALIGN word boundary.              */
                   *pmem_08_dest-- = *pmem_08_src--;
                    size_rem      -=  sizeof(CPU_INT08U);
                    i--;
                }
            }

                                                                /* See Note #3.                                         */
            pmem_align_dest = (      CPU_ALIGN *)(((CPU_INT08U *)pmem_08_dest - sizeof(CPU_ALIGN)) + 1);
            pmem_align_src  = (const CPU_ALIGN *)(((CPU_INT08U *)pmem_08_src  - sizeof(CPU_ALIGN)) + 1);
            while (size_rem      >=  sizeof(CPU_ALIGN)) {       /* While mem bufs aligned on CPU_ALIGN word boundaries, */
               *pmem_align_dest-- = *pmem_align_src--;          /* ... copy psrc to pdest with CPU_ALIGN-sized words.   */
                size_rem         -=  sizeof(CPU_ALIGN);
            }

            pmem_08_dest = (      CPU_INT08U *)pmem_align_dest + sizeof(CPU_ALIGN) - 1;
            pmem_08_src  = (const CPU_INT08U *)pmem_align_src  + sizeof(CPU_ALIGN) - 1;

        }
    }

    while (size_rem > 0) {                                      /* For unaligned mem bufs or trailing octets, ...       */
       *pmem_08_dest-- = *pmem_08_src--;                        /* ... copy psrc to pdest by octets.                    */
        size_rem      -=  sizeof(CPU_INT08U);
    }
}


/*
*********************************************************************************************************
*                                              Mem_Cmp()
*
* Description : Verifies that ALL data octets in two memory buffers are identical in sequence.
*
* Argument(s) : p1_mem      Pointer to first  memory buffer.
*
*               p2_mem      Pointer to second memory buffer.
*
*               size        Number of data buffer octets to compare (see Note #1).
*
* Return(s)   : DEF_YES, if 'size' number of data octets are identical in both memory buffers.
*
*               DEF_NO,  otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Null compares allowed (i.e. zero-length compares); 'DEF_YES' returned to indicate
*                   identical null compare.
*
*               (2) Many memory buffer comparisons vary ONLY in the least significant octets -- e.g.
*                   network address buffers.  Consequently, memory buffer comparison is more efficient
*                   if the comparison starts from the end of the memory buffers which will abort sooner
*                   on dissimilar memory buffers that vary only in the least significant octets.
*
*               (3) For best CPU performance, optimized to compare data buffers using 'CPU_ALIGN'-sized
*                   data words. Since many word-aligned processors REQUIRE that multi-octet words be accessed on
*                   word-aligned addresses, 'CPU_ALIGN'-sized words MUST be accessed on 'CPU_ALIGN'd
*                   addresses.
*
*               (4) Modulo arithmetic is used to determine whether a memory buffer starts on a 'CPU_ALIGN'
*                   address boundary.
*
*                   Modulo arithmetic in ANSI-C REQUIREs operations performed on integer values.  Thus
*                   address values MUST be cast to an appropriately-sized integer value PRIOR to any
*                  'mem_align_mod' arithmetic operation.
*********************************************************************************************************
*/

CPU_BOOLEAN  Mem_Cmp (const  void        *p1_mem,
                      const  void        *p2_mem,
                             CPU_SIZE_T   size)
{
           CPU_SIZE_T    size_rem;
           CPU_ALIGN    *p1_mem_align;
           CPU_ALIGN    *p2_mem_align;
    const  CPU_INT08U   *p1_mem_08;
    const  CPU_INT08U   *p2_mem_08;
           CPU_DATA      i;
           CPU_DATA      mem_align_mod_1;
           CPU_DATA      mem_align_mod_2;
           CPU_BOOLEAN   mem_aligned;
           CPU_BOOLEAN   mem_cmp;


    if (size < 1) {                                             /* See Note #1.                                         */
        return (DEF_YES);
    }
    if (p1_mem == (void *)0) {
        return (DEF_NO);
    }
    if (p2_mem == (void *)0) {
        return (DEF_NO);
    }


    mem_cmp         =  DEF_YES;                                 /* Assume mem bufs are identical until cmp fails.       */
    size_rem        =  size;
                                                                /* Start @ end of mem bufs (see Note #2).               */
    p1_mem_08       = (const CPU_INT08U *)p1_mem + size;
    p2_mem_08       = (const CPU_INT08U *)p2_mem + size;
                                                                /* See Note #4.                                         */
    mem_align_mod_1 = (CPU_INT08U)((CPU_ADDR)p1_mem_08 % sizeof(CPU_ALIGN));
    mem_align_mod_2 = (CPU_INT08U)((CPU_ADDR)p2_mem_08 % sizeof(CPU_ALIGN));

    mem_aligned     = (mem_align_mod_1 == mem_align_mod_2) ? DEF_YES : DEF_NO;

    if (mem_aligned == DEF_YES) {                               /* If mem bufs' alignment offset equal, ...             */
                                                                /* ... optimize cmp for mem buf alignment.              */
        if (mem_align_mod_1 != 0u) {                            /* If trailing octets avail,                  ...       */
            i = mem_align_mod_1;
            while ((mem_cmp == DEF_YES) &&                      /* ... cmp mem bufs while identical &         ...       */
                   (size_rem > 0)       &&                      /* ... start mem buf cmp with trailing octets ...       */
                   (i        > 0)) {                            /* ... until next CPU_ALIGN word boundary.              */
                p1_mem_08--;
                p2_mem_08--;
                if (*p1_mem_08 != *p2_mem_08) {                 /* If ANY data octet(s) NOT identical, cmp fails.       */
                     mem_cmp = DEF_NO;
                }
                size_rem -= sizeof(CPU_INT08U);
                i--;
            }
        }

        if (mem_cmp == DEF_YES) {                               /* If cmp still identical, cmp aligned mem bufs.        */
            p1_mem_align = (CPU_ALIGN *)p1_mem_08;              /* See Note #3.                                         */
            p2_mem_align = (CPU_ALIGN *)p2_mem_08;

            while ((mem_cmp  == DEF_YES) &&                     /* Cmp mem bufs while identical & ...                   */
                   (size_rem >= sizeof(CPU_ALIGN))) {           /* ... mem bufs aligned on CPU_ALIGN word boundaries.   */
                p1_mem_align--;
                p2_mem_align--;
                if (*p1_mem_align != *p2_mem_align) {           /* If ANY data octet(s) NOT identical, cmp fails.       */
                     mem_cmp = DEF_NO;
                }
                size_rem -= sizeof(CPU_ALIGN);
            }

            p1_mem_08 = (CPU_INT08U *)p1_mem_align;
            p2_mem_08 = (CPU_INT08U *)p2_mem_align;
        }
    }

    while ((mem_cmp == DEF_YES) &&                              /* Cmp mem bufs while identical ...                     */
           (size_rem > 0)) {                                    /* ... for unaligned mem bufs or trailing octets.       */
        p1_mem_08--;
        p2_mem_08--;
        if (*p1_mem_08 != *p2_mem_08) {                         /* If ANY data octet(s) NOT identical, cmp fails.       */
             mem_cmp = DEF_NO;
        }
        size_rem -= sizeof(CPU_INT08U);
    }

    return (mem_cmp);
}


/*
*********************************************************************************************************
*                                           Mem_HeapAlloc()
*
* Description : Allocates a memory block from the heap memory segment.
*
* Argument(s) : size            Size      of memory block to allocate (in bytes).
*
*               align           Alignment of memory block to specific word boundary (in bytes).
*
*               p_bytes_reqd    Optional pointer to a variable to ... :
*
*                                   (a) Return the number of bytes required to successfully
*                                           allocate the memory block, if any error(s);
*                                   (b) Return 0, otherwise.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   LIB_MEM_ERR_NONE                Operation was successful.
*                                   LIB_MEM_ERR_HEAP_EMPTY          No more memory available on heap.
*
*                                   ---------------------RETURNED BY Mem_SegAllocInternal()---------------------
*                                   LIB_MEM_ERR_INVALID_MEM_ALIGN   Invalid memory block alignment requested.
*                                   LIB_MEM_ERR_INVALID_MEM_SIZE    Invalid memory block size specified.
*                                   LIB_MEM_ERR_NULL_PTR            Error or segment data pointer NULL.
*
* Return(s)   : Pointer to memory block, if NO error(s).
*
*               Pointer to NULL,         otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) Pointers to variables that return values MUST be initialized PRIOR to all other
*                   validation or function handling in case of any error(s).
*
*               (2) This function is DEPRECATED and will be removed in a future version of this product.
*                   Mem_SegAlloc(), Mem_SegAllocExt() or Mem_SegAllocHW() should be used instead.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
void  *Mem_HeapAlloc (CPU_SIZE_T   size,
                      CPU_SIZE_T   align,
                      CPU_SIZE_T  *p_bytes_reqd,
                      LIB_ERR     *p_err)
{
    void  *p_mem;


    p_mem = Mem_SegAllocInternal(DEF_NULL,
                                &Mem_SegHeap,
                                 size,
                                 align,
                                 LIB_MEM_CFG_HEAP_PADDING_ALIGN,
                                 p_bytes_reqd,
                                 p_err);
    if (*p_err == LIB_MEM_ERR_SEG_OVF) {
       *p_err = LIB_MEM_ERR_HEAP_OVF;
    }

    return (p_mem);
}
#endif


/*
*********************************************************************************************************
*                                        Mem_HeapGetSizeRem()
*
* Description : Gets remaining heap memory size available to allocate.
*
* Argument(s) : align       Desired word boundary alignment (in bytes) to return remaining memory size from.
*
*               p_err       Pointer to variable that will receive the return error code from this function
*
*                               LIB_MEM_ERR_NONE                Operation was successful.
*
*                               --------------------RETURNED BY Mem_SegRemSizeGet()--------------------
*                               LIB_MEM_ERR_NULL_PTR            Segment data pointer NULL.
*                               LIB_MEM_ERR_INVALID_MEM_ALIGN   Invalid memory alignment.
*
* Return(s)   : Remaining heap memory size (in bytes), if NO error(s).
*
*               0,                                     otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) This function is DEPRECATED and will be removed in a future version of this product.
*                   Mem_SegRemSizeGet() should be used instead.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
CPU_SIZE_T  Mem_HeapGetSizeRem (CPU_SIZE_T   align,
                                LIB_ERR     *p_err)
{
    CPU_SIZE_T  rem_size;


    rem_size = Mem_SegRemSizeGet(&Mem_SegHeap,
                                  align,
                                  DEF_NULL,
                                  p_err);
    if (*p_err != LIB_MEM_ERR_NONE) {
        return (0u);
    }

    return (rem_size);
}
#endif


/*
*********************************************************************************************************
*                                            Mem_SegCreate()
*
* Description : Creates a new memory segment to be used for runtime memory allocation.
*
* Argument(s) : p_name          Pointer to segment name.
*
*               p_seg           Pointer to segment data. Must be allocated by caller.
*
*               seg_base_addr   Address of segment's first byte.
*
*               size            Total size of segment, in bytes.
*
*               padding_align   Padding alignment, in bytes, that will be added to any allocated buffer from
*                               this memory segment. MUST be a power of 2. LIB_MEM_PADDING_ALIGN_NONE
*                               means no padding.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   LIB_MEM_ERR_NONE                    Operation was successful.
*                                   LIB_MEM_ERR_INVALID_SEG_SIZE        Invalid segment size specified.
*                                   LIB_MEM_ERR_INVALID_MEM_ALIGN       Invalid padding alignment.
*                                   LIB_MEM_ERR_NULL_PTR                Error or segment data pointer NULL.
*
*                                   -------------------RETURNED BY Mem_SegOverlapChkCritical()-------------------
*                                   LIB_MEM_ERR_INVALID_SEG_OVERLAP     Segment overlaps another existing segment.
*                                   LIB_MEM_ERR_INVALID_SEG_EXISTS      Segment already exists.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) New segments are checked for overlap with existing segments. A critical section needs
*                   to be maintained during the whole list search and add procedure to prevent a reentrant
*                   call from creating another segment overlapping with the one being added.
*********************************************************************************************************
*/

void  Mem_SegCreate (const  CPU_CHAR    *p_name,
                            MEM_SEG     *p_seg,
                            CPU_ADDR     seg_base_addr,
                            CPU_SIZE_T   size,
                            CPU_SIZE_T   padding_align,
                            LIB_ERR     *p_err)
{
    CPU_SR_ALLOC();


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {                                    /* Chk for null err ptr.                                */
        CPU_SW_EXCEPTION(;);
    }

    if (p_seg == DEF_NULL) {                                    /* Chk for null seg ptr.                                */
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return;
    }

    if (size < 1u) {                                            /* Chk for invalid sized seg.                           */
       *p_err = LIB_MEM_ERR_INVALID_SEG_SIZE;
        return;
    }
                                                                /* Chk for addr space ovf.                              */
    if (seg_base_addr + (size - 1u) < seg_base_addr) {
       *p_err = LIB_MEM_ERR_INVALID_SEG_SIZE;
        return;
    }

    if ((padding_align               != LIB_MEM_PADDING_ALIGN_NONE) &&
        (MATH_IS_PWR2(padding_align) != DEF_YES)) {
       *p_err = LIB_MEM_ERR_INVALID_MEM_ALIGN;
        return;
    }
#endif

    CPU_CRITICAL_ENTER();
#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED) && \
    (LIB_MEM_CFG_HEAP_SIZE       > 0u)
    (void)Mem_SegOverlapChkCritical(seg_base_addr,              /* Chk for overlap.                                     */
                                    size,
                                    p_err);
    if (*p_err != LIB_MEM_ERR_NONE) {
        CPU_CRITICAL_EXIT();
        return;
    }
#endif

    Mem_SegCreateCritical(p_name,                               /* Create seg.                                          */
                          p_seg,
                          seg_base_addr,
                          padding_align,
                          size);
    CPU_CRITICAL_EXIT();

   *p_err = LIB_MEM_ERR_NONE;
}


/*
*********************************************************************************************************
*                                            Mem_SegClr()
*
* Description : Clears a memory segment.
*
* Argument(s) : p_seg           Pointer to segment data. Must be allocated by caller.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                               LIB_MEM_ERR_NONE                Operation was successful.
*                               LIB_MEM_ERR_NULL_PTR            Segment data pointer NULL.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) This function must be used with extreme caution. It must only be called on memory
*                   segments that are no longer used.
*
*               (2) This function is disabled when debug mode is enabled to avoid heap memory leaks.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_DISABLED)
void  Mem_SegClr (MEM_SEG  *p_seg,
                  LIB_ERR  *p_err)
{
    CPU_SR_ALLOC();


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {                                    /* Chk for null err ptr.                                */
        CPU_SW_EXCEPTION(;);
    }

    if (p_seg == DEF_NULL) {                                    /* Chk for null seg ptr.                                */
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return;
    }
#endif

    CPU_CRITICAL_ENTER();
    p_seg->AddrNext = p_seg->AddrBase;
    CPU_CRITICAL_EXIT();

   *p_err = LIB_MEM_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                                         Mem_SegRemSizeGet()
*
* Description : Gets free space of memory segment.
*
* Argument(s) : p_seg       Pointer to segment data.
*
*               align       Alignment in bytes to assume for calculation of free space.
*
*               p_seg_info  Pointer to structure that will receive further segment info data (used size,
*                           total size, base address and next allocation address).
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                           LIB_MEM_ERR_NONE                Operation was successful.
*                           LIB_MEM_ERR_NULL_PTR            Segment data pointer NULL.
*                           LIB_MEM_ERR_INVALID_MEM_ALIGN   Invalid memory alignment.
*
* Return(s)   : Memory segment remaining size in bytes,     if successful.
*               0,                                          otherwise or if memory segment empty.
*
* Caller(s)   : Application,
*               Mem_HeapGetSizeRem(),
*               Mem_OutputUsage().
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_SIZE_T  Mem_SegRemSizeGet (MEM_SEG       *p_seg,
                               CPU_SIZE_T     align,
                               MEM_SEG_INFO  *p_seg_info,
                               LIB_ERR       *p_err)
{
    CPU_SIZE_T  rem_size;
    CPU_SIZE_T  total_size;
    CPU_SIZE_T  used_size;
    CPU_ADDR    next_addr_align;
    CPU_SR_ALLOC();


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {                                    /* Chk for null err ptr.                                */
        CPU_SW_EXCEPTION(0);
    }

    if (MATH_IS_PWR2(align) != DEF_YES) {                       /* Chk for invalid align val.                           */
       *p_err = LIB_MEM_ERR_INVALID_MEM_ALIGN;
        return (0u);
    }
#endif

    if (p_seg == DEF_NULL) {                                    /* Dflt to heap in case p_seg is null.                  */
#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
        p_seg = &Mem_SegHeap;
#else
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return (0u);
#endif
    }

    CPU_CRITICAL_ENTER();                                       /* Calc seg stats.                                      */
    next_addr_align = MATH_ROUND_INC_UP_PWR2(p_seg->AddrNext, align);
    CPU_CRITICAL_EXIT();

    total_size = (p_seg->AddrEnd  - p_seg->AddrBase) + 1u;
    used_size  =  p_seg->AddrNext - p_seg->AddrBase;

    if (next_addr_align > p_seg->AddrEnd){
        next_addr_align = 0u;
        rem_size        = 0u;
    } else {
        rem_size        = total_size - (next_addr_align - p_seg->AddrBase);
    }

    if (p_seg_info != DEF_NULL) {
        p_seg_info->TotalSize     = total_size;
        p_seg_info->UsedSize      = used_size;
        p_seg_info->AddrBase      = p_seg->AddrBase;
        p_seg_info->AddrNextAlloc = next_addr_align;
    }

   *p_err = LIB_MEM_ERR_NONE;

    return (rem_size);
}


/*
*********************************************************************************************************
*                                            Mem_SegAlloc()
*
* Description : Allocates memory from specified segment. Returned memory block will be aligned on a CPU
*               word boundary.
*
* Argument(s) : p_name  Pointer to allocated object name. Used for allocations tracking. May be DEF_NULL.
*
*               p_seg   Pointer to segment from which to allocate memory. Will be allocated from
*                       general-purpose heap if null.
*
*               size    Size of memory block to allocate, in bytes.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           LIB_MEM_ERR_NONE                Operation was successful.
*
*                           ------------------RETURNED BY Mem_SegAllocInternal()-------------------
*                           LIB_MEM_ERR_INVALID_MEM_ALIGN   Invalid memory block alignment requested.
*                           LIB_MEM_ERR_INVALID_MEM_SIZE    Invalid memory block size specified.
*                           LIB_MEM_ERR_NULL_PTR            Error or segment data pointer NULL.
*                           LIB_MEM_ERR_SEG_OVF             Allocation would overflow memory segment.
*
* Return(s)   : Pointer to allocated memory block, if successful.
*
*               DEF_NULL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) The memory block returned  by this function will be aligned on a word boundary. In
*                   order to specify a specific alignment value, use either Mem_SegAllocExt() or
*                   Mem_SegAllocHW().
*********************************************************************************************************
*/

void  *Mem_SegAlloc (const  CPU_CHAR    *p_name,
                            MEM_SEG     *p_seg,
                            CPU_SIZE_T   size,
                            LIB_ERR     *p_err)
{
    void  *p_blk;


    if (p_seg == DEF_NULL) {                                    /* Alloc from heap if p_seg is null.                    */
#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
        p_seg = &Mem_SegHeap;
#else
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return (DEF_NULL);
#endif
    }

    p_blk = Mem_SegAllocInternal(p_name,
                                 p_seg,
                                 size,
                                 sizeof(CPU_ALIGN),
                                 LIB_MEM_PADDING_ALIGN_NONE,
                                 DEF_NULL,
                                 p_err);

    return (p_blk);
}


/*
*********************************************************************************************************
*                                           Mem_SegAllocExt()
*
* Description : Allocates memory from specified memory segment.
*
* Argument(s) : p_name          Pointer to allocated object name. Used for allocations tracking. May be DEF_NULL.
*
*               p_seg           Pointer to segment from which to allocate memory. Will be allocated from
*                               general-purpose heap if null.
*
*               size            Size of memory block to allocate, in bytes.
*
*               align           Required alignment of memory block, in bytes. MUST be a power of 2.
*
*               p_bytes_reqd    Pointer to variable that will receive the number of free bytes missing for
*                               the allocation to succeed. Set to DEF_NULL to skip calculation.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   LIB_MEM_ERR_NONE                Operation was successful.
*
*                                   ------------------RETURNED BY Mem_SegAllocInternal()-------------------
*                                   LIB_MEM_ERR_INVALID_MEM_ALIGN   Invalid memory block alignment requested.
*                                   LIB_MEM_ERR_INVALID_MEM_SIZE    Invalid memory block size specified.
*                                   LIB_MEM_ERR_NULL_PTR            Error or segment data pointer NULL.
*                                   LIB_MEM_ERR_SEG_OVF             Allocation would overflow memory segment.
*
* Return(s)   : Pointer to allocated memory block, if successful.
*
*               DEF_NULL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  *Mem_SegAllocExt (const  CPU_CHAR    *p_name,
                               MEM_SEG     *p_seg,
                               CPU_SIZE_T   size,
                               CPU_SIZE_T   align,
                               CPU_SIZE_T  *p_bytes_reqd,
                               LIB_ERR     *p_err)
{
    void  *p_blk;


    if (p_seg == DEF_NULL) {                                    /* Alloc from heap if p_seg is null.                    */
#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
        p_seg = &Mem_SegHeap;
#else
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return (DEF_NULL);
#endif
    }

    p_blk = Mem_SegAllocInternal(p_name,
                                 p_seg,
                                 size,
                                 align,
                                 LIB_MEM_PADDING_ALIGN_NONE,
                                 p_bytes_reqd,
                                 p_err);

    return (p_blk);
}


/*
*********************************************************************************************************
*                                          Mem_SegAllocHW()
*
* Description : Allocates memory from specified segment. The returned buffer will be padded in function
*               of memory segment's properties.
*
* Argument(s) : p_name          Pointer to allocated object name. Used for allocations tracking. May be DEF_NULL.
*
*               p_seg           Pointer to segment from which to allocate memory. Will be allocated from
*                               general-purpose heap if null.
*
*               size            Size of memory block to allocate, in bytes.
*
*               align           Required alignment of memory block, in bytes. MUST be a power of 2.
*
*               p_bytes_reqd    Pointer to variable that will receive the number of free bytes missing for
*                               the allocation to succeed. Set to DEF_NULL to skip calculation.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   LIB_MEM_ERR_NONE                Operation was successful.
*
*                                   ------------------RETURNED BY Mem_SegAllocInternal()-------------------
*                                   LIB_MEM_ERR_INVALID_MEM_ALIGN   Invalid memory block alignment requested.
*                                   LIB_MEM_ERR_INVALID_MEM_SIZE    Invalid memory block size specified.
*                                   LIB_MEM_ERR_NULL_PTR            Error or segment data pointer NULL.
*                                   LIB_MEM_ERR_SEG_OVF             Allocation would overflow memory segment.
*
* Return(s)   : Pointer to allocated memory block, if successful.
*
*               DEF_NULL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  *Mem_SegAllocHW (const  CPU_CHAR    *p_name,
                              MEM_SEG     *p_seg,
                              CPU_SIZE_T   size,
                              CPU_SIZE_T   align,
                              CPU_SIZE_T  *p_bytes_reqd,
                              LIB_ERR     *p_err)
{
    void  *p_blk;


    if (p_seg == DEF_NULL) {                                    /* Alloc from heap if p_seg is null.                    */
#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
        p_seg = &Mem_SegHeap;
#else
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return (DEF_NULL);
#endif
    }

    p_blk = Mem_SegAllocInternal(p_name,
                                 p_seg,
                                 size,
                                 align,
                                 p_seg->PaddingAlign,
                                 p_bytes_reqd,
                                 p_err);

    return (p_blk);
}


/*
*********************************************************************************************************
*                                          Mem_PoolCreate()
*
* Description : (1) Creates a memory pool :
*
*                   (a) Create    memory pool from heap or dedicated memory
*                   (b) Allocate  memory pool memory blocks
*                   (c) Configure memory pool
*
*
* Argument(s) : p_pool          Pointer to a memory pool structure to create (see Note #1).
*
*               p_mem_base      Memory pool segment base address :
*
*                                       (a)     Null address    Memory pool allocated from general-purpose heap.
*                                       (b) Non-null address    Memory pool allocated from dedicated memory
*                                                                   specified by its base address.
*
*               mem_size        Size      of memory pool segment          (in bytes).
*
*               blk_nbr         Number    of memory pool blocks to create.
*
*               blk_size        Size      of memory pool blocks to create (in bytes).
*
*               blk_align       Alignment of memory pool blocks to specific word boundary (in bytes).
*
*               p_bytes_reqd    Optional pointer to a variable to ... :
*
*                                   (a) Return the number of bytes required to successfully
*                                               allocate the memory pool, if any error(s);
*                                   (b) Return 0, otherwise.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   LIB_MEM_ERR_NONE                    Operation was successful.
*                                   LIB_MEM_ERR_NULL_PTR                Pointer to memory pool is null.
*                                   LIB_MEM_ERR_INVALID_BLK_ALIGN       Invalid block alignment requested.
*                                   LIB_MEM_ERR_INVALID_BLK_NBR         Invalid number of blocks specified.
*                                   LIB_MEM_ERR_INVALID_BLK_SIZE        Invalid block size specified.
*                                   LIB_MEM_ERR_INVALID_SEG_SIZE        Invalid segment size.
*                                   LIB_MEM_ERR_HEAP_EMPTY              No more memory available on heap.
*
*                                   ---------------RETURNED BY Mem_SegOverlapChkCritical()----------------
*                                   LIB_MEM_ERR_INVALID_SEG_EXISTS      Segment already exists.
*                                   LIB_MEM_ERR_INVALID_SEG_OVERLAP     Segment overlaps another existing segment.
*
*                                   -----------------RETURNED BY Mem_SegAllocExtCritical()-----------------
*                                   LIB_MEM_ERR_SEG_OVF                 Allocation would overflow memory segment.
*
*                                   ------------------RETURNED BY Mem_SegAllocInternal()-------------------
*                                   LIB_MEM_ERR_INVALID_MEM_ALIGN       Invalid memory block alignment requested.
*                                   LIB_MEM_ERR_INVALID_MEM_SIZE        Invalid memory block size specified.
*                                   LIB_MEM_ERR_NULL_PTR                Error or segment data pointer NULL.
*                                   LIB_MEM_ERR_SEG_OVF                 Allocation would overflow memory segment.
*
*                                   -----------------------RETURNED BY Mem_PoolClr()-----------------------
*                                   LIB_MEM_ERR_NULL_PTR                Argument 'p_pool' passed a NULL pointer.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) This function is DEPRECATED and will be removed in a future version of this product.
*                   Mem_DynPoolCreate() or Mem_DynPoolCreateHW() should be used instead.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
void  Mem_PoolCreate (MEM_POOL          *p_pool,
                      void              *p_mem_base,
                      CPU_SIZE_T         mem_size,
                      MEM_POOL_BLK_QTY   blk_nbr,
                      CPU_SIZE_T         blk_size,
                      CPU_SIZE_T         blk_align,
                      CPU_SIZE_T        *p_bytes_reqd,
                      LIB_ERR           *p_err)
{
    MEM_SEG           *p_seg;
    void              *p_pool_mem;
    CPU_SIZE_T         pool_size;
    CPU_SIZE_T         blk_size_align;
    CPU_ADDR           pool_addr_end;
    MEM_POOL_BLK_QTY   blk_ix;
    CPU_INT08U        *p_blk;
    CPU_SR_ALLOC();


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* --------------- VALIDATE RTN ERR PTR --------------- */
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

                                                                /* ------------- VALIDATE MEM POOL CREATE ------------- */
    if (p_pool == DEF_NULL) {
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return;
    }

    if (p_mem_base != DEF_NULL) {
        if (mem_size < 1u) {
           *p_err = LIB_MEM_ERR_INVALID_SEG_SIZE;
            return;
        }
    }

    if (blk_nbr < 1u) {
       *p_err = LIB_MEM_ERR_INVALID_BLK_NBR;
        return;
    }

    if (blk_size < 1u) {
       *p_err = LIB_MEM_ERR_INVALID_BLK_SIZE;
        return;
    }

    if (MATH_IS_PWR2(blk_align) != DEF_YES) {                   /* Chk that req alignment is a pwr of 2.                */
       *p_err = LIB_MEM_ERR_INVALID_BLK_ALIGN;
        return;
    }
#endif

    Mem_PoolClr(p_pool, p_err);                                 /* Init mem pool.                                       */
    if (*p_err != LIB_MEM_ERR_NONE) {
         return;
    }

                                                                /* -------- DETERMINE AND/OR ALLOC SEG TO USE --------- */
    if (p_mem_base == DEF_NULL) {                               /* Use heap seg.                                        */
        p_seg = &Mem_SegHeap;
    } else {                                                    /* Use other seg.                                       */
        CPU_CRITICAL_ENTER();
        p_seg = Mem_SegOverlapChkCritical((CPU_ADDR)p_mem_base,
                                                    mem_size,
                                                    p_err);
        switch (*p_err) {
            case LIB_MEM_ERR_INVALID_SEG_EXISTS:                /* Seg already exists.                                  */
                 break;

            case LIB_MEM_ERR_NONE:                              /* Seg must be created.                                 */
                 p_seg = (MEM_SEG *)Mem_SegAllocExtCritical(&Mem_SegHeap,
                                                             sizeof(MEM_SEG),
                                                             sizeof(CPU_ALIGN),
                                                             LIB_MEM_PADDING_ALIGN_NONE,
                                                             p_bytes_reqd,
                                                             p_err);
                 if (*p_err != LIB_MEM_ERR_NONE) {
                     CPU_CRITICAL_EXIT();
                     return;
                 }

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)                    /* Track alloc if req'd.                                */
                 Mem_SegAllocTrackCritical("Unknown segment data",
                                           &Mem_SegHeap,
                                            sizeof(MEM_SEG),
                                            p_err);
                 if (*p_err != LIB_MEM_ERR_NONE) {
                     CPU_CRITICAL_EXIT();
                     return;
                 }
#endif

                 Mem_SegCreateCritical(          DEF_NULL,
                                                 p_seg,
                                       (CPU_ADDR)p_mem_base,
                                                 LIB_MEM_PADDING_ALIGN_NONE,
                                                 mem_size);
                 break;


            case LIB_MEM_ERR_INVALID_SEG_OVERLAP:
            default:
                 CPU_CRITICAL_EXIT();
                 return;                                        /* Prevent 'break NOT reachable' compiler warning.      */
        }

        CPU_CRITICAL_EXIT();
    }


                                                                /* ---------------- ALLOC MEM FOR POOL ---------------- */
                                                                /* Calc blk size with align.                            */
    blk_size_align =  MATH_ROUND_INC_UP_PWR2(blk_size, blk_align);
    pool_size      =  blk_size_align * blk_nbr;                 /* Calc required size for pool.                         */

                                                                /* Alloc mem for pool.                                  */
    p_pool_mem = (void *)Mem_SegAllocInternal("Unnamed static pool",
                                               p_seg,
                                               pool_size,
                                               blk_align,
                                               LIB_MEM_PADDING_ALIGN_NONE,
                                               p_bytes_reqd,
                                               p_err);
    if (*p_err != LIB_MEM_ERR_NONE) {
        return;
    }

                                                                /* ------------ ALLOC MEM FOR FREE BLK TBL ------------ */
    p_pool->BlkFreeTbl = (void **)Mem_SegAllocInternal("Unnamed static pool free blk tbl",
                                                       &Mem_SegHeap,
                                                        blk_nbr * sizeof(void *),
                                                        sizeof(CPU_ALIGN),
                                                        LIB_MEM_PADDING_ALIGN_NONE,
                                                        p_bytes_reqd,
                                                        p_err);
    if (*p_err != LIB_MEM_ERR_NONE) {
        return;
    }

                                                                /* ------------------ INIT BLK LIST ------------------- */
    p_blk = (CPU_INT08U *)p_pool_mem;
    for (blk_ix = 0; blk_ix < blk_nbr; blk_ix++) {
        p_pool->BlkFreeTbl[blk_ix]  = p_blk;
        p_blk                      += blk_size_align;
    }


                                                                /* ------------------ INIT POOL DATA ------------------ */
    pool_addr_end         = (CPU_ADDR)p_pool_mem + (pool_size - 1u);
    p_pool->PoolAddrStart =  p_pool_mem;
    p_pool->PoolAddrEnd   = (void *)pool_addr_end;
    p_pool->BlkNbr        =  blk_nbr;
    p_pool->BlkSize       =  blk_size_align;
    p_pool->BlkFreeTblIx  =  blk_nbr;
}
#endif


/*
*********************************************************************************************************
*                                            Mem_PoolClr()
*
* Description : Clears a memory pool (see Note #1).
*
* Argument(s) : p_pool   Pointer to a memory pool structure to clear (see Note #2).
*
*               p_err    Pointer to variable that will receive the return error code from this function :
*
*                               LIB_MEM_ERR_NONE                Operation was successful.
*                               LIB_MEM_ERR_NULL_PTR            Argument 'p_pool' passed a NULL pointer.
*
* Return(s)   : none.
*
* Caller(s)   : Application,
*               Mem_PoolCreate().
*
* Note(s)     : (1) (a) Mem_PoolClr() ONLY clears a memory pool structure's variables & should ONLY be
*                       called to initialize a memory pool structure prior to calling Mem_PoolCreate().
*
*                   (b) Mem_PoolClr() does NOT deallocate memory from the memory pool or deallocate the
*                       memory pool itself & MUST NOT be called after calling Mem_PoolCreate() since
*                       this will likely corrupt the memory pool management.
*
*               (2) Assumes 'p_pool' points to a valid memory pool (if non-NULL).
*
*               (3) This function is DEPRECATED and will be removed in a future version of this product.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
void  Mem_PoolClr (MEM_POOL  *p_pool,
                   LIB_ERR   *p_err)
{
#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* -------------- VALIDATE RTN ERR  PTR --------------- */
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

                                                                /* -------------- VALIDATE MEM POOL PTR --------------- */
    if (p_pool == DEF_NULL) {
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return;
    }
#endif

    p_pool->PoolAddrStart = DEF_NULL;
    p_pool->PoolAddrEnd   = DEF_NULL;
    p_pool->BlkSize       = 0u;
    p_pool->BlkNbr        = 0u;
    p_pool->BlkFreeTbl    = DEF_NULL;
    p_pool->BlkFreeTblIx  = 0u;

   *p_err = LIB_MEM_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                                          Mem_PoolBlkGet()
*
* Description : Gets a memory block from memory pool.
*
* Argument(s) : p_pool  Pointer to  memory pool to get memory block from.
*
*               size    Size of requested memory (in bytes).
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           LIB_MEM_ERR_NONE                Operation was successful.
*                           LIB_MEM_ERR_INVALID_BLK_SIZE    Invalid memory pool block size requested.
*                           LIB_MEM_ERR_NULL_PTR            Argument 'p_pool' passed a NULL pointer.
*                           LIB_MEM_ERR_POOL_EMPTY          NO memory blocks available in memory pool.
*
* Return(s)   : Pointer to memory block, if NO error(s).
*
*               Pointer to NULL,         otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) This function is DEPRECATED and will be removed in a future version of this product.
*                   Mem_DynPoolBlkGet() should be used instead.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
void  *Mem_PoolBlkGet (MEM_POOL    *p_pool,
                       CPU_SIZE_T   size,
                       LIB_ERR     *p_err)
{
    CPU_INT08U  *p_blk;
    CPU_SR_ALLOC();


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* -------------- VALIDATE MEM POOL GET --------------- */
    if (p_err == DEF_NULL) {                                    /* Validate err ptr.                                    */
        CPU_SW_EXCEPTION(DEF_NULL);
    }

    if (p_pool == DEF_NULL) {                                   /* Validate pool ptr.                                   */
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return (DEF_NULL);
    }

    if (size < 1u) {                                            /* Validate req'd size as non-NULL.                     */
       *p_err = LIB_MEM_ERR_INVALID_BLK_SIZE;
        return (DEF_NULL);
    }

    if (size > p_pool->BlkSize) {                               /* Validate req'd size <= mem pool blk size.            */
       *p_err = LIB_MEM_ERR_INVALID_BLK_SIZE;
        return (DEF_NULL);
    }
#else
    (void)size;                                                 /* Prevent possible 'variable unused' warning.          */
#endif


                                                                /* -------------- GET MEM BLK FROM POOL --------------- */
    p_blk = DEF_NULL;
    CPU_CRITICAL_ENTER();
    if (p_pool->BlkFreeTblIx > 0u) {
        p_pool->BlkFreeTblIx                     -=  1u;
        p_blk                                     = (CPU_INT08U *)p_pool->BlkFreeTbl[p_pool->BlkFreeTblIx];
        p_pool->BlkFreeTbl[p_pool->BlkFreeTblIx]  =  DEF_NULL;
    }
    CPU_CRITICAL_EXIT();

    if (p_blk == DEF_NULL) {
       *p_err = LIB_MEM_ERR_POOL_EMPTY;
    } else {
       *p_err = LIB_MEM_ERR_NONE;
    }

    return (p_blk);
}
#endif


/*
*********************************************************************************************************
*                                          Mem_PoolBlkFree()
*
* Description : Free a memory block to memory pool.
*
* Argument(s) : p_pool  Pointer to memory pool to free memory block.
*
*               p_blk   Pointer to memory block address to free.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           LIB_MEM_ERR_NONE                        Operation was successful.
*                           LIB_MEM_ERR_NULL_PTR                    Argument 'p_pool'/'p_blk' passed
*                                                                       a NULL pointer.
*                           LIB_MEM_ERR_INVALID_BLK_ADDR            Invalid memory block address.
*                           LIB_MEM_ERR_INVALID_BLK_ADDR_IN_POOL            Memory block address already
*                                                                        in memory pool.
*                           LIB_MEM_ERR_POOL_FULL                   Pool is full.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) This function is DEPRECATED and will be removed in a future version of this product.
*                   Mem_DynPoolBlkFree() should be used instead.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
void  Mem_PoolBlkFree (MEM_POOL  *p_pool,
                       void      *p_blk,
                       LIB_ERR   *p_err)
{
#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    CPU_SIZE_T   tbl_ix;
    CPU_BOOLEAN  addr_valid;
#endif
    CPU_SR_ALLOC();


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* -------------- VALIDATE MEM POOL FREE -------------- */
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(;);
    }

    if (p_pool == DEF_NULL) {                                   /* Validate mem ptrs.                                   */
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return;
    }

    if (p_blk == DEF_NULL) {
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return;
    }

    addr_valid = Mem_PoolBlkIsValidAddr(p_pool, p_blk);         /* Validate mem blk as valid pool blk addr.             */
    if (addr_valid != DEF_OK) {
       *p_err = LIB_MEM_ERR_INVALID_BLK_ADDR;
        return;
    }

    CPU_CRITICAL_ENTER();                                       /* Make sure blk isn't already in free list.            */
    for (tbl_ix = 0u; tbl_ix < p_pool->BlkNbr; tbl_ix++) {
        if (p_pool->BlkFreeTbl[tbl_ix] == p_blk) {
            CPU_CRITICAL_EXIT();
           *p_err = LIB_MEM_ERR_INVALID_BLK_ADDR_IN_POOL;
            return;
        }
    }
#else                                                           /* Double-free possibility if not in critical section.  */
    CPU_CRITICAL_ENTER();
#endif
                                                                /* --------------- FREE MEM BLK TO POOL --------------- */
    if (p_pool->BlkFreeTblIx >= p_pool->BlkNbr) {
        CPU_CRITICAL_EXIT();
       *p_err = LIB_MEM_ERR_POOL_FULL;
        return;
    }

    p_pool->BlkFreeTbl[p_pool->BlkFreeTblIx]  = p_blk;
    p_pool->BlkFreeTblIx                     += 1u;
    CPU_CRITICAL_EXIT();

   *p_err = LIB_MEM_ERR_NONE;
}
#endif


/*
*********************************************************************************************************
*                                      Mem_PoolBlkGetNbrAvail()
*
* Description : Get memory pool's remaining number of blocks available to allocate.
*
* Argument(s) : p_pool   Pointer to a memory pool structure.
*
*               p_err    Pointer to variable that will receive the return error code from this function :
*
*                               LIB_MEM_ERR_NONE                Operation was successful.
*                               LIB_MEM_ERR_NULL_PTR            Argument 'p_pool' passed a NULL pointer.
*
* Return(s)   : Remaining memory pool blocks,   if NO error(s).
*
*               0,                              otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) This function is DEPRECATED and will be removed in a future version of this product.
*                   Mem_DynPoolBlkNbrAvailGet() should be used instead.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
MEM_POOL_BLK_QTY  Mem_PoolBlkGetNbrAvail (MEM_POOL  *p_pool,
                                          LIB_ERR   *p_err)
{
    CPU_SIZE_T  nbr_avail;
    CPU_SR_ALLOC();


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
                                                                /* --------------- VALIDATE RTN ERR PTR --------------- */
    if (p_err == DEF_NULL) {
        CPU_SW_EXCEPTION(0u);
    }
                                                                /* ---------------- VALIDATE MEM POOL ----------------- */
    if (p_pool == DEF_NULL) {                                   /* Validate mem ptr.                                    */
       *p_err =  LIB_MEM_ERR_NULL_PTR;
        return (0u);
    }
#endif

    CPU_CRITICAL_ENTER();
    nbr_avail = p_pool->BlkFreeTblIx;
    CPU_CRITICAL_EXIT();

   *p_err = LIB_MEM_ERR_NONE;

    return (nbr_avail);
}
#endif


/*
*********************************************************************************************************
*                                          Mem_DynPoolCreate()
*
* Description : Creates a dynamic memory pool.
*
* Argument(s) : p_name          Pointer to pool name.
*
*               p_pool          Pointer to pool data.
*
*               p_seg           Pointer to segment from which to allocate memory. Will be allocated from
*                               general-purpose heap if null.
*
*               blk_size        Size of memory block to allocate from pool, in bytes. See Note #1.
*
*               blk_align       Required alignment of memory block, in bytes. MUST be a power of 2.
*
*               blk_qty_init    Initial number of elements to be allocated in pool.
*
*               blk_qty_max     Maximum number of elements that can be allocated from this pool. Set to
*                               LIB_MEM_BLK_QTY_UNLIMITED if no limit.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   LIB_MEM_ERR_NONE                Operation was successful.
*
*                                   --------------------RETURNED BY Mem_DynPoolCreateInternal()-------------------
*                                   LIB_MEM_ERR_INVALID_BLK_ALIGN   Invalid requested block alignment.
*                                   LIB_MEM_ERR_INVALID_BLK_SIZE    Invalid requested block size.
*                                   LIB_MEM_ERR_INVALID_BLK_NBR     Invalid requested block quantity max.
*                                   LIB_MEM_ERR_NULL_PTR            Pool data pointer NULL.
*                                   LIB_MEM_ERR_INVALID_MEM_ALIGN   Invalid memory block alignment requested.
*                                   LIB_MEM_ERR_INVALID_MEM_SIZE    Invalid memory block size specified.
*                                   LIB_MEM_ERR_NULL_PTR            Error or segment data pointer NULL.
*                                   LIB_MEM_ERR_SEG_OVF             Allocation would overflow memory segment.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) 'blk_size' must be big enough to fit a pointer since the pointer to the next free
*                   block is stored in the block itself (only when free/unused).
*********************************************************************************************************
*/

void  Mem_DynPoolCreate (const  CPU_CHAR      *p_name,
                                MEM_DYN_POOL  *p_pool,
                                MEM_SEG       *p_seg,
                                CPU_SIZE_T     blk_size,
                                CPU_SIZE_T     blk_align,
                                CPU_SIZE_T     blk_qty_init,
                                CPU_SIZE_T     blk_qty_max,
                                LIB_ERR       *p_err)
{
    if (p_seg == DEF_NULL) {                                    /* Alloc from heap if p_seg is null.                    */
#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
        p_seg = &Mem_SegHeap;
#else
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return;
#endif
    }

    Mem_DynPoolCreateInternal(p_name,
                              p_pool,
                              p_seg,
                              blk_size,
                              blk_align,
                              LIB_MEM_PADDING_ALIGN_NONE,
                              blk_qty_init,
                              blk_qty_max,
                              p_err);
}


/*
*********************************************************************************************************
*                                        Mem_DynPoolCreateHW()
*
* Description : Creates a dynamic memory pool. Memory blocks will be padded according to memory segment's
*               properties.
*
* Argument(s) : p_name          Pointer to pool name.
*
*               p_pool          Pointer to pool data.
*
*               p_seg           Pointer to segment from which to allocate memory. Will allocate from
*                               general-purpose heap if null.
*
*               blk_size        Size of memory block to allocate from pool, in bytes. See Note #1.
*
*               blk_align       Required alignment of memory block, in bytes. MUST be a power of 2.
*
*               blk_qty_init    Initial number of elements to be allocated in pool.
*
*               blk_qty_max     Maximum number of elements that can be allocated from this pool. Set to
*                               LIB_MEM_BLK_QTY_UNLIMITED if no limit.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   LIB_MEM_ERR_NONE                Operation was successful.
*
*                                   -------------------RETURNED BY Mem_DynPoolCreateInternal()-------------------
*                                   LIB_MEM_ERR_INVALID_MEM_ALIGN   Invalid memory block alignment requested.
*                                   LIB_MEM_ERR_INVALID_MEM_SIZE    Invalid memory block size specified.
*                                   LIB_MEM_ERR_NULL_PTR            Error or segment data pointer NULL.
*                                   LIB_MEM_ERR_SEG_OVF             Allocation would overflow memory segment.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : (1) 'blk_size' must be big enough to fit a pointer since the pointer to the next free
*                   block is stored in the block itself (only when free/unused).
*********************************************************************************************************
*/

void  Mem_DynPoolCreateHW (const  CPU_CHAR      *p_name,
                                  MEM_DYN_POOL  *p_pool,
                                  MEM_SEG       *p_seg,
                                  CPU_SIZE_T     blk_size,
                                  CPU_SIZE_T     blk_align,
                                  CPU_SIZE_T     blk_qty_init,
                                  CPU_SIZE_T     blk_qty_max,
                                  LIB_ERR       *p_err)
{
    if (p_seg == DEF_NULL) {                                    /* Alloc from heap if p_seg is null.                    */
#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
        p_seg = &Mem_SegHeap;
#else
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return;
#endif
    }

    Mem_DynPoolCreateInternal(p_name,
                              p_pool,
                              p_seg,
                              blk_size,
                              blk_align,
                              p_seg->PaddingAlign,
                              blk_qty_init,
                              blk_qty_max,
                              p_err);
}


/*
*********************************************************************************************************
*                                          Mem_DynPoolBlkGet()
*
* Description : Gets a memory block from specified pool, growing it if needed.
*
* Argument(s) : p_pool  Pointer to pool data.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           LIB_MEM_ERR_NONE                    Operation was successful.
*                           LIB_MEM_ERR_NULL_PTR                Pool data pointer NULL.
*                           LIB_MEM_ERR_POOL_EMPTY              Pools is empty.
*
*                           ----------------------RETURNED BY Mem_SegAllocInternal()-----------------------
*                           LIB_MEM_ERR_INVALID_MEM_ALIGN       Invalid memory block alignment requested.
*                           LIB_MEM_ERR_INVALID_MEM_SIZE        Invalid memory block size specified.
*                           LIB_MEM_ERR_NULL_PTR                Error or segment data pointer NULL.
*                           LIB_MEM_ERR_SEG_OVF                 Allocation would overflow memory segment.
*
* Return(s)   : Pointer to memory block, if successful.
*
*               DEF_NULL, otherwise.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  *Mem_DynPoolBlkGet (MEM_DYN_POOL  *p_pool,
                          LIB_ERR       *p_err)
{
           void      *p_blk;
    const  CPU_CHAR  *p_pool_name;
    CPU_SR_ALLOC();


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {                                    /* Chk for NULL err ptr.                                */
        CPU_SW_EXCEPTION(DEF_NULL);
    }

    if (p_pool == DEF_NULL) {                                   /* Chk for NULL pool data ptr.                          */
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return (DEF_NULL);
    }
#endif

                                                                /* Ensure pool is not empty if qty is limited.          */
    if (p_pool->BlkQtyMax != LIB_MEM_BLK_QTY_UNLIMITED) {
        CPU_CRITICAL_ENTER();
        if (p_pool->BlkAllocCnt >= p_pool->BlkQtyMax) {
            CPU_CRITICAL_EXIT();

           *p_err = LIB_MEM_ERR_POOL_EMPTY;
            return (DEF_NULL);
        }

        p_pool->BlkAllocCnt++;
        CPU_CRITICAL_EXIT();
    }

                                                                /* --------------- ALLOC FROM FREE LIST --------------- */
    CPU_CRITICAL_ENTER();
    if (p_pool->BlkFreePtr != DEF_NULL) {
        p_blk              = p_pool->BlkFreePtr;
        p_pool->BlkFreePtr = *((void **)p_blk);
        CPU_CRITICAL_EXIT();

       *p_err = LIB_MEM_ERR_NONE;

        return (p_blk);
    }
    CPU_CRITICAL_EXIT();

                                                                /* ------------------ ALLOC NEW BLK ------------------- */
#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
    p_pool_name = p_pool->NamePtr;
#else
    p_pool_name = DEF_NULL;
#endif
    p_blk = Mem_SegAllocInternal(p_pool_name,
                                 p_pool->PoolSegPtr,
                                 p_pool->BlkSize,
                                 p_pool->BlkAlign,
                                 p_pool->BlkPaddingAlign,
                                 DEF_NULL,
                                 p_err);
    if (*p_err != LIB_MEM_ERR_NONE) {
        if (p_pool->BlkQtyMax != LIB_MEM_BLK_QTY_UNLIMITED) {
            p_pool->BlkAllocCnt--;
        }
        return (DEF_NULL);
    }

    return (p_blk);
}


/*
*********************************************************************************************************
*                                         Mem_DynPoolBlkFree()
*
* Description : Frees memory block, making it available for future use.
*
* Argument(s) : p_pool  Pointer to pool data.
*
*               p_blk   Pointer to first byte of memory block.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           LIB_MEM_ERR_NONE        Operation was successful.
*                           LIB_MEM_ERR_NULL_PTR    'p_pool' or 'p_blk' pointer passed is NULL.
*                           LIB_MEM_ERR_POOL_FULL   Pool is full.
*
* Return(s)   : none.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  Mem_DynPoolBlkFree (MEM_DYN_POOL  *p_pool,
                          void          *p_blk,
                          LIB_ERR       *p_err)
{
    CPU_SR_ALLOC();


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {                                    /* Chk for NULL err ptr.                                */
        CPU_SW_EXCEPTION(;);
    }

    if (p_pool == DEF_NULL) {                                   /* Chk for NULL pool data ptr.                          */
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return;
    }

    if (p_blk == DEF_NULL) {
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return;
    }
#endif

    if (p_pool->BlkQtyMax != LIB_MEM_BLK_QTY_UNLIMITED) {       /* Ensure pool is not full.                             */
        CPU_CRITICAL_ENTER();
        if (p_pool->BlkAllocCnt == 0u) {
            CPU_CRITICAL_EXIT();

           *p_err = LIB_MEM_ERR_POOL_FULL;
            return;
        }

        p_pool->BlkAllocCnt--;
        CPU_CRITICAL_EXIT();
    }

    CPU_CRITICAL_ENTER();
   *((void **)p_blk)   = p_pool->BlkFreePtr;
    p_pool->BlkFreePtr = p_blk;
    CPU_CRITICAL_EXIT();

   *p_err = LIB_MEM_ERR_NONE;
}


/*
*********************************************************************************************************
*                                     Mem_DynPoolBlkNbrAvailGet()
*
* Description : Gets number of available blocks in dynamic memory pool. This call will fail with a
*               dynamic memory pool for which no limit was set at creation.
*
* Argument(s) : p_pool  Pointer to pool data.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           LIB_MEM_ERR_NONE                Operation was successful.
*                           LIB_MEM_ERR_NULL_PTR            'p_pool' pointer passed is NULL.
*                           LIB_MEM_ERR_POOL_UNLIMITED      Pool has no specified limit.
*
* Return(s)   : Number of blocks available in dynamic memory pool, if successful.
*
*               0, if pool is empty or if an error occurred.
*
* Caller(s)   : Application.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_SIZE_T  Mem_DynPoolBlkNbrAvailGet (MEM_DYN_POOL  *p_pool,
                                       LIB_ERR       *p_err)
{
    CPU_SIZE_T  blk_nbr_avail;
    CPU_SR_ALLOC();


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {                                    /* Chk for NULL err ptr.                                */
        CPU_SW_EXCEPTION(0);
    }

    if (p_pool == DEF_NULL) {                                   /* Chk for NULL pool data ptr.                          */
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return (0u);
    }
#endif

    if (p_pool->BlkQtyMax != LIB_MEM_BLK_QTY_UNLIMITED) {
        CPU_CRITICAL_ENTER();
        blk_nbr_avail = p_pool->BlkQtyMax - p_pool->BlkAllocCnt;
        CPU_CRITICAL_EXIT();

       *p_err = LIB_MEM_ERR_NONE;
    } else {
        blk_nbr_avail = 0u;
       *p_err         = LIB_MEM_ERR_POOL_UNLIMITED;
    }

    return (blk_nbr_avail);
}


/*
*********************************************************************************************************
*                                           Mem_OutputUsage()
*
* Description : Outputs memory usage report through 'out_fnct'.
*
* Argument(s) : out_fnct        Pointer to output function.
*
*               print_details   DEF_YES, if the size of each allocation should be printed.
*                               DEF_NO,  otherwise.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   LIB_MEM_ERR_NONE                Operation was successful.
*                                   LIB_MEM_ERR_NULL_PTR            'out_fnct' pointer passed is NULL.
*
*                                   ---------------------RETURNED BY Mem_SegRemSizeGet()--------------------
*                                   LIB_MEM_ERR_NULL_PTR            Segment data pointer NULL.
*                                   LIB_MEM_ERR_INVALID_MEM_ALIGN   Invalid memory alignment.
*
* Return(s)   : None.
*
* Caller(s)   : Application.
*
* Note(s)     : none.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
void  Mem_OutputUsage(void     (*out_fnct) (CPU_CHAR *),
                      LIB_ERR   *p_err)
{
    CPU_CHAR   str[DEF_INT_32U_NBR_DIG_MAX];
    MEM_SEG   *p_seg;
    CPU_SR_ALLOC();


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {                                    /* Chk for NULL err ptr.                                */
        CPU_SW_EXCEPTION(;);
    }

    if (out_fnct == DEF_NULL) {                                 /* Chk for NULL out fnct ptr.                           */
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return;
    }
#endif

    out_fnct((CPU_CHAR *)"---------------- Memory allocation info ----------------\r\n");
    out_fnct((CPU_CHAR *)"| Type    | Size       | Free size  | Name\r\n");
    out_fnct((CPU_CHAR *)"|---------|------------|------------|-------------------\r\n");

    CPU_CRITICAL_ENTER();
    p_seg = Mem_SegHeadPtr;
    while (p_seg != DEF_NULL) {
        CPU_SIZE_T       rem_size;
        MEM_SEG_INFO     seg_info;
        MEM_ALLOC_INFO  *p_alloc;


        rem_size = Mem_SegRemSizeGet(p_seg, 1u, &seg_info, p_err);
        if (*p_err != LIB_MEM_ERR_NONE) {
            return;
        }

        out_fnct((CPU_CHAR *)"| Section | ");

        (void)Str_FmtNbr_Int32U(seg_info.TotalSize,
                                10u,
                                DEF_NBR_BASE_DEC,
                                ' ',
                                DEF_NO,
                                DEF_YES,
                               &str[0u]);

        out_fnct(str);
        out_fnct((CPU_CHAR *)" | ");

        (void)Str_FmtNbr_Int32U(rem_size,
                                10u,
                                DEF_NBR_BASE_DEC,
                                ' ',
                                DEF_NO,
                                DEF_YES,
                               &str[0u]);

        out_fnct(str);
        out_fnct((CPU_CHAR *)" | ");
        out_fnct((p_seg->NamePtr != DEF_NULL) ? (CPU_CHAR *)p_seg->NamePtr : (CPU_CHAR *)"Unknown");
        out_fnct((CPU_CHAR *)"\r\n");

        p_alloc = p_seg->AllocInfoHeadPtr;
        while (p_alloc != DEF_NULL) {
            out_fnct((CPU_CHAR *)"| -> Obj  | ");

            (void)Str_FmtNbr_Int32U(p_alloc->Size,
                                    10u,
                                    DEF_NBR_BASE_DEC,
                                    ' ',
                                    DEF_NO,
                                    DEF_YES,
                                   &str[0u]);

            out_fnct(str);
            out_fnct((CPU_CHAR *)" |            | ");

            out_fnct((p_alloc->NamePtr != DEF_NULL) ? (CPU_CHAR *)p_alloc->NamePtr : (CPU_CHAR *)"Unknown");
            out_fnct((CPU_CHAR *)"\r\n");

            p_alloc = p_alloc->NextPtr;
        }

        p_seg = p_seg->NextPtr;
    }
    CPU_CRITICAL_EXIT();

   *p_err = LIB_MEM_ERR_NONE;
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
*                                       Mem_SegCreateCritical()
*
* Description : Creates a new memory segment to be used for runtime memory allocation or dynamic pools.
*
* Argument(s) : p_name          Pointer to segment name.
*
*               p_seg           Pointer to segment data. Must be allocated by caller.
*               -----           Argument validated by caller.
*
*               seg_base_addr   Segment's first byte address.
*
*               padding_align   Padding alignment, in bytes, that will be added to any allocated buffer
*                               from this memory segment. MUST be a power of 2.
*                               LIB_MEM_PADDING_ALIGN_NONE means no padding.
*               -------------   Argument validated by caller.
*
*               size            Total size of segment, in bytes.
*               ----            Argument validated by caller.
*
* Return(s)   : Pointer to segment data, if successful.
*
*               DEF_NULL, otherwise.
*
* Caller(s)   : Mem_PoolCreate(),
*               Mem_SegCreate().
*
* Note(s)     : (1) This function MUST be called within a CRITICAL_SECTION.
*********************************************************************************************************
*/

static  void  Mem_SegCreateCritical(const  CPU_CHAR    *p_name,
                                           MEM_SEG     *p_seg,
                                           CPU_ADDR     seg_base_addr,
                                           CPU_SIZE_T   padding_align,
                                           CPU_SIZE_T   size)
{
    p_seg->AddrBase         =  seg_base_addr;
    p_seg->AddrEnd          = (seg_base_addr + (size - 1u));
    p_seg->AddrNext         =  seg_base_addr;
    p_seg->NextPtr          =  Mem_SegHeadPtr;
    p_seg->PaddingAlign     =  padding_align;

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
    p_seg->NamePtr          = p_name;
    p_seg->AllocInfoHeadPtr = DEF_NULL;
#else
    (void)p_name;
#endif

    Mem_SegHeadPtr = p_seg;
}


/*
*********************************************************************************************************
*                                      Mem_SegOverlapChkCritical()
*
* Description : Checks if existing memory segment exists or overlaps with specified memory area.
*
* Argument(s) : seg_base_addr   Address of first byte of memory area.
*
*               size            Size of memory area, in bytes.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               LIB_MEM_ERR_INVALID_SEG_OVERLAP     Segment overlaps another existing segment.
*                               LIB_MEM_ERR_INVALID_SEG_EXISTS      Segment already exists.
*
* Return(s)   : Pointer to memory segment that overlaps.
*
*               DEF_NULL, otherwise.
*
* Caller(s)   : Mem_PoolCreate(),
*               Mem_SegCreate().
*
* Note(s)     : (1) This function MUST be called within a CRITICAL_SECTION.
*********************************************************************************************************
*/

#if  (LIB_MEM_CFG_HEAP_SIZE      >  0u)
static  MEM_SEG  *Mem_SegOverlapChkCritical (CPU_ADDR     seg_base_addr,
                                             CPU_SIZE_T   size,
                                             LIB_ERR     *p_err)
{
    MEM_SEG   *p_seg_chk;
    CPU_ADDR   seg_new_end;
    CPU_ADDR   seg_chk_start;
    CPU_ADDR   seg_chk_end;


    seg_new_end = seg_base_addr + (size - 1u);
    p_seg_chk   = Mem_SegHeadPtr;

    while (p_seg_chk != DEF_NULL) {
        seg_chk_start = (CPU_ADDR)p_seg_chk->AddrBase;
        seg_chk_end   = (CPU_ADDR)p_seg_chk->AddrEnd;

        if ((seg_base_addr == seg_chk_start) && (seg_new_end == seg_chk_end)) {
           *p_err = LIB_MEM_ERR_INVALID_SEG_EXISTS;
            return (p_seg_chk);
        } else if (((seg_base_addr >= seg_chk_start) && (seg_base_addr <= seg_chk_end)) ||
                   ((seg_base_addr <= seg_chk_start) && (seg_new_end   >= seg_chk_start))) {
           *p_err = LIB_MEM_ERR_INVALID_SEG_OVERLAP;
            return (p_seg_chk);
        } else {
                                                                /* Empty Else Statement                                 */
        }

        p_seg_chk = p_seg_chk->NextPtr;
    }

   *p_err = LIB_MEM_ERR_NONE;

    return (DEF_NULL);
}
#endif


/*
*********************************************************************************************************
*                                       Mem_SegAllocInternal()
*
* Description : Allocates memory from specified segment.
*
* Argument(s) : p_name  Pointer to allocated object name. Used for allocations tracking. May be DEF_NULL.
*
*               p_seg           Pointer to segment from which to allocate memory.
*               -----           Argument validated by caller.
*
*               size            Size of memory block to allocate, in bytes.
*
*               align           Required alignment of memory block, in bytes. MUST be a power of 2.
*
*               padding_align   Padding alignment, in bytes, that will be added to any allocated buffer from
*                               this memory segment. MUST be a power of 2. LIB_MEM_PADDING_ALIGN_NONE
*                               means no padding.
*
*               p_bytes_reqd    Pointer to variable that will receive the number of free bytes missing for
*                               the allocation to succeed. Set to DEF_NULL to skip calculation.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   LIB_MEM_ERR_INVALID_MEM_ALIGN   Invalid memory block alignment requested.
*                                   LIB_MEM_ERR_INVALID_MEM_SIZE    Invalid memory block size specified.
*                                   LIB_MEM_ERR_NULL_PTR            Error or segment data pointer NULL.
*
*                                   ------------------RETURNED BY Mem_SegAllocExtCritical()------------------
*                                   LIB_MEM_ERR_SEG_OVF             Allocation would overflow memory segment.
*
* Return(s)   : Pointer to allocated memory block, if successful.
*
*               DEF_NULL, otherwise.
*
* Caller(s)   : Mem_DynPoolBlkGet(),
*               Mem_DynPoolCreateInternal(),
*               Mem_HeapAlloc(),
*               Mem_PoolCreate(),
*               Mem_SegAlloc(),
*               Mem_SegAllocExt(),
*               Mem_SegAllocHW().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  *Mem_SegAllocInternal (const  CPU_CHAR    *p_name,
                                            MEM_SEG     *p_seg,
                                            CPU_SIZE_T   size,
                                            CPU_SIZE_T   align,
                                            CPU_SIZE_T   padding_align,
                                            CPU_SIZE_T  *p_bytes_reqd,
                                            LIB_ERR     *p_err)
{
    void  *p_blk;
    CPU_SR_ALLOC();


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {                                    /* Chk for null err ptr.                                */
        CPU_SW_EXCEPTION(DEF_NULL);
    }

    if (size < 1u) {                                            /* Chk for invalid sized mem req.                       */
       *p_err = LIB_MEM_ERR_INVALID_MEM_SIZE;
        return (DEF_NULL);
    }

    if (MATH_IS_PWR2(align) != DEF_YES) {                       /* Chk that align is a pwr of 2.                        */
       *p_err = LIB_MEM_ERR_INVALID_MEM_ALIGN;
        return (DEF_NULL);
    }
#endif

    CPU_CRITICAL_ENTER();
    p_blk = Mem_SegAllocExtCritical(p_seg,
                                    size,
                                    align,
                                    padding_align,
                                    p_bytes_reqd,
                                    p_err);
    if (*p_err != LIB_MEM_ERR_NONE) {
        CPU_CRITICAL_EXIT();
        return (DEF_NULL);
    }

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)                    /* Track alloc if req'd.                                */
    Mem_SegAllocTrackCritical(p_name,
                              p_seg,
                              size,
                              p_err);
    if (*p_err != LIB_MEM_ERR_NONE) {
        CPU_CRITICAL_EXIT();
        return (DEF_NULL);
    }
#else
    (void)p_name;
#endif
    CPU_CRITICAL_EXIT();

    return (p_blk);
}


/*
*********************************************************************************************************
*                                      Mem_SegAllocExtCritical()
*
* Description : Allocates memory from specified segment.
*
* Argument(s) : p_seg           Pointer to segment from which to allocate memory.
*
*               size            Size of memory block to allocate, in bytes.
*
*               align           Required alignment of memory block, in bytes. MUST be a power of 2.
*
*               padding_align   Padding alignment, in bytes, that will be added to any allocated buffer from
*                               this memory segment. MUST be a power of 2. LIB_MEM_PADDING_ALIGN_NONE
*                               means no padding.
*
*               p_bytes_reqd    Pointer to variable that will receive the number of free bytes missing for
*                               the allocation to succeed. Set to DEF_NULL to skip calculation.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   LIB_MEM_ERR_SEG_OVF     Allocation would overflow memory segment.
*
* Return(s)   : Pointer to allocated memory block, if successful.
*
*               DEF_NULL, otherwise.
*
* Caller(s)   : Mem_PoolCreate(),
*               Mem_SegAllocInternal(),
*               Mem_SegAllocTrackCritical().
*
* Note(s)     : (1) This function MUST be called within a CRITICAL_SECTION.
*********************************************************************************************************
*/

static  void  *Mem_SegAllocExtCritical (MEM_SEG     *p_seg,
                                        CPU_SIZE_T   size,
                                        CPU_SIZE_T   align,
                                        CPU_SIZE_T   padding_align,
                                        CPU_SIZE_T  *p_bytes_reqd,
                                        LIB_ERR     *p_err)
{
    CPU_ADDR    blk_addr;
    CPU_ADDR    addr_next;
    CPU_SIZE_T  size_rem_seg;
    CPU_SIZE_T  size_tot_blk;
    CPU_SIZE_T  blk_align = DEF_MAX(align, padding_align);


    blk_addr     = MATH_ROUND_INC_UP_PWR2(p_seg->AddrNext,      /* Compute align'ed blk addr.                           */
                                          blk_align);
    addr_next    = MATH_ROUND_INC_UP_PWR2(blk_addr + size,      /* Compute addr of next alloc.                          */
                                          padding_align);
    size_rem_seg = (p_seg->AddrEnd - p_seg->AddrNext) + 1u;
    size_tot_blk =  addr_next      - p_seg->AddrNext;           /* Compute tot blk size including align and padding.    */
    if (size_rem_seg < size_tot_blk) {                          /* If seg doesn't have enough space ...                 */
        if (p_bytes_reqd != DEF_NULL) {                         /* ... calc nbr of req'd bytes.                         */
           *p_bytes_reqd = size_tot_blk - size_rem_seg;
        }

       *p_err = LIB_MEM_ERR_SEG_OVF;
        return (DEF_NULL);
    }

    p_seg->AddrNext = addr_next;

   *p_err = LIB_MEM_ERR_NONE;

    return ((void *)blk_addr);
}


/*
*********************************************************************************************************
*                                     Mem_SegAllocTrackCritical()
*
* Description : Tracks segment allocation, adding the 'size' of the allocation under the 'p_name' entry.
*
* Argument(s) : p_name  Pointer to the name of the object. This string is not copied and its memory should
*                       remain accessible at all times.
*
*               p_seg   Pointer to segment data.
*
*               size    Allocation size, in bytes.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           LIB_MEM_ERR_HEAP_EMPTY      No more memory available on heap
*
*                           --------------RETURNED BY Mem_SegAllocExtCritical()---------------
*                           LIB_MEM_ERR_SEG_OVF         Allocation would overflow memory segment.
*
* Return(s)   : none.
*
* Caller(s)   : Mem_PoolCreate(),
*               Mem_SegAllocInternal().
*
* Note(s)     : none.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
static  void  Mem_SegAllocTrackCritical (const  CPU_CHAR    *p_name,
                                                MEM_SEG     *p_seg,
                                                CPU_SIZE_T   size,
                                                LIB_ERR     *p_err)
{
    MEM_ALLOC_INFO  *p_alloc;


                                                                /* ------- UPDATE ALLOC INFO LIST, IF POSSIBLE -------- */
    p_alloc = p_seg->AllocInfoHeadPtr;
    while (p_alloc != DEF_NULL) {
        if (p_alloc->NamePtr == p_name) {
            p_alloc->Size += size;
           *p_err = LIB_MEM_ERR_NONE;
            return;
        }

        p_alloc = p_alloc->NextPtr;
    }

                                                                /* --------- ADD NEW ALLOC INFO ENTRY IN LIST --------- */
    p_alloc = (MEM_ALLOC_INFO *)Mem_SegAllocExtCritical(&Mem_SegHeap,             /* Alloc new alloc info struct on heap.                 */
                                                         sizeof(MEM_ALLOC_INFO),
                                                         sizeof(CPU_ALIGN),
                                                         LIB_MEM_PADDING_ALIGN_NONE,
                                                         DEF_NULL,
                                                         p_err);
    if (*p_err != LIB_MEM_ERR_NONE) {
        return;
    }

    p_alloc->NamePtr = p_name;                                  /* Populate alloc info.                                 */
    p_alloc->Size    = size;

    p_alloc->NextPtr        = p_seg->AllocInfoHeadPtr;          /* Prepend new item in list.                            */
    p_seg->AllocInfoHeadPtr = p_alloc;
}
#endif


/*
*********************************************************************************************************
*                                     Mem_DynPoolCreateInternal()
*
* Description : Creates a dynamic memory pool.
*
* Argument(s) : p_name              Pointer to pool name.
*
*               p_pool              Pointer to pool data.
*
*               p_seg               Pointer to segment from which to allocate memory.
*
*               blk_size            Size of memory block to allocate from pool, in bytes. See Note #1.
*
*               blk_align           Required alignment of memory block, in bytes. MUST be a power of 2.
*
*               blk_padding_align   Block's padding alignment, in bytes, that will be added at the end
*                                   of block's buffer. MUST be a power of 2. LIB_MEM_PADDING_ALIGN_NONE
*                                   means no padding.
*
*               blk_qty_init        Initial number of elements to be allocated in pool.
*
*               blk_qty_max         Maximum number of elements that can be allocated from this pool. Set to
*                                   LIB_MEM_BLK_QTY_UNLIMITED if no limit.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   LIB_MEM_ERR_INVALID_BLK_ALIGN   Invalid requested block alignment.
*                                   LIB_MEM_ERR_INVALID_BLK_SIZE    Invalid requested block size.
*                                   LIB_MEM_ERR_INVALID_BLK_NBR     Invalid requested block quantity max.
*                                   LIB_MEM_ERR_NULL_PTR            Pool data pointer NULL.
*
*                                   ------------------RETURNED BY Mem_SegAllocInternal()-------------------
*                                   LIB_MEM_ERR_INVALID_MEM_ALIGN   Invalid memory block alignment requested.
*                                   LIB_MEM_ERR_INVALID_MEM_SIZE    Invalid memory block size specified.
*                                   LIB_MEM_ERR_NULL_PTR            Error or segment data pointer NULL.
*                                   LIB_MEM_ERR_SEG_OVF             Allocation would overflow memory segment.
*
* Return(s)   : None.
*
* Caller(s)   : Mem_DynPoolCreate(),
*               Mem_DynPoolCreateHW().
*
* Note(s)     : (1) 'blk_size' must be big enough to fit a pointer since the pointer to the next free
*                   block is stored in the block itself (only when free/unused).
*********************************************************************************************************
*/

static  void  Mem_DynPoolCreateInternal (const  CPU_CHAR      *p_name,
                                                MEM_DYN_POOL  *p_pool,
                                                MEM_SEG       *p_seg,
                                                CPU_SIZE_T     blk_size,
                                                CPU_SIZE_T     blk_align,
                                                CPU_SIZE_T     blk_padding_align,
                                                CPU_SIZE_T     blk_qty_init,
                                                CPU_SIZE_T     blk_qty_max,
                                                LIB_ERR       *p_err)
{
    CPU_INT08U  *p_blks          = DEF_NULL;
    CPU_SIZE_T   blk_size_align;
    CPU_SIZE_T   blk_align_worst = DEF_MAX(blk_align, blk_padding_align);


#if (LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)
    if (p_err == DEF_NULL) {                                    /* Chk for NULL err ptr.                                */
        CPU_SW_EXCEPTION(;);
    }

    if (p_pool == DEF_NULL) {                                   /* Chk for NULL pool data ptr.                          */
       *p_err = LIB_MEM_ERR_NULL_PTR;
        return;
    }

    if (blk_size < 1u) {                                        /* Chk for inv blk size.                                */
       *p_err = LIB_MEM_ERR_INVALID_BLK_SIZE;
        return;
    }

    if ((blk_qty_max  != LIB_MEM_BLK_QTY_UNLIMITED) &&          /* Chk for invalid blk qty.                             */
        (blk_qty_init >  blk_qty_max)) {
       *p_err = LIB_MEM_ERR_INVALID_BLK_NBR;
        return;
    }

    if (MATH_IS_PWR2(blk_align) != DEF_YES) {                   /* Chk for illegal align spec.                          */
       *p_err = LIB_MEM_ERR_INVALID_BLK_ALIGN;
        return;
    }
#endif

                                                                /* Calc blk size with align.                            */
    if (blk_size < sizeof(void *)) {                            /* If size if smaller than ptr ...                      */
                                                                /* ... inc size to ptr size.                            */
        blk_size_align = MATH_ROUND_INC_UP_PWR2(sizeof(void *), blk_align_worst);
    } else {
        blk_size_align = MATH_ROUND_INC_UP_PWR2(blk_size, blk_align_worst);
    }

    if (blk_qty_init != 0u) {                                   /* Alloc init blks.                                     */
        CPU_SIZE_T  i;
        p_blks = (CPU_INT08U *)Mem_SegAllocInternal(p_name,
                                                    p_seg,
                                                    blk_size_align * blk_qty_init,
                                                    DEF_MAX(blk_align, sizeof(void *)),
                                                    LIB_MEM_PADDING_ALIGN_NONE,
                                                    DEF_NULL,
                                                    p_err);
        if (*p_err != LIB_MEM_ERR_NONE) {
            return;
        }

                                                                /* ----------------- CREATE POOL DATA ----------------- */
                                                                /* Init free list.                                      */
        p_pool->BlkFreePtr = (void *)p_blks;
        for (i = 0u; i < blk_qty_init - 1u; i++) {
           *((void **)p_blks)  = p_blks + blk_size_align;
            p_blks            += blk_size_align;
        }
       *((void **)p_blks) = DEF_NULL;
    } else {
        p_pool->BlkFreePtr = DEF_NULL;
    }

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
    p_pool->PoolSegPtr      = ((p_seg != DEF_NULL) ? p_seg : &Mem_SegHeap);
#else
    p_pool->PoolSegPtr      =   p_seg;
#endif
    p_pool->BlkSize         =   blk_size;
    p_pool->BlkAlign        =   blk_align_worst;
    p_pool->BlkPaddingAlign =   blk_padding_align;
    p_pool->BlkQtyMax       =   blk_qty_max;
    p_pool->BlkAllocCnt     =   0u;

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
    p_pool->NamePtr = p_name;
#endif

   *p_err = LIB_MEM_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      Mem_PoolBlkIsValidAddr()
*
* Description : Calculates if a given memory block address is valid for the memory pool.
*
* Argument(s) : p_pool   Pointer to memory pool structure to validate memory block address.
*               ------   Argument validated by caller.
*
*               p_mem    Pointer to memory block address to validate.
*               -----    Argument validated by caller.
*
* Return(s)   : DEF_YES, if valid memory pool block address.
*
*               DEF_NO,  otherwise.
*
* Caller(s)   : Mem_PoolBlkFree().
*
* Note(s)     : (1) This function is DEPRECATED and will be removed in a future version of this product.
*********************************************************************************************************
*/

#if ((LIB_MEM_CFG_ARG_CHK_EXT_EN == DEF_ENABLED) && \
     (LIB_MEM_CFG_HEAP_SIZE      >  0u))
static  CPU_BOOLEAN  Mem_PoolBlkIsValidAddr (MEM_POOL  *p_pool,
                                             void      *p_mem)
{
    CPU_ADDR  pool_offset;


    if ((p_mem < p_pool->PoolAddrStart) ||
        (p_mem > p_pool->PoolAddrEnd)) {
        return (DEF_FALSE);
    }

    pool_offset = (CPU_ADDR)p_mem - (CPU_ADDR)p_pool->PoolAddrStart;
    if (pool_offset % p_pool->BlkSize != 0u) {
        return (DEF_FALSE);
    } else {
        return (DEF_TRUE);
    }
}
#endif

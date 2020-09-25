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
*                                           CACHE CPU MODULE
*
* Filename : cpu_cache.h
* Version  : v1.32.00
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) This cache CPU header file is protected from multiple pre-processor inclusion through use of
*               the  cache CPU module present pre-processor macro definition.
*********************************************************************************************************
*/

#ifndef  CPU_CACHE_MODULE_PRESENT                               /* See Note #1.                                         */
#define  CPU_CACHE_MODULE_PRESENT


/*
*********************************************************************************************************
*                                               EXTERNS
*********************************************************************************************************
*/

#ifdef   CPU_CACHE_MODULE
#define  CPU_CACHE_EXT
#else
#define  CPU_CACHE_EXT  extern
#endif


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <lib_def.h>
#include  <cpu_cfg.h>


/*
*********************************************************************************************************
*                                         CACHE CONFIGURATION
*********************************************************************************************************
*/

#ifndef CPU_CFG_CACHE_MGMT_EN
#define CPU_CFG_CACHE_MGMT_EN  DEF_DISABLED
#endif


/*
*********************************************************************************************************
*                                      CACHE OPERATIONS DEFINES
*********************************************************************************************************
*/

#if (CPU_CFG_CACHE_MGMT_EN == DEF_ENABLED)
#ifndef  CPU_DCACHE_RANGE_FLUSH
#define  CPU_DCACHE_RANGE_FLUSH(addr_start, len)  CPU_DCache_RangeFlush(addr_start, len)
#endif /* CPU_DCACHE_RANGE_FLUSH */
#else
#define  CPU_DCACHE_RANGE_FLUSH(addr_start, len)
#endif /* CPU_CFG_CACHE_MGMT_EN) */


#if (CPU_CFG_CACHE_MGMT_EN == DEF_ENABLED)
#ifndef  CPU_DCACHE_RANGE_INV
#define  CPU_DCACHE_RANGE_INV(addr_start, len)  CPU_DCache_RangeInv(addr_start, len)
#endif /* CPU_DCACHE_RANGE_INV */
#else
#define  CPU_DCACHE_RANGE_INV(addr_start, len)
#endif /* CPU_CFG_CACHE_MGMT_EN) */


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

#if (CPU_CFG_CACHE_MGMT_EN == DEF_ENABLED)

#ifdef __cplusplus
extern  "C" {
#endif

void  CPU_Cache_Init       (void);

void  CPU_DCache_RangeFlush(void      *addr_start,
                            CPU_ADDR   len);

void  CPU_DCache_RangeInv  (void      *addr_start,
                            CPU_ADDR   len);

#ifdef __cplusplus
}
#endif

#endif /* CPU_CFG_CACHE_MGMT_EN */


/*
*********************************************************************************************************
*                                             MODULE END
*
* Note(s) : (1) See 'cpu_core.h  MODULE'.
*********************************************************************************************************
*/

#endif                                                          /* End of CPU core module include.                      */

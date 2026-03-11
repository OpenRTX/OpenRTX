// 模块
#include "ic_allocator.h"

// 包
#include "ic_common.h"

// 平台BSP
#include "cache.h"

// 其他
#include "venus_log.h"

// rtos
#include "lsf_os_port_internal.h"

// gcc specific
#include <cmsis_gcc.h>

// 标准库
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <math.h>

// mpoolq定制-------------------------------------
#define BOOL char
#define TRUE 1
#define FALSE 0
//#define ASSERT_MSG( cond, msg ) IC_ASSERT(cond, msg)

#define MACROS_TO_INLINES

#define MPOOLQ_T IC_Allocator_Pool
typedef int MPOOLQ_CELL_T;
#include "fifo/mpoolq.h"

// allocator heap buf------------------------------------------------
// #define POOL_SIZE 32bytes*N = 32/sizeof(MPOOLQ_CELL_T)个cells*N
// cp侧分配buf, 以0x200c0000的64k ram 作为共享内存
// cacheable. __attribute__((section (".dram0.data"), aligned(IC_MAX_DCACHE_LINESIZE)))
// MPOOLQ_CELL_T poolbuf [MPOOLQ_UU_TO_CELLS (POOL_SIZE) ];

// 28 = 32 - 4 bytes
#define IC_ALLOCATOR_HEADER_GAP (IC_DCACHELINE_ROUNDUP_SIZE(sizeof(MPOOLQ_CELL_T))-sizeof(MPOOLQ_CELL_T))

//------------------------------------------------
IC_Allocator *IC_Allocator_ctor(void *addr, IC_Allocator_Pool *pool, int mode, int apAddrOffset)
{
    IC_Allocator *self = (IC_Allocator *) addr;

    // cp侧初始化buf

    self->apAddrOffset = apAddrOffset;
    self->mode = mode;
    self->pool = pool;

    CLOGD("[%s] IC_Allocator_Pool @ %p : size: %d cells. head %p. tail %p.",
          __FUNCTION__,
          pool, pool->size, pool->head.r, pool->tail.r);

    if (IC_ALLOCATOR_MODE_FREE == mode) {
        // 本端先放弃所有数据帧所有权, 后续使用或free时, 直接读取就是最新值
        unsigned long base = (unsigned long) (pool->base) + self->apAddrOffset;
        // pool->size 记录 CELL数量
        dcache_invalidate_range(base,
                                base + (pool->size * sizeof(MPOOLQ_CELL_T)));

        /* Attempt to create a mutex type semaphore. */
        self->freeMutex = lsf_sem_create_mutex();
        IC_ASSERT(NULL != self->freeMutex);
    }
    else if (IC_ALLOCATOR_MODE_ALLOC == mode) {
        /* Attempt to create a mutex type semaphore. */
        self->allocMutex = lsf_sem_create_mutex();
        IC_ASSERT(NULL != self->allocMutex);
    }

    // trace
    // mpoolq_dump (self->pool);

    return self;
}

// 暂时用不到, 非正式实现
int IC_Allocator_dtor(IC_Allocator *self)
{
	(void) self;

    return IC_OK;
}

// 返回值: 返回ptr: cache line aligned
void *IC_Allocator_alloc(IC_Allocator *self, size_t user_size)
{
    BaseType_t ret;

    // user_size 向上取整
    size_t aligned_size = IC_DCACHELINE_ROUNDUP_SIZE(user_size);
    size_t alloc_size = IC_ALLOCATOR_HEADER_GAP + aligned_size;

    // Lock: 多个alloc互斥
    ret = lsf_sem_take(self->allocMutex, lsf_max_delay);
    IC_ASSERT(lsf_task_pass == ret);

    char *ptr = (char *) mpoolq_alloc(self->pool, alloc_size);

    // 如分配成功, 则调整指针为cache line aligned供写入
    if (NULL != ptr) {
        ptr = (char *)ptr + IC_ALLOCATOR_HEADER_GAP;
    }
    // else: 分配不成功, 则不调整, NULL传出

    // UnLock: 多个alloc互斥
    ret = lsf_sem_give(self->allocMutex);
    IC_ASSERT(lsf_task_pass == ret);

    return ptr;
}

// @param ptr       : cache line aligned address
int IC_Allocator_free(IC_Allocator *self, void *ptr)
{
    BaseType_t ret;

    // check args
    IC_ASSERT((((unsigned long) ptr) % IC_MAX_DCACHE_LINESIZE) == 0);

    // Lock: 多个free互斥
    ret = lsf_sem_take(self->freeMutex, lsf_max_delay);
    IC_ASSERT(lsf_task_pass == ret);

    // 将其调整为期望的紧跟header的ptr: 后退28bytes
    char *realPtr = (char *)(((uint32_t) ptr) - IC_ALLOCATOR_HEADER_GAP);

    mpoolq_free(self->pool, realPtr);

    // UnLock: 多个free互斥
    ret = lsf_sem_give(self->freeMutex);
    IC_ASSERT(lsf_task_pass == ret);

    return 0;
}

//------------------------------------------------
//------------------------------------------------
// Sharemem辅助功能--------------------------------


// 将一段内存刷出cache, 并从本端cache脱离
// @param ptr       : cache line aligned address
// @param user_size : bytes, 允许非cache line aligned
int IC_Sharemem_updateAndDetach(void *ptr, size_t user_size)
{
    IC_ASSERT((((unsigned long) ptr) % IC_MAX_DCACHE_LINESIZE) == 0);

    // 之前的数据写入 ->
    // synchronization & memory barrier. 期望: store
    __DSB();

    // write-back用户数据 => 共享内存
    dcache_clean_range((unsigned long) ptr,
                       ((unsigned long) ptr) + IC_DCACHELINE_ROUNDUP_SIZE(user_size));
    // synchronization & memory barrier. 期望: full system and store
    __DSB();
    // alloc端脱钩用户数据
    dcache_invalidate_range((unsigned long) ptr,
                            ((unsigned long) ptr) + IC_DCACHELINE_ROUNDUP_SIZE(user_size));
    // synchronization & memory barrier. 期望: full system
    __DSB();
    // -> 之后的数据发出

    return 0;
}

// 将一段内存从本端cache脱离, without write back
// @param ptr       : cache line aligned address
// @param user_size : bytes, 允许非cache line aligned
int IC_Sharemem_detach(void *ptr, size_t user_size)
{
    IC_ASSERT((((unsigned long) ptr) % IC_MAX_DCACHE_LINESIZE) == 0);

    // 之前的数据写入 ->
    // synchronization & memory barrier. 期望: store
    __DSB();

    // free端脱钩用户数据
    dcache_invalidate_range((unsigned long) ptr,
                            ((unsigned long) ptr) + IC_DCACHELINE_ROUNDUP_SIZE(user_size));
    // synchronization & memory barrier. 期望: full system
    __DSB();
    // -> 之后的控制权交出

    return 0;
}

// 模块
#include "ic_allocator.h"

// 包
#include "ic_common.h"

// 其他
#include "venus_log.h"

// 平台BSP
#include "cache.h"

// rtos
#include <xtensa/hal.h>
#include <xtensa/config/core.h>
#include "xtensa/xos.h"

// 标准库
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// mpoolq定制-------------------------------------
#define BOOL char
#define TRUE 1
#define FALSE 0
//#define ASSERT_MSG( cond, msg ) IC_ASSERT(cond, msg)

#define MACROS_TO_INLINES

#define MPOOLQ_T IC_Allocator_Pool
typedef int MPOOLQ_CELL_T;
#include "fifo/mpoolq.h"

// config ------------------------------------------------
// heap由上层给入
// cp侧分配buf, 以0x200c0000的64k ram 作为共享内存
// #define POOL_SIZE 32bytes*N = 32/sizeof(MPOOLQ_CELL_T)个cells*N
// __attribute__((section (".share_sram.data"), aligned(IC_MAX_DCACHE_LINESIZE)))
// MPOOLQ_CELL_T poolbuf [MPOOLQ_UU_TO_CELLS (POOL_SIZE) ];

// 28 = 32 - 4 bytes
#define IC_ALLOCATOR_HEADER_GAP (IC_DCACHELINE_ROUNDUP_SIZE(sizeof(MPOOLQ_CELL_T))-sizeof(MPOOLQ_CELL_T))

// allocator share part: mpool obj buf------------------------------------------
#define IC_ALLOCATOR_OBJ_NUM 2

__attribute__((section (".share_sram.data"), aligned(IC_MAX_DCACHE_LINESIZE)))
static unsigned char _IC_Allocator_PoolObjs[IC_ALLOCATOR_OBJ_NUM][IC_DCACHELINE_ROUNDUP_SIZE(sizeof(IC_Allocator_Pool))];

// TBD: 改为从共享内存的对象池alloc(sizeof(obj))
static IC_Allocator_Pool* IC_Allocator_allocPool(int objID)
{
    IC_Allocator_Pool *result = NULL;

    if (objID < IC_ALLOCATOR_OBJ_NUM) {
        result = (IC_Allocator_Pool *) _IC_Allocator_PoolObjs[objID];
    }

    return result;
}

// 类静态成员 ------------------------------------------------
static bool _IC_Allocator_Class_inited = false;

// 类全体对象跟踪, 用于远程会话的管理, 会话ID用于索引对象
static IC_Allocator *_IC_Allocator_objs[IC_ALLOCATOR_OBJ_NUM];

static int
_IC_Allocator_Class_init(void)
{
    // 已经初始化过, 则忽略后续
    if (_IC_Allocator_Class_inited) return 0;

    _IC_Allocator_Class_inited = true;

    // TBD: 类全体对象跟踪, 用于远程会话的管理, 若抽象为ICObj Interface, 可放在ic_proxy统一记录.
    for (int i = 0; i < IC_ALLOCATOR_OBJ_NUM; ++i) {
        _IC_Allocator_objs[i] = NULL;
    }

    return 0;
}

IC_Allocator * IC_Allocator_Class_getObj(int objID, size_t *shareSize)
{
    *shareSize = sizeof(struct IC_Allocator_Pool);

    if (objID < IC_ALLOCATOR_OBJ_NUM) {
        return _IC_Allocator_objs[objID];
    }

    return NULL;
}

//------------------------------------------------
IC_Allocator *IC_Allocator_ctor(void *addr, int objID, void *poolBuf, int poolBufSize,
                                int mode, int apAddrOffset)
{
    int32_t  ret;

    // 先类初始化
    _IC_Allocator_Class_init();

    // 对象初始化
    IC_Allocator *self = (IC_Allocator *) addr;

    // cp侧初始化buf, 共享对象预分配在共享内存
    IC_Allocator_Pool *pool = IC_Allocator_allocPool(objID);
    if (pool == NULL) return NULL;

    MPOOLQ_INIT (pool, poolBuf, poolBufSize/sizeof(MPOOLQ_CELL_T));

    self->apAddrOffset = apAddrOffset;
    self->mode = mode;
    self->pool = pool;

    CLOGD("[%s] IC_Allocator_Pool @ %p : size: %d cells. head %p. tail %p.",
          __FUNCTION__,
          pool, pool->size, pool->head.r, pool->tail.r);

    if (IC_ALLOCATOR_MODE_FREE == mode) {
        // 本端先放弃pool buf所有权, 后续使用或free时, 直接读取就是最新值
        // pool->size 记录 CELL数量
        xthal_dcache_region_invalidate((void *) pool->base, pool->size*sizeof(MPOOLQ_CELL_T));

        /* Attempt to create a mutex. */
        ret = xos_mutex_create(& self->freeMutex, XOS_MUTEX_WAIT_PRIORITY, 0);
        IC_ASSERT(ret == XOS_OK);

    }
    else if (IC_ALLOCATOR_MODE_ALLOC == mode) {
        /* Attempt to create a mutex. */
        ret = xos_mutex_create(& self->allocMutex, XOS_MUTEX_WAIT_PRIORITY, 0);
        IC_ASSERT(ret == XOS_OK);
    }

    // trace
    // mpoolq_dump (self->pool);

    // 加入远程会话跟踪
    _IC_Allocator_objs[objID] = self;

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
    int32_t ret;

    // user_size 向上取整
    size_t aligned_size = IC_DCACHELINE_ROUNDUP_SIZE(user_size);
    size_t alloc_size = IC_ALLOCATOR_HEADER_GAP + aligned_size;

    // Lock: 多个alloc互斥
    ret = xos_mutex_lock(& self->allocMutex);
    IC_ASSERT(XOS_OK == ret);

    char *ptr = (char *) mpoolq_alloc(self->pool, alloc_size);

    // 如分配成功, 则调整指针为cache line aligned供写入
    if (NULL != ptr) {
        ptr = (char *)ptr + IC_ALLOCATOR_HEADER_GAP;
    }
    // else: 分配不成功, 则不调整, NULL传出

    // UnLock: 多个alloc互斥
    ret = xos_mutex_unlock(& self->allocMutex);
    IC_ASSERT(XOS_OK == ret);

    return ptr;
}

// @param ptr       : cache line aligned address
int IC_Allocator_free(IC_Allocator *self, void *ptr)
{
    int32_t ret;

    // check args
    IC_ASSERT((((unsigned long) ptr) % IC_MAX_DCACHE_LINESIZE) == 0);

    // Lock: 多个free互斥
    ret = xos_mutex_lock(& self->freeMutex);
    IC_ASSERT(XOS_OK == ret);

    // 将其调整为期望的紧跟header的ptr: 后退28bytes
    char *realPtr = (char *)(((uint32_t) ptr) - IC_ALLOCATOR_HEADER_GAP);

    mpoolq_free(self->pool, realPtr);

    // UnLock: 多个free互斥
    ret = xos_mutex_unlock(& self->freeMutex);
    IC_ASSERT(XOS_OK == ret);

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
#pragma flush_memory

    // write-back用户数据 => 共享内存
    xthal_dcache_region_writeback(ptr, IC_DCACHELINE_ROUNDUP_SIZE(user_size));

    // synchronization & memory barrier. 期望: full system and store
#pragma flush_memory
    // alloc端脱钩用户数据
    xthal_dcache_region_invalidate(ptr, IC_DCACHELINE_ROUNDUP_SIZE(user_size));

    // synchronization & memory barrier. 期望: full system
#pragma flush_memory
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
#pragma flush_memory

    // free端脱钩用户数据
    xthal_dcache_region_invalidate(ptr, IC_DCACHELINE_ROUNDUP_SIZE(user_size));

    // synchronization & memory barrier. 期望: full system
#pragma flush_memory
    // -> 之后的控制权交出

    return 0;
}

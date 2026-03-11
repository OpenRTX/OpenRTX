#ifndef __IC_ALLOCATOR_H__
#define __IC_ALLOCATOR_H__

#ifdef __cplusplus
extern "C" {
#endif

// 包
#include "ic_common.h"

// 平台
#include "lsf_os_port_internal.h"

// 标准库
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

//------------------------------------------------------------

enum {
    IC_ALLOCATOR_MODE_ALLOC,
    IC_ALLOCATOR_MODE_FREE,
};

typedef struct _tag_IC_Allocator_Config
{
    int mode; // 0:alloc; 1:free
} IC_Allocator_Config;

typedef struct IC_Allocator_Pool IC_Allocator_Pool;

typedef struct _IC_Allocator IC_Allocator;

struct _IC_Allocator {
    // IC_ALLOCATOR_MODE_ALLOC/IC_ALLOCATOR_MODE_FREE
    int mode;

    int apAddrOffset;

    // 共享部分
    IC_Allocator_Pool *pool;

    union {
        // 单核alloc
        lsf_sem_t allocMutex;

        // 单核free
        lsf_sem_t freeMutex;
    };
};

// 类静态函数, 在反序列化时构造对象.
// IC_Allocator *IC_AllocatorCreator_createObj(IC_Allocator *addr, int objID, IC_Allocator_Pool *pool, int mode);

IC_Allocator *IC_Allocator_ctor(void *addr, IC_Allocator_Pool *pool, int mode, int apAddrOffset);

int IC_Allocator_dtor(IC_Allocator *self);

// alloc------------------------------------------------------------

// 返回值: 返回ptr   : cache line aligned
void *IC_Allocator_alloc(IC_Allocator *self, size_t user_size);

// @param ptr       : cache line aligned address
int IC_Allocator_free(IC_Allocator *self, void *ptr);

// helper函数 ------------------------------------------------------------

// 将一段内存刷出cache, 并从本端cache脱离
// @param ptr       : cache line aligned address
// @param user_size : bytes, 允许非cache line aligned
int IC_Sharemem_updateAndDetach(void *ptr, size_t user_size);

// 将一段内存从本端cache脱离, without write back
// @param ptr       : cache line aligned address
// @param user_size : bytes, 允许非cache line aligned
int IC_Sharemem_detach(void *ptr, size_t user_size);

#ifdef __cplusplus
}
#endif

#endif /* __IC_ALLOCATOR_H__ */

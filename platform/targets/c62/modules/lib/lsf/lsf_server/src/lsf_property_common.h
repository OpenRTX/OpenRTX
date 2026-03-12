#ifndef __LSF_PROPERTY_COMMON_H__
#define __LSF_PROPERTY_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "variant.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// 核间属性 flag ------------------------------------------------

// 同步模式 - bit [0]
#define LSF_PROPERTY_FLAG_SYNCMODE_POS          (0U)
#define LSF_PROPERTY_FLAG_SYNCMODE_MSK          (1U << LSF_PROPERTY_FLAG_SYNCMODE_POS)
// 同步get/set
#define LSF_PROPERTY_FLAG_SYNC                  (0U << LSF_PROPERTY_FLAG_SYNCMODE_POS)
// 异步get/set
#define LSF_PROPERTY_FLAG_ASYNC                 (1U << LSF_PROPERTY_FLAG_SYNCMODE_POS)

// set/get 读写模式 - bit [1-2]
#define LSF_PROPERTY_FLAG_RWMODE_POS            (1U)
#define LSF_PROPERTY_FLAG_RWMODE_MSK            (3U << LSF_PROPERTY_FLAG_RWMODE_POS)
// set only
#define LSF_PROPERTY_FLAG_SET_ONLY              (0U << LSF_PROPERTY_FLAG_RWMODE_POS)
// set/get
#define LSF_PROPERTY_FLAG_SET_GET               (1U << LSF_PROPERTY_FLAG_RWMODE_POS)
// get only
#define LSF_PROPERTY_FLAG_GET_ONLY              (2U << LSF_PROPERTY_FLAG_RWMODE_POS)
// notify only
#define LSF_PROPERTY_FLAG_NOTIFY                (3U << LSF_PROPERTY_FLAG_RWMODE_POS)

// // bit [2]
// #define LSF_PROPERTY_FLAG_NO_NOTIFY  (0UL << 2)          ///< 不支持cp通知
// #define LSF_PROPERTY_FLAG_NOTIFY     (1UL << 2)          ///< 支持cp通知

// 核间属性 type ------------------------------------------------

#define LSF_PROPERTY_TYPE_UINT32     (0UL)          ///< 整型值
#define LSF_PROPERTY_TYPE_BUF        (1UL)          ///< 内存块
#define LSF_PROPERTY_TYPE_REF        (2UL)          ///< 对象引用
#define LSF_PROPERTY_TYPE_PTR        (3UL)          ///< 指针

//------------------------------------------------

// onSet方法
typedef int (*LsfProperty_OnSetFunc)(int group_id, int property_id, LsfVariant *new_var);

// onGet方法
typedef int (*LsfProperty_OnGetFunc)(int group_id, int property_id, LsfVariant *new_var);

typedef enum {
    // 未设置, 初始时
    LSF_PROPERTY_CONTEXT_INVALID = 0,

    // 运行上下文在task中
    LSF_PROPERTY_CONTEXT_TASK,

    // 没有指定运行上下文
    LSF_PROPERTY_CONTEXT_NONE,
} LSF_PROPERTY_CONTEXT_TYPE;

// 核间属性 Config------------------------------------------------
typedef struct tagLsfPropertyConfig {
    // 顺序递增的ID
    int property_id;

    // 核间属性的flag
    uint32_t flag;

    // 类型: 值(整型值, 内存块), 对象引用, TODO: 上下文(task, 中断)?
    int type;
} LsfPropertyConfig;

enum {
    // 核间属性 专属消息
    LSF_MSG_PROPERTY_BASE = 1000,

    LSF_MSG_PROPERTY_DISPATCH,
};

typedef struct tagLsfMsg {
    uint32_t id; // 消息标识
    uint32_t param; // 消息参数
} LsfMsg;

#ifdef __cplusplus
}
#endif

#endif /* __LSF_PROPERTY_COMMON_H__ */

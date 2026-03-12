#ifndef __AUDIOFLINGER_PROPERTY_DEF_H__
#define __AUDIOFLINGER_PROPERTY_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lsf_property_common.h"

// 核间内存管理
#include "ic_allocator.h"
extern IC_Allocator *g_pFreeer;

#define PROPERTY_GROUP_ID_AUDIOFLINGER  (0)

// 本模块 核间属性ID. AP/CP对等.
enum {
    ISERVICEMANAGER_PROPERTY,

    IAUDIOFLINGER_PROPERTY,

    IAUDIOTRACK_PROPERTY,

    IAUDIORECORD_PROPERTY,

    IAUDIOPOLICYSERVICE_PROPERTY,
};

//TODO: 被多次include, 副作用?

// 本模块 核间属性 声明列表. 必须AP/CP对等.
// 核间属性对象随着模块的运行动态激活.
static LsfPropertyConfig audioflinger_property_defs[] = {
    {
        .property_id = ISERVICEMANAGER_PROPERTY,
        .flag = LSF_PROPERTY_FLAG_SYNC | LSF_PROPERTY_FLAG_SET_ONLY,
        .type = LSF_PROPERTY_TYPE_BUF,
    },
    {
        .property_id = IAUDIOFLINGER_PROPERTY,
        .flag = LSF_PROPERTY_FLAG_SYNC | LSF_PROPERTY_FLAG_SET_ONLY,
        .type = LSF_PROPERTY_TYPE_BUF,
    },
    {
        .property_id = IAUDIOTRACK_PROPERTY,
        .flag = LSF_PROPERTY_FLAG_SYNC | LSF_PROPERTY_FLAG_SET_ONLY,
        .type = LSF_PROPERTY_TYPE_BUF,
    },
    {
        .property_id = IAUDIORECORD_PROPERTY,
        .flag = LSF_PROPERTY_FLAG_SYNC | LSF_PROPERTY_FLAG_SET_ONLY,
        .type = LSF_PROPERTY_TYPE_BUF,
    },
    {
        .property_id = IAUDIOPOLICYSERVICE_PROPERTY,
        .flag = LSF_PROPERTY_FLAG_SYNC | LSF_PROPERTY_FLAG_SET_ONLY,
        .type = LSF_PROPERTY_TYPE_BUF,
    },
};

#ifdef __cplusplus
}
#endif

#endif /* __AUDIOFLINGER_PROPERTY_DEF_H__ */

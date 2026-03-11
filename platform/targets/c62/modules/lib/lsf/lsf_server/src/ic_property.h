#ifndef __LSF_PROPERTY_H__
#define __LSF_PROPERTY_H__

#ifdef __cplusplus
extern "C" {
#endif

// 本单元
#include "lsf_config.h"
#include "lsf_property_common.h"

// 模块内
#include "variant.h"

#include "ic_common.h"

// 其他模块
#include "urpc_api.h"

// 平台
#include <xtensa/config/core.h>
#include <xtensa/xos.h>

// 标准库
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// \------------------------------------------------------------
// 对象实例定义

// 不存在对象的核间共享部分

// 对象的单核私有部分
typedef struct tagLsfProperty {
    // property ID, 对象本身不需要id吧?
    // int property_id;

    // 类型: 值(整型值, 内存块), 对象引用
    int type;

    LsfProperty_OnSetFunc on_set;
    LsfProperty_OnGetFunc on_get;

} LsfProperty;

typedef enum {
    LSF_PROPERTY_FUNC_ID_SET,
    LSF_PROPERTY_FUNC_ID_GET,
    LSF_PROPERTY_FUNC_ID_NOTIFY_REGISTER,
} LSF_PROPERTY_FUNC_ID;

// 核间属性 配置管理项 结构------------------------------------------------
typedef struct tagLsfPropertyItem {
    // 顺序递增的ID
        // TODO 似乎有点冗余了
    int property_id;

    // 核间属性的flag
    uint32_t flag;

    // 运行上下文不应该放在 属性对象中, 应该是runtime的一部分
    // context type
    LSF_PROPERTY_CONTEXT_TYPE context_type;
    XosMsgQueue *context_msgQ;

    // 类型: 值(整型值, 内存块), 对象引用, TODO: 上下文(task, 中断)?
    // 类型信息放在对象中, 初始化管理表时, 对象尚未建立, 所以暂时记录于此.(看起来有些冗余?)
    int type;

    int group_id;

    // 对应的 核间属性 对象
    LsfProperty *property;
} LsfPropertyItem;

// api > 属性 管理 ------------------------------------------------
// 管理表大小可配置: group数

// 属性服务通过专有的rpc响应查询和获取 核间属性 对象.
//     属性服务也处理 核间对象 引用的正确传递与proxy构建
// 属性服务, 包含了 属性配置表.
int LsfPropertyService_init(void);

// int LsfPropertyService_uninit(void);

// 内部数据多线程访问, 加mutex
int LsfPropertyService_addGroup(int group_id, LsfPropertyConfig *config_array, int config_count);

// 找不到则返回出错码, 并*out_property == NULL;
int LsfPropertyService_find(int group_id, int property_id, LsfPropertyItem **out_property_item);

// 动态挂接 核间属性 对象 -> 管理条目
// TODO 单独抽出接口, 不是简单赋值, 可能触发动作, 或者是原子操作?
int LsfPropertyService_attach(LsfPropertyItem *property_item, LsfProperty *property,
                              LSF_PROPERTY_CONTEXT_TYPE context_type, void *context_param);

// api > 投递/执行 ------------------------------------------------

typedef struct tagLsfPropertyParcel {
    // 目标对象
    LsfPropertyItem *property_item;

    // 函数 id
    int func_id;

    // 函数参数: 目前只有单参数: Variant
    LsfVariant var_arg;

    // rpc_frame, async回复用
    urpc_frame frame;
} LsfPropertyParcel;

// 挂接到urpc handler
// int LsfPropertyService_查询();
// int LsfPropertyService_请求();

int LsfPropertyService_dispatch(LsfPropertyItem *property_item, LsfPropertyParcel *parcel);

int LsfPropertyService_execute(LsfPropertyItem *property_item, LsfPropertyParcel *parcel);

// api > LsfProperty ------------------------------------------------

LsfProperty* LsfProperty_new(int type);

LsfProperty* LsfProperty_ctor(LsfProperty *self, int type);

int LsfProperty_dtor(LsfProperty *self);

// 类型定义放在lsf.h似乎不合理, 单向依赖才对啊
//     然后, 命名按谁的呢? LsfPropertyOnSetFunc ; LSF_PROPERTY_CONTEXT_TYPE
//     Y: 又, 这些概念放在公共定义.h? LsfProperty 和 lsf 都依赖于公共定义.h?
//     N: lsf_property_pub.h?
//     N: 干脆两个名字, 互不依赖, 实现时等价转换, 实际就是一样的东西
int LsfProperty_register(LsfProperty *self,
                         LsfProperty_OnSetFunc onset_func,
                         LsfProperty_OnGetFunc onget_func);

int LsfProperty_set(LsfProperty *self, int groupd_id, int property_id, LsfVariant *var);

int LsfProperty_get(LsfProperty *self, int groupd_id, int property_id, LsfVariant *out_var);

// native处理, 在task(约定?)上下文中被调用, onRegisterNotify 复用 onSet()
int LsfProperty_registerNotify(LsfProperty *self, int property_id, LsfVariant *var);

int LsfProperty_notify(LsfProperty *self, int group_id, int property_id, LsfVariant *var);

#ifdef __cplusplus
}
#endif

#endif /* __LSF_PROPERTY_H__ */

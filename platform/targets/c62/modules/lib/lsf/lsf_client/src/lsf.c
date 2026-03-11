// 本模块配置
#include "lsf_config.h"

// 本单元
#include "lsf.h"

// 本模块
#include "ic_common.h"
#include "ic_property.h"
#include "ic_allocator.h"

// 其他模块
#include "ic_proxy.h"
#include "rpc_client.h"

// log模块
#include "venus_log.h"

// 平台
#include "lsf_os_port_internal.h"

// 标准库
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// \------------------------------------------------------------
// 对象实例定义

// 不存在对象的核间共享部分

// 对象的单核私有部分
// struct lsf {
//     //
//     int dummy;
// };

// static lsf s_lsf_module;

// \------------------------------------------------------------

// \------------------------------------------------------------
// 类静态数据和类静态方法
// TODO: 与远程通信相关的操作似乎可以抽出在通信基类?

// 模块初始化计数
static bool _lsf_module_isInited = false;

// \------------------------------------------------------------
IC_Allocator *g_pAllocator;

// id 0, allocator, AP-alloc, CP-free
#define IC_ALLOCATOR_ID_0 0
static IC_Allocator g_allocator;

IC_Allocator *g_pFreeer;

// id 1, free-er
#define IC_ALLOCATOR_ID_1 1
IC_Allocator g_freeer;

// \------------------------------------------------------------
// 实现
// TODO: 所有函数加assert

/**
  \brief       lsf模块初始化
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_init(void)
{
    CLOGD("[api] %s", __FUNCTION__);

    // 已经初始化过, 则忽略后续
    if (_lsf_module_isInited) {
//        *out_handle = & s_lsf_module;
        return 0;
    }
    _lsf_module_isInited = true;

    // rpc初始化, 以便挂接handler, 不启动
    RPC_Client_init();

    // 初始化Proxy, 初始化内置rpc机制, 注册handler到urpc
    IC_Proxy_init();

    // 其中挂接handler
    LsfPropertyService_init();

    return LSF_SUCCESS;
}

/**
  \brief       连接ap/cp
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_connect(void)
{
    CLOGD("[api] %s", __FUNCTION__);

    // rpc启动, 并与server同步
    RPC_Client_Start();

    // 此时server端的IC_Allocator对象已就绪
    g_pAllocator = IC_Proxy_getRemoteICAllocator(& g_allocator, IC_ALLOCATOR_ID_0);
    g_pFreeer = IC_Proxy_getRemoteICAllocator(& g_freeer, IC_ALLOCATOR_ID_1);

    // 建立push通道, 以支持notify
    LsfPropertyService_connect();

    return LSF_SUCCESS;
}

/*
    各service处发布一张局部清单. 模块性更好.
    对ap来说, 各service-client(AP)只关心其使用到的 service核间属性 清单.
*/
int lsf_import_properties(int group_id, LsfPropertyConfig *config_array, int config_count)
{
    CLOGD("[api] %s", __FUNCTION__);

    // 错误值转义后返回

    return LsfPropertyService_addGroup(group_id, config_array, config_count);
}

/*
    向 核间属性 注册 notify回调, 同时指定回调的运行上下文.
    核间属性 激活, 到此, 才真正建立 核间属性 proxy, 才可接收到对端的的notify,
*/
// 多线程场景考虑: 对端notify收到和本端建立proxy
int lsf_register(int group_id, int property_id,
                 LsfProperty_OnSetFunc onnotify_func, LsfProperty_OnGetFunc onget_func,
                 LSF_PROPERTY_CONTEXT_TYPE context_type, void *context_param)
{
    // TODO: 合法性检查
    int ret;

    CLOGD("[api] %s", __FUNCTION__);

    // 查询不到条目则返回出错,

    LsfProperty *property;

    int retry_cnt = 10;
    // 获取 proxy对象
    do {
        // 没找到, 返回出错? 继续查询吧
        ret = LsfPropertyService_getProperty(group_id, property_id, &property,
                                             context_type, context_param);

        if (IC_OK != ret) {
            lsf_task_delay(1000);
            retry_cnt --;
        }
    } while (IC_OK != ret && retry_cnt >= 0);

    if (IC_OK != ret) {
        return ret;
    }

    // 核间属性 配置: 回调方法和运行上下文
    LsfProperty_register(property, group_id, property_id, onnotify_func);

    // TODO 错误处理: 不支持重复register

    return LSF_SUCCESS;
}

/**
  \brief       设置 核间属性, 同步阻塞
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_set(int group_id, int property_id, LsfVariant *var)
{
    // TODO: 合法性检查
    int ret;

    CLOGD("[api] %s", __FUNCTION__);

    // 查询不到条目则返回出错,

    LsfProperty *property;

    // 获取 proxy对象
    do {
        // 没找到, 返回出错? 继续查询吧
        ret = LsfPropertyService_getProperty(group_id, property_id, &property,
                                             0, NULL);

        if (IC_OK != ret) {
            // 重试间隔, 放到config中?
            lsf_task_delay(1000);
        }
    } while (IC_OK != ret);

    // 使用前, (先进行 对象级 连接? 没必要?) 再远程方法调用

    // 找到: set它
    ret = LsfProperty_set(property, group_id, property_id, var);

    return ret;
}

/**
  \brief       读取 核间属性, 同步阻塞
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_get(int group_id, int property_id, LsfVariant **out_var)
{
    // TODO: 合法性检查
    int ret;

    CLOGD("[api] %s", __FUNCTION__);

    // 查询不到条目则返回出错,

    LsfProperty *property;

    // 获取 proxy对象
    do {
        // 没找到, 返回出错? 继续查询吧
        ret = LsfPropertyService_getProperty(group_id, property_id, &property,
                                             0, NULL);

        if (IC_OK != ret) {
            // 重试间隔, 放到config中?
            lsf_task_delay(1000);
        }
    } while (IC_OK != ret);

    // 使用前, (先进行 对象级 连接? 没必要?) 再远程方法调用

    LsfProperty_get(property, group_id, property_id, out_var);

    return LSF_SUCCESS;
}

/**
  \brief       lsf消息处理, 需要嵌入在支持lsf的消息循环中
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_defaultMsgProc(LsfMsg *msg)
{
    CLOGD("[api] %s", __FUNCTION__);

    IC_ASSERT(NULL != msg);

    if (LSF_MSG_PROPERTY_DISPATCH != msg->id) {
        return -1;
    }

    LsfPropertyParcel *parcel = (LsfPropertyParcel *) msg->param;

    // 分发给实际property对象, 不用查表了, msg中已经有对象地址
    LsfPropertyService_execute(parcel->property_item, parcel);

    // 不涉及Variant本体的释放, 只清空其内部间接内存占用: 对端分配的内存, 由本端释放
    LsfVariant_clear(& parcel->var_arg);
    // 处理完free(parcel)
    free(parcel);

    return 0;
}

/**
  \brief       断开连接ap/cp
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_disconnect(void)
{
    return LSF_SUCCESS;
}

/**
  \brief       lsf模块去初始化
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_uninit(void)
{
    return LSF_SUCCESS;
}

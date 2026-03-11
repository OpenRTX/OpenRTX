// 本模块
// 模块配置项
#include "lsf_config.h"
#include "lsf.h"

#include "ic_common.h"
#include "ic_service.h"

#include "ic_property.h"
#include "ic_allocator.h"

// 其他模块
#include "urpc_api.h"
#include "rpc_server.h"
#include "venus_log.h"

// 平台
#include <xtensa/hal.h>
#include <xtensa/config/core.h>
#include "xtensa/xos.h"

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
// cp侧分配buf, 以0x200c0000的64k ram 作为共享内存
// #define POOL_SIZE 32bytes*N = 32/sizeof(MPOOLQ_CELL_T)个cells*N
#define POOL_SIZE (32*100)
__attribute__((section (".share_sram.data"), aligned(32)))
uint8_t poolBuf_0[POOL_SIZE];

IC_Allocator *g_pFreeer;

// id 0, free-er; AP-alloc, CP-free
#define IC_ALLOCATOR_ID_0 0
static IC_Allocator g_freeer;

//------------------------------------------------
// cp侧分配buf, 以0x200c0000的64k ram 作为共享内存
// #define POOL_SIZE 32bytes*N = 32/sizeof(MPOOLQ_CELL_T)个cells*N
// #define POOL_SIZE (32*100)
__attribute__((section (".share_sram.data"), aligned(32)))
uint8_t poolBuf_1[POOL_SIZE];

IC_Allocator *g_pAllocator;

// id 1, allocator; AP-free <== CP-alloc
#define IC_ALLOCATOR_ID_1 1
static IC_Allocator g_allocator;

// \------------------------------------------------------------
// 类静态数据和类静态方法
// TODO: 与远程通信相关的操作似乎可以抽出在通信基类?

// 模块初始化计数
static bool _lsf_module_isInited = false;

// \------------------------------------------------------------

// \------------------------------------------------------------
// 实现
// TODO: 所有函数加assert

/**
  \brief       lsf模块初始化
               重复调用计数
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_init(void)
{
    // 已经初始化过, 则忽略后续
    if (_lsf_module_isInited) {
        return 0;
    }
    _lsf_module_isInited = true;

    // rpc初始化, 以便挂接handler, 不启动
    RPC_Server_init();

    g_pFreeer = IC_Allocator_ctor((void *) & g_freeer,
                                      IC_ALLOCATOR_ID_0,
                                      poolBuf_0,
                                      POOL_SIZE,
                                      IC_ALLOCATOR_MODE_FREE,
                                      0);
    IC_ASSERT(g_pFreeer != NULL);

    g_pAllocator = IC_Allocator_ctor((void *) & g_allocator,
                                      IC_ALLOCATOR_ID_1,
                                      poolBuf_1,
                                      POOL_SIZE,
                                      IC_ALLOCATOR_MODE_ALLOC,
                                      0);
    IC_ASSERT(g_pAllocator != NULL);

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
    // lsf_connect前已经放入核间流对象
    // 初始化服务, 共享对象放进去, 初始化内置rpc机制, 注册到rpc, 以便接收调用
    IC_Service_init();

    // rpc启动, 并与client同步
    RPC_Server_Start();

    // 后台rpc任务运行: IC_Service远程发布共享对象.

    return LSF_SUCCESS;
}

/*
    各service处发布一张局部清单. 模块性更好.
    对ap来说, 各service-client(AP)只关心其使用到的 service核间属性 清单.
*/
int lsf_export_properties(int group_id, LsfPropertyConfig *config_array, int config_count)
{
    // 错误值转义后返回

    return LsfPropertyService_addGroup(group_id, config_array, config_count);
}

// 多线程场景考虑: 对端请求和本端service注册同时
int lsf_register(int group_id, int property_id,
                 LsfProperty_OnSetFunc onset_func, LsfProperty_OnGetFunc onget_func,
                 LSF_PROPERTY_CONTEXT_TYPE context_type, void *context_param)
{
    // TODO: 合法性检查
    int ret;

    // 属性服务 检索 管理条目
    LsfPropertyItem *property_item;
    ret = LsfPropertyService_find(group_id, property_id, &property_item);
    if (IC_OK == ret) {
        // 新建 核间属性 对象, 交给 属性服务, 从而激活 此核间属性

        // checkpoint
        IC_ASSERT(NULL == property_item->property);

        LsfProperty * property = LsfProperty_new(property_item->type);
        IC_ASSERT(property != NULL);

        // 核间属性 配置: 回调方法和运行上下文
        LsfProperty_register(property, onset_func, onget_func);

        LsfPropertyService_attach(property_item, property, context_type, context_param);
    }
    else {
        // TODO 错误处理: 不支持重复register
    }

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
    CLOGD("%s\n", __FUNCTION__);

    // ICProperty *property;
    // LsfPropertyService_find(group_id, property_id, & property);

    // ICProperty_set(property, var);

    return LSF_SUCCESS;
}

/**
  \brief       读取 核间属性, 同步阻塞
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_get(int group_id, int property_id, LsfVariant **out_var)
{
    CLOGD("%s\n", __FUNCTION__);

    // ICProperty *property;
    // LsfPropertyService_find(group_id, property_id, & property);

    // ICProperty_get(property, out_var);

    return LSF_SUCCESS;
}

int lsf_notify(int group_id, int property_id, LsfVariant *var)
{
    CLOGD("%s\n", __FUNCTION__);

    // TODO: 合法性检查
    int ret;

    // 属性服务 检索 管理条目
    LsfPropertyItem *property_item;
    ret = LsfPropertyService_find(group_id, property_id, &property_item);
    if (IC_OK == ret) {
        // checkpoint
        IC_ASSERT(NULL != property_item->property);

        // 核间属性 发出通知
        LsfProperty_notify(property_item->property, group_id, property_id, var);
    }
    else {
        // TODO 错误处理
    }

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
    // CLOGD("[ScanService > LSF_MSG_PROPERTY_DISPATCH]");

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

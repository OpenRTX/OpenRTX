// 本单元
#include "ic_property.h"
#include "ic_common.h"
#include "ic_allocator.h"

// 其他模块
#include "urpc_api.h"
#include "rpc_client.h"

// log模块
#include "venus_log.h"

// 平台
#include "lsf_os_port_internal.h"

#include <cmsis_gcc.h>

// 标准库
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// 外部定义 ------------------------------------------------

// 核间属性 自定义项目表. 类的全部对象, 索引分派rpc用.
// extern struct lsf_property_config lsf_property_config_table[];

// 核间属性 自定义项目表
// extern struct lsf_property_config lsf_property_config_table[10];
// #include "lsf_property_definition.impl"

// 核间内存管理
extern IC_Allocator *g_pAllocator;

// \------------------------------------------------------------
// 对象实例定义

// 不存在对象的核间共享部分

// \------------------------------------------------------------

// \------------------------------------------------------------
// 类静态数据和类静态方法
// rpc分发 由类方法完成, 分发给对象实例property
// TODO: 与远程通信相关的操作似乎可以抽出在通信基类?

// // 类初始化计数
// static bool _ICProperty_class_isInited = false;
// static int _ICProperty_class_init(void);

static void _LsfProperty_err_handle(urpc_frame* frame);

static void _LsfProperty_dummy_handler(urpc_frame* frame);

// property->set 的async response handler, ap->cp再ap<-cp时.
static void _LsfPropertyService_Set_Async_Responser(urpc_frame *frame);

static void _LsfPropertyService_Get_Async_Responser(urpc_frame * frame);

static void _LsfPropertyService_Notify_push_responser(urpc_frame * frame);

static void _LsfPropertyService_Notify_registerProxy_Responser(urpc_frame * frame);

// 类共享对象. rpc handler注册结构
// TODO: 需要 同步? dummy_handles_property[]
static urpc_handle _rpc_client_property_handles[] = {
        {_LsfProperty_err_handle,
            URPC_FLAG_RESPONSE | URPC_FLAG_ASYNC, URPC_DESC("prop_0")},

        {_LsfProperty_dummy_handler,
            URPC_FLAG_ASYNC | URPC_FLAG_RESPONSE, URPC_DESC("prop_1")},

        {_LsfPropertyService_Set_Async_Responser,
            URPC_FLAG_ASYNC | URPC_FLAG_RESPONSE, URPC_DESC("prop_2")},

        {_LsfPropertyService_Get_Async_Responser,
            URPC_FLAG_ASYNC | URPC_FLAG_RESPONSE, URPC_DESC("prop_3")},

        {_LsfPropertyService_Notify_push_responser,
            URPC_FLAG_PUSH | URPC_FLAG_RESPONSE, URPC_DESC("prop_4")},

        {_LsfPropertyService_Notify_registerProxy_Responser,
            URPC_FLAG_ASYNC | URPC_FLAG_RESPONSE, URPC_DESC("prop_5")},
};

// 属性服务 ------------------------------------------------

// 根据group数量, 分配管理表
typedef struct tagLsfPropertyGroup {
    // 本组property
    LsfPropertyItem *property_items;

    int count;

    // 本group是否已被export
    // bool group_exported;
} LsfPropertyGroup;

// 改名: LSF_PROPERTY_GROUP_COUNT -> +config
static LsfPropertyGroup s_property_groups[LSF_PROPERTY_GROUP_COUNT] = {0};

// 初始化计数
static bool _LsfPropertyService_isInited = false;

// 属性服务 > API ------------------------------------------------

// 语义上, 属性服务 发布/检索property对象. ap和cp的语义并不对称.
// ap语义: 检索/建立proxy, 所以不由用户来new对象/attach.
// cp的语义: 先查位, 再新建native对象, 并就位之.

// 检索表先建立, 以便先connect两端 属性服务, 但其中的 核间属性 对象 随着运行才能就位, 类似一种lazy加载
// 空配置表支持 属性服务的查询和获取请求即可
int LsfPropertyService_init(void)
{
    CLOGD("[api] %s", __FUNCTION__);

    // 删: 根据 自定义项目表, 初始化对象实例
    // int property_count = sizeof(lsf_property_config_table) / sizeof(lsf_property_config_table[0]);
    // for (int i = 0; i < property_count; ++i) {
    //     ICProperty * property = ICProperty_new(lsf_property_config_table[i].property_id,
    //                                            lsf_property_config_table[i].flag,
    //                                            lsf_property_config_table[i].type);
    //     IC_ASSERT(property != NULL);

    //     // 链接回 总表
    //     lsf_property_config_table[i].property = property;
    // }

    // 若已初始化过, 则忽略后续
    if (_LsfPropertyService_isInited) return 0;

    _LsfPropertyService_isInited = true;

    // 初始化rpc client相关
    // 配置"核间Property"所需handler, 注册到rpc, 以便接收和发起远程调用.
    RPC_Client_eps[RPC_CLIENT_EP_PROPERTY].handles = _rpc_client_property_handles;
    RPC_Client_eps[RPC_CLIENT_EP_PROPERTY].handles_cnt =
        sizeof(_rpc_client_property_handles) / sizeof(urpc_handle);

    return 0;
}

// 建立push通道, 以支持notify
// 语义: 无需两端对象或属性服务同步. 讨论见: [分布式对象框架.guga]
int LsfPropertyService_connect(void)
{
    CLOGD("[api] %s", __FUNCTION__);

    // consumer端同步时注册到对端producer的 "可用帧增加_push服务"
    // 对端的"push注册_handler"调用完毕即意味着同步完成
    urpc_frame buf;
    urpc_frame *frame = &buf;

    // 约定参数在: [0~4), uint32. 通知即可, 实际参数值无意义.
    write_u32((uint8_t *)(& (frame->rpc.payload[0])), 1);

    frame->header.eps = RPC_SERVER_EP_PROPERTY;
    frame->rpc.dst_id = 4;      // _LsfPropertyService_Notify_registerPush_handler
    frame->rpc.src_id = 4;      // _LsfPropertyService_Notify_push_responser

    int8_t urpc_ret = urpc_open_event_client(RPC_Client_stub, frame);
    IC_ASSERT(URPC_SUCCESS == urpc_ret, "ret: %d \n", urpc_ret);

    return 0;
}

// 内部数据多线程访问, 加mutex? 一写多读, 不用加mutex.
int LsfPropertyService_addGroup(int group_id, LsfPropertyConfig *config_array, int config_count)
{
    CLOGD("[api] %s", __FUNCTION__);

    if (NULL != s_property_groups[group_id].property_items) {
        // group已经存在, 不能add
        return -1; // E_GROUP_EXISTED
    }

    // 预备 管理表
    LsfPropertyItem *property_items = (LsfPropertyItem *) malloc(config_count * sizeof(LsfPropertyItem));
    IC_ASSERT(NULL != property_items);
    memset(property_items, 0, config_count * sizeof(LsfPropertyItem));

    // 配置表 -> 管理表 逐项填充, 空表, 无实际对象
    for (int i = 0; i < config_count; ++i) {
        // 录入 配置参数
        property_items[i].property_id = config_array[i].property_id;
        property_items[i].flag = config_array[i].flag;
        property_items[i].type = config_array[i].type;

        property_items[i].property = NULL;

        // 对象 链接回 总表?

        int id = config_array[i].property_id;
        // 期望 核间属性 清单 的排列符合预期: 0起始,递增
        IC_ASSERT(id == i);
    }

    s_property_groups[group_id].count = config_count;

    // 单核而言, compiler barrier 够用.
    __COMPILER_BARRIER();
    // 同时作为 有效性 标记, 按write-release语义, 放在最后更新.
    s_property_groups[group_id].property_items = property_items;

    return 0;
}

// 语义: 在服务本端中查询property条目, 而非property对象
// TODO: 多线程场景, rpc task和service task都在使用. 多读不写, 可以. 如同时有lsf_register写入?
int LsfPropertyService_find(int group_id, int property_id, LsfPropertyItem **out_property_item)
{
    CLOGD("[api] %s", __FUNCTION__);

    // 参数校验

    LsfPropertyItem *property_items = s_property_groups[group_id].property_items;

    // 单核而言, compiler barrier 够用.
    __COMPILER_BARRIER();
    // 作为 有效性 标记, 按read-acquire语义.

    // data dependency确保顺序
    // 检索到
    if ((NULL != property_items)
        && (s_property_groups[group_id].count > property_id)) {

        *out_property_item = & property_items[property_id];

        return 0;
    }

    // 未检索到
    *out_property_item = NULL;

    return -1;
}

// 语义: 在服务本端中查询property对象, 查不到则向对端检索并建立proxy, 再返回proxy对象.
// 首次查询到会建立proxy对象
// 约定 - 已经import属性表. 找到 - 意味着已经有proxy对象. 没找到 - proxy对象未建立.
// 多线程场景参考设计推演, rpc task和service task都在使用.
int LsfPropertyService_getProperty(int group_id, int property_id, LsfProperty **out_property,
                                   LSF_PROPERTY_CONTEXT_TYPE context_type, void *context_param)
{
    CLOGD("[api] %s", __FUNCTION__);

    // 参数校验

/*
    内部逻辑:
    获取对象,
        查位子(必须成功),查proxy对象(可能没有),
    若首次获取时, proxy对象不存在:
        检索对端 核间属性 对象, 对端必须先就位:
            因为 核间属性 随着 service出现比较晚, 甚至不随着service出现
            查询对端对象, 以id
        查询成功后:
            建立本端proxy对象
            并就位在 属性服务 的 核间属性 配置表中, 供上层按id检索
*/

    LsfPropertyItem *property_items = s_property_groups[group_id].property_items;
    // 单核而言, compiler barrier 够用.
    __COMPILER_BARRIER();
    // 作为 有效性 标记, 按read-acquire语义.

    // property条目 有效性检查
    if (!((NULL != property_items)
          && (s_property_groups[group_id].count > property_id))) {
        *out_property = NULL;
        return -1;
    }

    // data dependency确保顺序
    LsfProperty *property = property_items[property_id].property;
    // 单核而言, compiler barrier 够用.
    __COMPILER_BARRIER();
    // 作为 有效性 标记, 按read-acquire语义.

    // 如果已经建立好了对象, 直接返回就可以.
    if (NULL != property) {
        *out_property = property;
        return 0;
    }

    // 查询对端
    urpc_frame buf;
    urpc_frame *frame = &buf;

    uint32_t remote = 0;
    // group id 高16位
    remote |= (uint32_t)(group_id) << 16;
    // property id 低16位
    remote |= property_id;
    // 写入: property obj ID. 参数定义: 起始0, uint32.
    write_u32((uint8_t *)(& (frame->rpc.payload[0])), remote);

    // 对端endpoint == 本端响应endpoint
    frame->header.eps = RPC_SERVER_EP_PROPERTY;
    // 对端handler序号: _LsfPropertyService_Query_handler
    frame->rpc.dst_id = 1;
    // 本端响应的handler序号, not used in sync send
    frame->rpc.src_id = 1;

    urpc_trans_sync_client(RPC_Client_stub, frame, frame);

    // 读取: 查询结果 0/1. 参数定义: 起始0的uint32.
    uint32_t property_existed = read_u32((uint8_t *)(& (frame->rpc.payload[0])));

    CLOGD("[%s:event] property query result: %d\n", __FUNCTION__, property_existed);

    // 没查到, 返回出错, api使用者可重试
    if (0 == property_existed) {
        *out_property = NULL;
        return -1;
    }

    // 构建本端的proxy对象
    LsfPropertyItem *property_item = & s_property_groups[group_id].property_items[property_id];
    property = LsfProperty_new(property_item->type);
    IC_ASSERT(property != NULL);

    property_item->context_type = context_type;
    property_item->context_msgQ = context_param;

    // 单核而言, compiler barrier 够用.
    __COMPILER_BARRIER();
    // 作为 有效性 标记, 按write-release语义.
    property_item->property = property;

    *out_property = property;

    return 0;
}

// 待删除? 动态挂接 核间属性 proxy对象
// TODO: 多线程场景. 正在更新时, 有读取, 可能不一致? 加锁?
int LsfPropertyService_attach(LsfPropertyItem *property_item, LsfProperty *property,
                              LSF_PROPERTY_CONTEXT_TYPE context_type, void *context_param)
{
    property_item->context_type = context_type;
    property_item->context_msgQ = context_param;

    // 单核而言, compiler barrier 够用.
    __COMPILER_BARRIER();
    // 同时作为 有效性 标记, 按write-release语义, 放在最后更新.
    property_item->property = property;

    return 0;

    // 链接入 总表
    // lsf_property_config_table[i].property = property;
}

int LsfPropertyService_dispatch(LsfPropertyItem *property_item, LsfPropertyParcel *parcel)
{
    CLOGD("[api] %s", __FUNCTION__);

	lsf_base_type ret;

    // 根据其运行上下文, 不同post方法
    if (LSF_PROPERTY_CONTEXT_TASK == property_item->context_type) {
        // 通用msg格式; 所有支持在task中运行方法的service约定好;
        LsfMsg msg;
        msg.id = LSF_MSG_PROPERTY_DISPATCH;
        msg.param = (uint32_t) parcel;

        ret = lsf_queue_send_to_back(property_item->context_msgQ, & msg, 0);
        IC_ASSERT(lsf_task_pass == ret);
    }
    else if (LSF_PROPERTY_CONTEXT_NONE == property_item->context_type) {
        // case non无独立上下文:
        // 直接运行proxy端: LsfProperty_notify()->onNotify方法;
    }

    return 0;
}

// get/notify 分发给实际property对象, 不用查表了, msg中已经有对象地址
int LsfPropertyService_execute(LsfPropertyItem *property_item, LsfPropertyParcel *parcel)
{
    CLOGD("[api] %s", __FUNCTION__);

    // 这个过程归为 PropertyStub_onTransact()? 单个类似乎不明显
    LsfProperty *property = property_item->property;
    LsfVariant *var = &(parcel->var_arg);

    switch (parcel->func_id) {
    case LSF_PROPERTY_FUNC_ID_NOTIFY:

        // checkpoint: 确认可set
        // property_item->flag & set位

        // group_id, property_id 传入?
        LsfProperty_notify(property, property_item->property_id, var);

        // notify无需回应对端

        break;

    default:
        IC_ASSERT(false);
        break;
    }

    return 0;
}

// notify push handler, ap<-cp, 接收RPC调用, 并通知给app任务上下文的proxy property.
// 运行于: rpc后台task
// rpc类型: push.
static void _LsfPropertyService_Notify_push_responser(urpc_frame * frame)
{
    CLOGD("[api] %s", __FUNCTION__);

    int32_t ret;

    IC_ASSERT(NULL != frame);

    // property marshal记录 的结构基于约定: 0-3, 索引; 4-7, set参数Variant

    // 读取: property obj 索引. 参数定义: 起始0, uint32.
    uint32_t index = read_u32((uint8_t *)(& (frame->rpc.payload[0])));
    // group id 高16位
    int group_id = index >> 16;
    // property id 低16位
    int property_id = index & 0xFFFF;

    // 分包
    LsfPropertyParcel *parcel = (LsfPropertyParcel *) malloc(sizeof(LsfPropertyParcel));
    IC_ASSERT(NULL != parcel);
    memset(parcel, 0, sizeof(LsfPropertyParcel));

    int varType = frame->rpc.payload[8];

    // unmarshal: type单独
    parcel->var_arg.variant_type = varType;

    // unmarshal: 按type读取marshal记录
    if (varType == type_uint) {
        size_t var_size;
        // uint32 marshal记录在payload ==> parcel->var_arg

        LsfVariantClass_unmarshal(&(parcel->var_arg),
                                  (uint8_t *)(& (frame->rpc.payload[4])), 4, &var_size);
    }
    else if (varType == type_membuf || varType == type_pointer) {
        // marshal记录在: payload[4-7]
        void *pShareBlock = (void *) read_u32((uint8_t *)(& (frame->rpc.payload[4])));

        size_t varMarshalSize;
        LsfVariantClass_unmarshal(&(parcel->var_arg),
                                  (uint8_t *) pShareBlock, 0, &varMarshalSize);
    }

    parcel->func_id = LSF_PROPERTY_FUNC_ID_NOTIFY;

    // 定址: 属性服务 检索 管理条目
    LsfPropertyItem *property_item;
    ret = LsfPropertyService_find(group_id, property_id, &property_item);
    // 在ap能接收到notify之前, cp必定先获知有ap端监听者, 必能找到其管理条目.
    IC_ASSERT(IC_OK == ret);

    // checkpoint
    IC_ASSERT(NULL != property_item->property);
    // 单核而言, compiler barrier 够用.
    __COMPILER_BARRIER();
    // property_item->property同时作为 有效性 标记, acquire语义.

    parcel->property_item = property_item;

    // 投递
    LsfPropertyService_dispatch(property_item, parcel);

    return;
}

// Property类 ------------------------------------------------

// 分配/新建对象实例
LsfProperty* LsfProperty_new(int type)
{
    CLOGD("[api] %s", __FUNCTION__);

    LsfProperty *self = (LsfProperty *) malloc(sizeof(LsfProperty));
    IC_ASSERT(self != NULL);

    self = LsfProperty_ctor(self, type);
    IC_ASSERT(self != NULL);

    return self;
}

// 基于已分配对象, 构造.
// 删? - 私有, 用户只能通过"factory method"产生对象
LsfProperty* LsfProperty_ctor(LsfProperty *self, int type)
{
    CLOGD("[api] %s", __FUNCTION__);

    // self->ID = objID;
    self->type = type;


    return self;
}

// 删: 预先建立好对象, 不支持动态查询创建对象, 所以不需要这个了?
// 类静态函数, 反序列化构造对象, 以对应于对端对象.
// ICProperty* ICProperty_Creator_createObj(int remoteObjID)
// {
//     // 先类初始化
//     _ICProperty_Class_init();

//     // 再建立对象
//     // 实例分配
//     ICProperty* handle = ICProperty_newInstance();
//     if (handle == NULL) return NULL;
//     // 构造
//     handle = ICProperty_ctor(handle, remoteObjID);

//     return handle;
// }

// 暂时用不到, 非正式实现
int LsfProperty_dtor(LsfProperty *self)
{
    return IC_OK;
}

// 在task上下文中被调用
int LsfProperty_set(LsfProperty *self, int group_id, int property_id, LsfVariant *var)
{
    CLOGD("[api] %s", __FUNCTION__);

    // 通知对端: _LsfPropertyService_Set_In_handler, ep_property

    // set payload 格式约定: 0-3, transaction; 4-7, marshal记录; 8-9, remote+var_type

    lsf_queue_t sync_msgQ = lsf_queue_create(1 , sizeof(LsfMsg));
    uint32_t transaction = (uint32_t) sync_msgQ;

    urpc_frame buf;
    urpc_frame *frame = &buf;

    write_u32((uint8_t *)(& (frame->rpc.payload[0])), transaction);

    if (var->variant_type == type_uint) {
        // 完整marshal记录放在payload
        size_t size;
        // 写入: marshal记录. payload[4-7] = value;
        LsfVariantClass_marshal(var,
                                (uint8_t *)(& (frame->rpc.payload[4])), 4, &size);

        // 写入: marshal记录: type.
    }
    else if (var->variant_type == type_membuf || var->variant_type == type_pointer) {
        size_t varMarshalSize = LsfVariant_getMarshalSize(var);

        void *pShareBlock = IC_Allocator_alloc(g_pAllocator, varMarshalSize);
        // TODO: 延时重试
        IC_ASSERT(pShareBlock != NULL);

        size_t marshaledSize;

        LsfVariantClass_marshal(var, (uint8_t *)pShareBlock, varMarshalSize, & marshaledSize);

        IC_Sharemem_updateAndDetach(pShareBlock, varMarshalSize);

        // 写入: marshal记录. payload[4-7] = pShareBlock;
        write_u32((uint8_t *)(& (frame->rpc.payload[4])), (uint32_t)pShareBlock);

        // 写入: marshal记录: type.
    }

    // payload[8-9] = remote + var_type
    write_u16((uint8_t*)(&(frame->rpc.payload[8])),
              REMOTE_AND_VARTYPE_U16(group_id, property_id, var->variant_type));

    frame->header.eps = RPC_SERVER_EP_PROPERTY;
    frame->rpc.dst_id = 2;
    frame->rpc.src_id = 2;  // response handle id

    // 之前的数据写入 ->
    // synchronization & memory barrier. 期望: store
    __DSB();

    urpc_send_async_client(RPC_Client_stub, frame);

    // 属性服务 检索 管理条目
    LsfPropertyItem *property_item;
    int32_t ret = LsfPropertyService_find(group_id, property_id, &property_item);
    if(IC_OK != ret) {
        lsf_queue_delete(sync_msgQ);
        return ret;
    }
    IC_ASSERT(NULL != property_item->property);

    // LSF_PROPERTY_FLAG_SYNC: 需等待对端; 否则返回.
    if ((property_item->flag & LSF_PROPERTY_FLAG_SYNCMODE_MSK) == LSF_PROPERTY_FLAG_SYNC) {
        lsf_base_type result;
        LsfMsg return_msg = {0};
        result = lsf_queue_receive(sync_msgQ, & return_msg, lsf_max_delay);
        IC_ASSERT(lsf_task_pass == result);

        if (LSF_MSG_PROPERTY_SET_RESPONSE == return_msg.id) {
            // 可能返回错误码
            LsfVariant *out_var = (LsfVariant *) return_msg.param;

            CLOGD("set response: {type: %u, value: %u}", out_var->variant_type, out_var->u.uiVal);
            uint32_t cp_ret = out_var->u.uiVal;

            free(out_var);

            if (cp_ret != IC_OK) {
                lsf_queue_delete(sync_msgQ);
                return cp_ret;
            }
        }

    }

    lsf_queue_delete(sync_msgQ);
    return IC_OK;
}

// Notify对端的async response handler, ap<-cp
// 运行于: rpc后台task
// rpc类型: async response.
static void
_LsfPropertyService_Set_Async_Responser(urpc_frame * frame)
{
    CLOGD("[api] %s", __FUNCTION__);

    // 参数检查
    IC_ASSERT(NULL != frame);

    // set responser payload 格式约定: 0-3, transaction; 4-7, marshal记录; 8-9, remote+var_type

    lsf_queue_t sync_msgQ = (lsf_queue_t) read_u32((uint8_t *)(& (frame->rpc.payload[0])));

    // 定址
    uint16_t remote_and_vartype_u16 = read_u16((uint8_t *)(& (frame->rpc.payload[8])));
    // group id
    int group_id = REMOTE_GROUP_ID(remote_and_vartype_u16);
    // property id
    int property_id = REMOTE_PROPERTY_ID(remote_and_vartype_u16);

    LsfVariant *new_var = (LsfVariant *) malloc(sizeof(LsfVariant));
    IC_ASSERT(NULL != new_var);
    memset(new_var, 0, sizeof(LsfVariant));

    int varType = REMOTE_VARTYPE(remote_and_vartype_u16);

    // unmarshal: type单独
    new_var->variant_type = varType;

    // unmarshal: 按type读取marshal记录
    if (varType == type_uint) {
        size_t var_size;
        // uint32 marshal记录在payload ==> new_var

        LsfVariantClass_unmarshal(new_var,
                                  (uint8_t *)(& (frame->rpc.payload[4])), 4, &var_size);
    }

    // 属性服务 检索 管理条目
    LsfPropertyItem *property_item;
    int32_t ret = LsfPropertyService_find(group_id, property_id, &property_item);
    IC_ASSERT(IC_OK == ret);
    IC_ASSERT(NULL != property_item->property);

    if ((property_item->flag & LSF_PROPERTY_FLAG_SYNCMODE_MSK) == LSF_PROPERTY_FLAG_SYNC) {
        LsfMsg msg = {
            .id = LSF_MSG_PROPERTY_SET_RESPONSE,
            .param = (uint32_t) new_var,
        };
        lsf_base_type os_ret = lsf_queue_send_to_back(sync_msgQ, & msg , 0);
        IC_ASSERT(lsf_task_pass == os_ret);
    }

    return;
}

// 在task上下文中被调用, 同步函数
int LsfProperty_get(LsfProperty *self, int group_id, int property_id, LsfVariant **out_var)
{
    CLOGD("[api] %s", __FUNCTION__);

    // 通知对端: _LsfPropertyService_Get_In_handler, ep_property

    // get payload 格式约定: 0-3, transaction; 4-7, NA.; 8-9, remote+var_type

    lsf_queue_t sync_msgQ = lsf_queue_create(1 , sizeof(LsfMsg));
    uint32_t transaction = (uint32_t) sync_msgQ;

    urpc_frame buf;
    urpc_frame *frame = &buf;

    write_u32((uint8_t *)(& (frame->rpc.payload[0])), transaction);

    // payload[8-9] = remote + var_type (NA.)
    write_u16((uint8_t*)(&(frame->rpc.payload[8])),
              REMOTE_AND_VARTYPE_U16(group_id, property_id, 0));

    // TODO: 要用push方式?
    frame->header.eps = RPC_SERVER_EP_PROPERTY;
    frame->rpc.dst_id = 3;  // _LsfPropertyService_Get_In_handler
    frame->rpc.src_id = 3;  // _LsfPropertyService_Get_Async_Responser

    // 之前的数据写入 ->
    // synchronization & memory barrier. 期望: store
    __DSB();

    urpc_send_async_client(RPC_Client_stub, frame);

    // 等待消息队列的结果
    lsf_base_type result;
    LsfMsg msg;
    result = lsf_queue_receive(sync_msgQ, & msg, lsf_max_delay);
    IC_ASSERT(lsf_task_pass == result);

    if (LSF_MSG_PROPERTY_GET_RESPONSE == msg.id) {
        // 可能返回错误码

        *out_var = (LsfVariant *) msg.param;
    }

    lsf_queue_delete(sync_msgQ);
    return IC_OK;
}

// Notify对端的async response handler, ap<-cp
// 运行于: rpc后台task
// rpc类型: async response.
static void
_LsfPropertyService_Get_Async_Responser(urpc_frame * frame)
{
    CLOGD("[api] %s", __FUNCTION__);

    // 参数检查
    IC_ASSERT(NULL != frame);

    // get responser 格式约定: 0-3, transaction; 4-7, get回的Variant; 8-9, remote+var_type

    lsf_queue_t sync_msgQ = (lsf_queue_t) read_u32((uint8_t *)(& (frame->rpc.payload[0])));

    // 分包
    // 定址
    uint16_t remote_and_vartype_u16 = read_u16((uint8_t *)(& (frame->rpc.payload[8])));
    // group id
    int group_id = REMOTE_GROUP_ID(remote_and_vartype_u16);
    // property id
    int property_id = REMOTE_PROPERTY_ID(remote_and_vartype_u16);

    LsfVariant *new_var = (LsfVariant *) malloc(sizeof(LsfVariant));
    IC_ASSERT(NULL != new_var);
    memset(new_var, 0, sizeof(LsfVariant));

    int varType = REMOTE_VARTYPE(remote_and_vartype_u16);

    // unmarshal: type单独
    new_var->variant_type = varType;

    // unmarshal: 按type读取marshal记录
    if (varType == type_uint) {
        size_t var_size;
        // uint32 marshal记录在payload ==> new_var

        LsfVariantClass_unmarshal(new_var,
                                  (uint8_t *)(& (frame->rpc.payload[4])), 4, &var_size);
    }
    else if (varType == type_membuf || varType == type_pointer) {
        // marshal记录在: payload[4-7]
        void *pShareBlock = (void *) read_u32((uint8_t *)(& (frame->rpc.payload[4])));

        size_t varMarshalSize;
        LsfVariantClass_unmarshal(new_var,
                                  (uint8_t *) pShareBlock, 0, &varMarshalSize);
    }

    // 定址: 属性服务 检索 管理条目
    LsfPropertyItem *property_item;
    int32_t ret = LsfPropertyService_find(group_id, property_id, &property_item);
    IC_ASSERT(IC_OK == ret);
    IC_ASSERT(NULL != property_item->property);

    // TODO: 投递统一在dispatch? get是阻塞式的, 直接将结果发送到property私有消息队列.
    // LsfPropertyService_dispatch(property_item, parcel);

    LsfMsg msg = {
        .id = LSF_MSG_PROPERTY_GET_RESPONSE,
        .param = (uint32_t) new_var,
    };
    lsf_base_type os_ret = lsf_queue_send_to_back(sync_msgQ, & msg , 0);
    IC_ASSERT(lsf_task_pass == os_ret);

    return;
}

// 在task上下文中被调用
int LsfProperty_register(LsfProperty *self, int group_id, int property_id,
                         LsfProperty_OnSetFunc onnotify_func)
{
    CLOGD("[api] %s", __FUNCTION__);

    // 预备好on_nofity的运行条件
    self->on_notify = onnotify_func;

    // 通知对端: _LsfPropertyService_Notify_registerProxy_handler, ep_property
    // property marshal记录 的结构基于约定: 0-3, 索引;

    urpc_frame buf;
    urpc_frame *frame = &buf;

    uint32_t remote = 0;
    // group id 高16位
    remote |= (uint32_t)(group_id) << 16;
    // property id 低16位
    remote |= property_id;

    // 写入: property obj ID. 参数定义: 起始0, uint32.
    write_u32((uint8_t *)(& (frame->rpc.payload[0])), remote);

    frame->header.eps = RPC_SERVER_EP_PROPERTY;
    frame->rpc.dst_id = 5;  // _LsfPropertyService_Notify_registerProxy_handler
    frame->rpc.src_id = 5;  // _LsfPropertyService_Notify_registerProxy_Responser
    urpc_send_async_client(RPC_Client_stub, frame);

    return IC_OK;
}

// 运行于: rpc后台task
// rpc类型: async response.
static void
_LsfPropertyService_Notify_registerProxy_Responser(urpc_frame * frame)
{
    CLOGD("[api] %s", __FUNCTION__);

    // 无实际操作

    (void) frame;

    // CLOGD("%s\n", __FUNCTION__);
}

// proxy property: 在task(约定?)上下文中被调用, ->onNotify()
int LsfProperty_notify(LsfProperty *self, int property_id, LsfVariant *var)
{
    CLOGD("[api] %s", __FUNCTION__);

    // onGet方法
    if (self->on_notify != NULL) {
        // group_id, property_id 需要放在对象定义么?
        self->on_notify(0, property_id, var);
    }

    return IC_OK;
}

static void _LsfProperty_err_handle(urpc_frame* frame)
{
    CLOGD("[api] %s", __FUNCTION__);
}

static void _LsfProperty_dummy_handler(urpc_frame* frame)
{
    (void) frame;

    CLOGD("[api] %s", __FUNCTION__);
}

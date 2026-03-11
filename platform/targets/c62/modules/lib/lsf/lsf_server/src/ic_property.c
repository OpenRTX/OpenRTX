// 本单元
#include "ic_common.h"
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

// 对象的单核私有部分
// struct LsfProperty {
//     // 对象ID, 用来索引查找对象实例
//     int ID;

//     // fence_wait时用
//     XosMsgQueue* msgQ_wait;

//     // 与对端同步用, 以确认已经连接到对端. 同步点后, 两端才开始wait/notify.
//     XosSem sema_SyncPoint;
// };

// 对象池
// #define LsfProperty_INSTANCE_COUNT 2
// static int _instanceCounter = 0;
// static struct LsfProperty _instances[LsfProperty_INSTANCE_COUNT];

// 消息队列预分配内存
#define LsfProperty_QUEUE_LENGTH    1
#define LsfProperty_QUEUE_ITEM_SIZE (sizeof(uint32_t))

//XOS_MSGQ_ALLOC(_obj0_Queue, LsfProperty_QUEUE_LENGTH, LsfProperty_QUEUE_ITEM_SIZE);
//XOS_MSGQ_ALLOC(_obj1_Queue, LsfProperty_QUEUE_LENGTH, LsfProperty_QUEUE_ITEM_SIZE);
// static uint8_t _obj0_Queue_buf[ sizeof(XosMsgQueue) + ((LsfProperty_QUEUE_LENGTH) * (LsfProperty_QUEUE_ITEM_SIZE)) ];
// XosMsgQueue * _obj0_Queue = (XosMsgQueue *) _obj0_Queue_buf;
// static uint8_t _obj1_Queue_buf[ sizeof(XosMsgQueue) + ((LsfProperty_QUEUE_LENGTH) * (LsfProperty_QUEUE_ITEM_SIZE)) ];
// XosMsgQueue * _obj1_Queue = (XosMsgQueue *) _obj1_Queue_buf;

// XosMsgQueue * _msgQueue_pool[2] = {
//     (XosMsgQueue *) _obj0_Queue_buf,
//     (XosMsgQueue *) _obj1_Queue_buf,
// };
// \------------------------------------------------------------

// \------------------------------------------------------------
// 类静态数据和类静态方法
// TODO: 与远程通信相关的操作似乎可以抽出在通信基类?

static void _LsfProperty_err_handle(urpc_frame* frame);

// property对象检索handler, ap->cp
static void _LsfPropertyService_Query_handler(urpc_frame * frame);

// 对端发起set的handler, ap->cp
static void _LsfPropertyService_Set_In_handler(urpc_frame *frame);

// 对端发起get的handler, ap->cp
static void _LsfPropertyService_Get_In_handler(urpc_frame *frame);

// notify所用的push模板
static urpc_frame _LsfPropertyService_push_frame_template;

// 用于Notify对端的 push注册的handler, ap->cp
static void _LsfPropertyService_Notify_registerPush_handler(urpc_frame *frame);

// 用于Notify中ap端proxy注册的handler, ap->cp
static void _LsfPropertyService_Notify_registerProxy_handler(urpc_frame *frame);

// ep4. rpc handler注册结构
// TODO: 第0项改为预定的错误处理, 需要让出.
static urpc_handle _rpc_server_prop_handles[] = {
        {_LsfProperty_err_handle,
            URPC_FLAG_REQUEST | URPC_FLAG_ASYNC, URPC_DESC("prop_0")},

        {_LsfPropertyService_Query_handler,
            URPC_FLAG_REQUEST | URPC_FLAG_SYNC, URPC_DESC("prop_1")},

        {_LsfPropertyService_Set_In_handler,
            URPC_FLAG_REQUEST | URPC_FLAG_ASYNC, URPC_DESC("prop_2")},

        {_LsfPropertyService_Get_In_handler,
            URPC_FLAG_REQUEST | URPC_FLAG_ASYNC, URPC_DESC("prop_3")},

        {_LsfPropertyService_Notify_registerPush_handler,
            URPC_FLAG_PUSH | URPC_FLAG_REQUEST, URPC_DESC("prop_4")},

        {_LsfPropertyService_Notify_registerProxy_handler,
            URPC_FLAG_REQUEST | URPC_FLAG_ASYNC, URPC_DESC("prop_5")},
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

// 仅仅建立 空配置表, 不建立 核间属性对象, 那些是随着运行动态添加进来
// 空配置表支持 属性服务的查询和获取请求即可
int LsfPropertyService_init(void)
{
    CLOGD("[api] %s", __FUNCTION__);

    // 删: 根据 自定义项目表, 初始化对象实例
    // int property_count = sizeof(lsf_property_config_table) / sizeof(lsf_property_config_table[0]);
    // for (int i = 0; i < property_count; ++i) {
    //     LsfProperty * property = LsfProperty_new(lsf_property_config_table[i].property_id,
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
    RPC_Server_eps[RPC_SERVER_EP_PROPERTY].handles = _rpc_server_prop_handles;
    RPC_Server_eps[RPC_SERVER_EP_PROPERTY].handles_cnt =
        sizeof(_rpc_server_prop_handles) / sizeof(urpc_handle);

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
    memset(property_items, 0, config_count * sizeof(LsfPropertyItem));

    // 配置表 -> 管理表 逐项填充, 空表, 无实际对象
    for (int i = 0; i < config_count; ++i) {
        // 录入 配置参数
        property_items[i].group_id = group_id;
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
#pragma no_reorder_memory
    // 同时作为 有效性 标记, 按write-release语义, 放在最后更新.
    s_property_groups[group_id].property_items = property_items;

    return 0;
}

// 语义: 在服务本端中查询property条目, 而非property对象
// TODO: 多线程场景, rpc task和service task都在使用. 多读不写, 可以. 如同时有lsf_register写入?
int LsfPropertyService_find(int group_id, int property_id, LsfPropertyItem **out_property_item)
{
    // CLOGD("[api] %s", __FUNCTION__);

    // 参数校验

    LsfPropertyItem *property_items = s_property_groups[group_id].property_items;

    // 单核而言, compiler barrier 够用.
#pragma no_reorder_memory
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

// 动态挂接 核间属性 对象
// TODO: 多线程场景. 正在更新时, 有读取, 可能不一致? 加锁?
int LsfPropertyService_attach(LsfPropertyItem *property_item, LsfProperty *property,
                              LSF_PROPERTY_CONTEXT_TYPE context_type, void *context_param)
{
    CLOGD("[api] %s", __FUNCTION__);

    property_item->context_type = context_type;
    property_item->context_msgQ = context_param;

    // 单核而言, compiler barrier 够用.
#pragma no_reorder_memory
    // 同时作为 有效性 标记, 按write-release语义, 放在最后更新.
    property_item->property = property;

    return 0;

    // 链接入 总表
    // lsf_property_config_table[i].property = property;
}

int LsfPropertyService_dispatch(LsfPropertyItem *property_item, LsfPropertyParcel *parcel)
{
    int32_t ret;

    // CLOGD("[api] %s", __FUNCTION__);

    // LsfPropertyParcel *parcel = (LsfPropertyParcel *) malloc(sizeof(LsfProperty_Parcel));
    // parcel->property_item = property_item;
    // parcel->func_id = func_id;
    // parcel->arg = var;

    // async模式: 在dispatch时就回应.
    // sync模式: 方法执行完毕才回应.

    // async模式.默认忽略, 对端不可能在等待吧, 不等 对端怎么知道是出错了呢? 还是得等
    // 或者 约定: async模式, 错误自负, 不保证set成功
    // lsf_reply(property_id, SKIP);

    // 根据其运行上下文, 不同post方法
    if (LSF_PROPERTY_CONTEXT_TASK == property_item->context_type) {
        // 通用msg格式; 所有支持在task中运行方法的service约定好;
        LsfMsg msg;
        msg.id = LSF_MSG_PROPERTY_DISPATCH;
        msg.param = (uint32_t) parcel;

        ret = xos_msgq_put(property_item->context_msgQ, (uint32_t *) & msg);
        IC_ASSERT(XOS_OK == ret);
    }
    else if (LSF_PROPERTY_CONTEXT_NONE == property_item->context_type) {
        // case non无独立上下文:
        // 直接运行native端: LsfProperty_set()->onSet方法;
        // urpc_send_async_server(RPC_Server_stub, frame);
    }

    return 0;
}

// 分发给实际property对象, 不用查表了, msg中已经有对象地址
int LsfPropertyService_execute(LsfPropertyItem *property_item, LsfPropertyParcel *parcel)
{
    // CLOGD("[api] %s", __FUNCTION__);

    // sync模式下:
    // 若目前状态还未就绪, 需要返回错误码
    // lsf_reply(property_id, OK);
    // lsf_reply(property_id, ERROR);

    // 这个过程归为 PropertyStub_onTransact()? 单个类似乎不明显
    LsfProperty *property = property_item->property;
    LsfVariant *var = &(parcel->var_arg);

    int ret = 0;

    switch (parcel->func_id) {
    case LSF_PROPERTY_FUNC_ID_SET:

        // checkpoint: 确认可set
        // property_item->flag & set位

        // group_id 冗余信息 不传, property_id 传入.
        ret = LsfProperty_set(property, property_item->group_id, property_item->property_id, var);

        // 若property为sync模式, 此处回应对端
        if ((property_item->flag & LSF_PROPERTY_FLAG_SYNCMODE_MSK) == LSF_PROPERTY_FLAG_SYNC) {
            // urpc_send_async_server(RPC_Server_stub, &(parcel->frame));

            // 完整marshal记录放在payload
            // CLOGD("Put u.uiVal: %d", ret);
            LsfVariant return_var;
            LsfVariant_init(&return_var);
            return_var.variant_type = type_uint;
            return_var.u.uiVal = ret;

            size_t size;
            // 写入: marshal记录. payload[4-7] = value;
            LsfVariantClass_marshal(&return_var,
                                    (uint8_t *)(& (parcel->frame.rpc.payload[4])), 4, &size);
            // CLOGD("LsfVariantClass_marshal size: %u", size);

            // 复用收到的rpc frame, 用于后续async模式回应. part 2: variant type
            uint16_t remote_and_vartype_u16 = read_u16((uint8_t *)(& (parcel->frame.rpc.payload[8])));
            remote_and_vartype_u16 = REMOTE_PATCH_VARTYPE_U16(remote_and_vartype_u16, type_uint);
            // payload[8-9] = remote + var_type
            write_u16((uint8_t*)(&(parcel->frame.rpc.payload[8])), remote_and_vartype_u16);

            // 之前的数据写入 ->
            // synchronization & memory barrier. 期望: store
#pragma flush_memory

            urpc_send_async_server(RPC_Server_stub, &(parcel->frame));
        }

        break;

    case LSF_PROPERTY_FUNC_ID_GET:

        // checkpoint: 确认可Get
        // property_item->flag & set位

        // group_id 冗余信息 不传, property_id 传入.
        // get的variant复用parcel, 统一free.
        LsfProperty_get(property, property_item->group_id, property_item->property_id, var);

        // get回应->part2: variant值

        if (var->variant_type == type_uint) {
            // 完整marshal记录放在payload
            size_t size;
            // 写入: marshal记录. payload[4-7] = value;
            LsfVariantClass_marshal(var,
                                    (uint8_t *)(& (parcel->frame.rpc.payload[4])), 4, &size);
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
            write_u32((uint8_t *)(& (parcel->frame.rpc.payload[4])), (uint32_t)pShareBlock);
        }

        // 复用收到的rpc frame, 用于后续async模式回应. part 2: variant type
        uint16_t remote_and_vartype_u16 = read_u16((uint8_t *)(& (parcel->frame.rpc.payload[8])));
        remote_and_vartype_u16 = REMOTE_PATCH_VARTYPE_U16(remote_and_vartype_u16, var->variant_type);
        // payload[8-9] = remote + var_type
        write_u16((uint8_t*)(&(parcel->frame.rpc.payload[8])), remote_and_vartype_u16);

        // 之前的数据写入 ->
        // synchronization & memory barrier. 期望: store
#pragma flush_memory

        urpc_send_async_server(RPC_Server_stub, &(parcel->frame));

        break;

    case LSF_PROPERTY_FUNC_ID_NOTIFY_REGISTER:

        // checkpoint: 确认可notify
        // property_item->flag & notify位

        // group_id, property_id 传入?
        // get的variant复用parcel, 统一free.
        LsfProperty_registerNotify(property, property_item->property_id, NULL);

        // registerProxy回应->part2: async回应

        urpc_send_async_server(RPC_Server_stub, &(parcel->frame));

        break;

    default:
        IC_ASSERT(false);
        break;
    }

    return 0;
}

// \------------------------------------------------------------
// 函数实现
// TODO: 所有函数加IC_ASSERT

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
// 私有, 用户只能通过"factory method"产生对象
LsfProperty* LsfProperty_ctor(LsfProperty *self, int type)
{
    CLOGD("[api] %s", __FUNCTION__);

    int32_t ret = XOS_OK;

    // self->ID = objID;
    self->type = type;

    // xos new msgQ(item: uint32, 长度: 1)
    // ret = xos_msgq_create(_msgQueue_pool[objID],
    //                         LsfProperty_QUEUE_LENGTH, LsfProperty_QUEUE_ITEM_SIZE, 0);
    // self->msgQ_wait = _msgQueue_pool[objID];
    // IC_ASSERT(NULL != self->msgQ_wait);

    // xos new sema: binary semaphore. 初始0, 需要对端激发.
    // ret = xos_sem_create(& self->sema_SyncPoint, XOS_SEM_WAIT_PRIORITY, 0);
    // IC_ASSERT( XOS_OK == ret );

    // 本对象加入类的对象实例索引
    // _Fence_Objs[objID] = self;

    return self;
}

// [对象工厂] 类静态函数, 新建对象, 对应于对端对象.
// LsfPropertyHandle
// LsfProperty_Creator_createObj(int objID)
// {
//     // 先类初始化
//     _LsfProperty_Class_init();

//     // 再建立对象
//     // 实例分配
//     LsfPropertyHandle handle = LsfProperty_newInstance();
//     if (handle == NULL) return NULL;
//     // 构造
//     handle = LsfProperty_ctor(handle, objID);

//     return handle;
// }

// TODO: 暂时用不到, 非正式实现
int
LsfProperty_dtor(LsfProperty *self)
{
    // TODO:
    // 若是最后一个本类对象, 从rpc中去掉
    // 从类对象索引中去掉
    // 将实例空间还给对象池

    return IC_OK;
}

int LsfProperty_register(LsfProperty *self,
                         LsfProperty_OnSetFunc onset_func,
                         LsfProperty_OnGetFunc onget_func)
{
    CLOGD("[api] %s", __FUNCTION__);

    // 删? 对象状态变化? inited -> 激活态?

    self->on_set = onset_func;
    self->on_get = onget_func;

    return IC_OK;
}

// 本端native对象先就绪, 再由对端查询成功->建立proxy对象,
// 再, proxy对象其实已经无需sync操作, 因为已经是就绪状态, 直接可用.
// TODO: app线程使用, 对象级的核间同步
//     若property未激活状态, 则返回错误状态: 无效
//     若property未激活状态, 则 不管错误状态
// int LsfProperty_syncWithRemote(LsfPropertyHandle self)
// {
//     int32_t ret;

//     // 以此为对象级的核间同步标志.
//     ret = xos_sem_get(& self->sema_SyncPoint);
//     IC_ASSERT(XOS_OK == ret);

//     return IC_OK;
// }

// native处理, 在task(约定?)上下文中被调用, ->onSet()
int LsfProperty_set(LsfProperty *self, int group_id, int property_id, LsfVariant *var)
{
    // CLOGD("[api] %s", __FUNCTION__);

    // onSet方法
    if (self->on_set != NULL) {
        return self->on_set(group_id, property_id, var);

        // TODO: onSet 成功/失败 需要后续处理?
    }

    // if self->模式 == sync {
    //     frame放在哪里呢?
    //     urpc_frame buf;
    //     urpc_frame *frame = &buf;

    //     // 复用frame以回复async rpc, 忽略返回值
    //     uint8_t id;
    //     // 对端发送Notify时, response handler已经记录在frame中, 此处互换即可
    //     id = frame->rpc.dst_id;
    //     frame->rpc.dst_id = frame->rpc.src_id;
    //     frame->rpc.src_id = id;

    //     urpc_send_async_server 回应以解除对端等待
    //     urpc_send_async_server(RPC_Server_stub, frame);

    // }

    return IC_OK;
}

// native处理, 在task(约定?)上下文中被调用, ->onGet()
int LsfProperty_get(LsfProperty *self, int groupd_id, int property_id, LsfVariant *var)
{
    // CLOGD("[api] %s", __FUNCTION__);

    // onGet方法
    if (self->on_get != NULL) {
        self->on_get(groupd_id, property_id, var);

        // TODO: onGet 成功/失败 需要后续处理?
    }

    return IC_OK;
}

// native处理, 在task(约定?)上下文中被调用, onRegisterNotify 复用 onSet()
int LsfProperty_registerNotify(LsfProperty *self, int property_id, LsfVariant *var)
{
    CLOGD("[api] %s", __FUNCTION__);

    // onRegisterNotify方法
    if (self->on_set != NULL) {
        // group_id 冗余信息 不传, property_id 传入.
        self->on_set(0, property_id, var);
    }

    return IC_OK;
}

// 运行于: app线程. 由远端经由rpc handler唤醒
// 等待对端通知, 如果对端已经有通知, 则立即返回, 否则阻塞等待让出调度
// int
// LsfProperty_wait(LsfPropertyHandle self, uint32_t *outMsg)
// {
//     int32_t ret;

//     uint32_t msg;

//     // 等待handler中转来的消息
//     ret = xos_msgq_get(self->msgQ_wait, & msg);
//     IC_ASSERT(XOS_OK == ret);

//     // 返回值
//     *outMsg = msg;

//     // 需要? 如果是wait后再读取共享变量, 就要以确保顺序, 但是那该由应用负责吧?
//     // memory barrier: read_acquire
// #pragma flush_memory

//     return IC_OK;
// }

int
LsfProperty_notify(LsfProperty *self, int group_id, int property_id,
                   LsfVariant *var)
{
    CLOGD("[api] %s", __FUNCTION__);

    // push的方式发出server端通知, 对端已经注册push服务.

    uint32_t remote = 0;
    // group id 高16位
    remote |= (uint32_t)(group_id) << 16;
    // property id 低16位
    remote |= property_id;

    // 基于已经记录的push消息模板, 填充: 核间属性 对象ID
    // 参数定义: 起始0, uint32. 起始 4, Variant序列化.
    write_u32((uint8_t *)(& (_LsfPropertyService_push_frame_template.rpc.payload[0])), remote);

    if (var->variant_type == type_uint) {
        // 完整marshal记录放在payload
        size_t size;
        // 写入: marshal记录. payload[4-7] = value;
        LsfVariantClass_marshal(var,
                                (uint8_t *)(& (_LsfPropertyService_push_frame_template.rpc.payload[4])),
                                4, &size);

        // 写入: marshal记录: type.
        _LsfPropertyService_push_frame_template.rpc.payload[8] = type_uint;
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
        write_u32((uint8_t *)(& (_LsfPropertyService_push_frame_template.rpc.payload[4])), (uint32_t)pShareBlock);
        // 写入: marshal记录: type.
        _LsfPropertyService_push_frame_template.rpc.payload[8] = var->variant_type;
    }

    // 之前的数据写入 ->
    // synchronization & memory barrier. 期望: store
#pragma flush_memory

    // 使用push注册时记录的frame信息, 发出通知
    urpc_push_event_server(RPC_Server_stub, & _LsfPropertyService_push_frame_template);

    return IC_OK;
}

// TODO: 对端可能多次查询, 检查幂等性设计
// 场景: 本端还未export属性列表, 对端已经来检索.
// 多线程场景: 正在更新时, 有对端查询.
static void _LsfPropertyService_Query_handler(urpc_frame * frame)
{
    // CLOGD("[api] %s", __FUNCTION__);

    IC_ASSERT(NULL != frame);

    // 读取: property obj 索引. 参数定义: 起始0, uint32.
    uint32_t index = read_u32((uint8_t *)(& (frame->rpc.payload[0])));
    // group id 高16位
    int group_id = index >> 16;
    // property id 低16位
    int property_id = index & 0xFFFF;

    uint32_t query_result = 0;
    // 定址: 属性服务 检索 管理条目
    LsfPropertyItem *property_item;
    int32_t ret = LsfPropertyService_find(group_id, property_id, &property_item);

    if (IC_OK == ret) {
        // 若附属property对象存在时, 才回应找到.
        if (NULL != property_item->property) {
            query_result = 1;
        }
    }
    // 单核而言, compiler barrier 够用.
#pragma no_reorder_memory
    // property_item->property同时作为 有效性 标记, acquire语义.

    // CLOGD("[%s:event] query result: %d\n", __FUNCTION__, query_result);

    // 写入: 查询结果 0/1. 参数定义: 起始0的uint32.
    write_u32((uint8_t *)(& (frame->rpc.payload[0])), query_result);

    // 对端发来时, 已经填好回应的handler号. 此处互换rpc目标和源地址即可.
    uint8_t id;
    id = frame->rpc.dst_id;
    frame->rpc.dst_id = frame->rpc.src_id;
    frame->rpc.src_id = id;
    // 发出
    urpc_send_sync_server(RPC_Server_stub, frame);

    return;
}

// 对端Notify的handler, ap->cp, 接收ap的RPC调用, 并通知给app任务上下文的流对象.
// rpc类型: async. client端的async handler空函数, client发通知后不阻塞.
// 运行于: rpc后台线程
static void _LsfPropertyService_Set_In_handler(urpc_frame *frame)
{
    // CLOGD("[api] %s", __FUNCTION__);

    int32_t ret;

    IC_ASSERT(NULL != frame);

    // set payload 格式约定: 0-3, transaction; 4-7, set参数Variant; 8-9, remote+var_type

    uint16_t remote_and_vartype_u16 = read_u16((uint8_t *)(& (frame->rpc.payload[8])));
    // group id
    int group_id = REMOTE_GROUP_ID(remote_and_vartype_u16);
    // property id
    int property_id = REMOTE_PROPERTY_ID(remote_and_vartype_u16);

    // 分包
    LsfPropertyParcel *parcel = (LsfPropertyParcel *) malloc(sizeof(LsfPropertyParcel));
    IC_ASSERT(NULL != parcel);
    memset(parcel, 0, sizeof(LsfPropertyParcel));

    int varType = REMOTE_VARTYPE(remote_and_vartype_u16);

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

    parcel->func_id = LSF_PROPERTY_FUNC_ID_SET;

    // 根据sync/async:
    // 若sync, 到达此处, 则两端property对象已经连接, 肯定是激活状态.

    // 定址: 属性服务 检索 管理条目
    LsfPropertyItem *property_item;
    ret = LsfPropertyService_find(group_id, property_id, &property_item);
    IC_ASSERT(IC_OK == ret);

    // checkpoint
    // TODO: 若附属property对象不存在时, 则回应错误码.
    IC_ASSERT(NULL != property_item->property);
    // 单核而言, compiler barrier 够用.
#pragma no_reorder_memory
    // 同时作为 有效性 标记, acquire语义.

    parcel->property_item = property_item;

    // 复用frame以回复async rpc, 忽略返回值
    uint8_t id;
    // 对端发送Notify时, response handler已经记录在frame中, 此处互换即可
    id = frame->rpc.dst_id;
    frame->rpc.dst_id = frame->rpc.src_id;
    frame->rpc.src_id = id;

    // 复用收到的rpc frame, 用于后续async模式回应. part 1: transaction, remote
    parcel->frame = *frame;

    // 若property为async模式, 此处回应对端, 不等到实际onSet完毕.
    if ((property_item->flag & LSF_PROPERTY_FLAG_SYNCMODE_MSK) == LSF_PROPERTY_FLAG_ASYNC) {
        urpc_send_async_server(RPC_Server_stub, frame);
    }

    // 投递
    LsfPropertyService_dispatch(property_item, parcel);

    return;
}

// 对端Get的handler, ap->cp, 接收ap的RPC调用, 并通知给app任务上下文的native对象.
// rpc类型: async. client端的async handler空函数, client发通知后不阻塞.
// 运行于: rpc后台线程
static void _LsfPropertyService_Get_In_handler(urpc_frame *frame)
{
    // CLOGD("[api] %s", __FUNCTION__);

    int32_t ret;

    IC_ASSERT(NULL != frame);

    // get payload 格式约定: 0-3, transaction; 4-7, NA.; 8-9, remote+var_type

    uint16_t remote_and_vartype_u16 = read_u16((uint8_t *)(& (frame->rpc.payload[8])));
    // group id
    int group_id = REMOTE_GROUP_ID(remote_and_vartype_u16);
    // property id
    int property_id = REMOTE_PROPERTY_ID(remote_and_vartype_u16);

    // 分包
    LsfPropertyParcel *parcel = (LsfPropertyParcel *) malloc(sizeof(LsfPropertyParcel));
    IC_ASSERT(NULL != parcel);
    memset(parcel, 0, sizeof(LsfPropertyParcel));

    parcel->func_id = LSF_PROPERTY_FUNC_ID_GET;

    // 定址: 属性服务 检索 管理条目
    LsfPropertyItem *property_item;
    ret = LsfPropertyService_find(group_id, property_id, &property_item);
    IC_ASSERT(IC_OK == ret);

    // checkpoint
    // TODO: 若附属property对象不存在时, 则回应错误码.
    IC_ASSERT(NULL != property_item->property);
    // 单核而言, compiler barrier 够用.
#pragma no_reorder_memory
    // 同时作为 有效性 标记, acquire语义.

    parcel->property_item = property_item;

    // 复用frame以回复async rpc, 忽略返回值
    uint8_t id;
    // 对端发送Get时, response handler已经记录在frame中, 此处互换即可
    id = frame->rpc.dst_id;
    frame->rpc.dst_id = frame->rpc.src_id;
    frame->rpc.src_id = id;

    // 复用收到的rpc frame, 用于后续async模式回应. part 1: transaction, remote
    parcel->frame = *frame;

    // 投递
    LsfPropertyService_dispatch(property_item, parcel);

    return;
}

// 对端同步时注册到本端的push通道, ap->cp, 仅调用一次.
// 运行于: rpc后台线程
// rpc类型: push注册
static void
_LsfPropertyService_Notify_registerPush_handler(urpc_frame* frame)
{
    uint8_t id;
    CLOGD("[api] %s", __FUNCTION__);

    // 暂存在rpc frame模板, 留待主动push时使用
    _LsfPropertyService_push_frame_template.header = frame->header;
    // 对端发起push注册时, push的response handler已经记录在frame中, 此处互换即可
    _LsfPropertyService_push_frame_template.rpc.dst_id = frame->rpc.src_id;
    _LsfPropertyService_push_frame_template.rpc.src_id = frame->rpc.dst_id;

    return;
}

// 用于Notify中ap端proxy注册的handler, ap->cp, 仅调用一次.
// 运行于: rpc后台线程
// rpc类型: async
static void
_LsfPropertyService_Notify_registerProxy_handler(urpc_frame* frame)
{
    CLOGD("[api] %s", __FUNCTION__);

    int32_t ret;

    IC_ASSERT(NULL != frame);

    // property marshal记录 的结构基于约定: 0-3, 索引;

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

    parcel->func_id = LSF_PROPERTY_FUNC_ID_NOTIFY_REGISTER;

    // 定址: 属性服务 检索 管理条目
    LsfPropertyItem *property_item;
    ret = LsfPropertyService_find(group_id, property_id, &property_item);
    IC_ASSERT(IC_OK == ret);

    // checkpoint
    // TODO: 若附属property对象不存在时, 则回应错误码.
    IC_ASSERT(NULL != property_item->property);
    // 单核而言, compiler barrier 够用.
#pragma no_reorder_memory
    // 同时作为 有效性 标记, acquire语义.

    parcel->property_item = property_item;

    // 复用frame以回复async rpc, 忽略返回值
    uint8_t id;
    // 对端发送registerProxy时, response handler已经记录在frame中, 此处互换即可
    id = frame->rpc.dst_id;
    frame->rpc.dst_id = frame->rpc.src_id;
    frame->rpc.src_id = id;

    // 记录rpc frame, 用于后续async模式回应
    parcel->frame = *frame;

    uint32_t remote = 0;
    // group id 高16位
    remote |= (uint32_t)(group_id) << 16;
    // property id 低16位
    remote |= property_id;

    // registerProxy回应->part1: 目标对象
    // 写入: property obj ID. 参数定义: 起始0, uint32.
    write_u32((uint8_t *)(& (parcel->frame.rpc.payload[0])), remote);

    // 投递
    LsfPropertyService_dispatch(property_item, parcel);

    return;
}

static void
_LsfProperty_err_handle(urpc_frame* frame)
{
    CLOGD("[api] %s", __FUNCTION__);
}

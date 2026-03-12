// 本模块
#include "ic_common.h"
#include "ic_fence.h"

// 其他模块
#include "urpc_api.h"
#include "rpc_client.h"
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

//------------------------------------------------
#undef CLOGD
#define CLOGD(fmt, ...)

// \------------------------------------------------------------
// 对象实例定义

// 不存在对象的核间共享部分

// 对象的单核私有部分
struct ICFence {
    // 对象ID, 用来索引查找对象实例
    int ID;

    // fence_wait时用
    lsf_queue_t msgQ_wait;
};

// 消息队列
#define ICFENCE_QUEUE_LENGTH    1
#define ICFENCE_QUEUE_ITEM_SIZE (sizeof(uint32_t))

// 对象池
#define ICFENCE_INSTANCE_COUNT 4
static int _instanceCounter = 0;
static struct ICFence _instances[ICFENCE_INSTANCE_COUNT];
// \------------------------------------------------------------

// \------------------------------------------------------------
// 类静态数据和类静态方法
// TODO: 与远程通信相关的操作似乎可以抽出在通信基类?

// 类初始化计数
static bool _ICFence_class_isInited = false;
static int _ICFence_class_init(void);

// 类的全部对象, 索引分派rpc用. TODO: 目前场景4个够用, 可改为根据宏定义, 或者动态分配更灵活透明
static ICFenceHandle _Fence_Objs[4];

static void
_ICFence_err_handle(urpc_frame* frame);

// 对端(RPC Server) Notify的 rpc push handler, ap<-cp, 接收RPC调用, 并通知给app任务上下文的Fence对象.
static void _ICFence_Notify_In_PushResponser(urpc_frame *frame);
// 空帧增加的async response handler, ap->cp再ap<-cp时.
static void _ICFence_Notify_Out_Async_Responser(urpc_frame *frame);

// ep3, 类共享对象. rpc handler注册结构
// TODO: 第0项将改为预定的错误处理, 需要让出.
static urpc_handle _rpc_client_ep3_handles[] = {
        {_ICFence_err_handle,
            URPC_FLAG_RESPONSE | URPC_FLAG_ASYNC, URPC_DESC("ep3_e")},

        {_ICFence_Notify_In_PushResponser,
            URPC_FLAG_PUSH | URPC_FLAG_RESPONSE, URPC_DESC("ep3_1")},
        {_ICFence_Notify_Out_Async_Responser,
            URPC_FLAG_ASYNC | URPC_FLAG_RESPONSE, URPC_DESC("ep3_2")}
};
// \------------------------------------------------------------

// TODO: 所有函数加assert
static int
_ICFence_Class_init(void)
{
    // 已经初始化过, 则忽略后续
    if (_ICFence_class_isInited) return 0;

    _ICFence_class_isInited = true;

    // 初始化rpc client相关
    // 配置"核间Fence"所需handler, 注册到rpc, 以便接收和发起远程调用.
    RPC_Client_eps[RPC_CLIENT_EP_3].handles = _rpc_client_ep3_handles;
    RPC_Client_eps[RPC_CLIENT_EP_3].handles_cnt =
        sizeof(_rpc_client_ep3_handles) / sizeof(urpc_handle);

    return 0;
}

// 分配对象实例
static ICFenceHandle
ICFence_newInstance(void)
{
    ICFenceHandle h = NULL;

    if (_instanceCounter < ICFENCE_INSTANCE_COUNT) {
        h = & _instances[_instanceCounter];
        _instanceCounter++;
    }

    return h;
}

// 基于已分配对象, 构造
// 私有, 用户只能通过"factory method"产生对象
static ICFenceHandle
ICFence_ctor(ICFenceHandle self, int objID)
{
    self->ID = objID;

    // new msgQ(item: uint32, 长度: 1)
    self->msgQ_wait = lsf_queue_create(ICFENCE_QUEUE_LENGTH, ICFENCE_QUEUE_ITEM_SIZE);
    IC_ASSERT(NULL != self->msgQ_wait);

    // 本对象加入类的对象实例索引
    _Fence_Objs[objID] = self;

    CLOGD("[%s] ICFence @ 0x%p: ID %d", __FUNCTION__,
          self, self->ID);

    return self;
}

// 类静态函数, 反序列化构造对象, 以对应于对端对象.
ICFenceHandle
ICFence_Creator_createObj(int remoteFenceObjID)
{
    // 先类初始化
    _ICFence_Class_init();

    // 再建立对象
    // 实例分配
    ICFenceHandle handle = ICFence_newInstance();
    if (handle == NULL) return NULL;
    // 构造
    handle = ICFence_ctor(handle, remoteFenceObjID);

    return handle;
}

// 暂时用不到, 非正式实现
int
ICFence_dtor(ICFenceHandle self)
{
    return IC_OK;
}

// 与对端Fence对象同步. 否则, 如何确保Fence_Notify一定能通知到对端?
int
ICFence_syncWithRemote(ICFenceHandle self)
{
    // 注册到对端的"push服务"
    CLOGD("[%s] ICFence @ 0x%p : ID %d.", __FUNCTION__,
          self, self->ID);

    // 与对端对象建立rpc push关联, 以便接收Fence_notify
    // 对端的"fence_push注册_handler"调用完毕即意味着同步完成: ep3_1
    urpc_frame buf;
    urpc_frame *frame = &buf;
    // 对端对象同样objID
    write_u32((uint8_t *)(& (frame->rpc.payload[0])), self->ID);
    frame->header.eps = RPC_SERVER_EP_3;
    frame->rpc.dst_id = 1+1;
    frame->rpc.src_id = 0+1;  // response handle id: 本端 ep3_0

    urpc_open_event_client(RPC_Client_stub, frame);

    // 一般而言, 初始化过程, 然后再回复确认, 双方才可正式开始协作, 以免不同步
    // 但简单场景下, 暂时省略回复确认
    // RPC_阻塞发送(AP的确认);

    // 再思考: 当open还未完成, 对方已经发来push消息. 概念上似乎有点怪.
    // 相当于, sync还未完全结束, 就能收到push消息了, 虽然结果没问题, 概念上...
    // 期望sync退出后, 才可能接收/处理push消息.

    return IC_OK;
}

// 运行于: app线程. 由远端经由rpc handler唤醒
// 等待对端通知, 如果对端已经有通知, 则立即返回, 否则阻塞等待让出调度
int
ICFence_wait(ICFenceHandle self, uint32_t *outMsg)
{
    BaseType_t ret;

    uint32_t msg;

    CLOGD("[%s] ICFence @ 0x%p : ID %d.", __FUNCTION__,
          self, self->ID);

    // 等待handler中转来的消息
    ret = lsf_queue_receive(self->msgQ_wait, & msg, lsf_max_delay);
    IC_ASSERT(lsf_task_pass == ret);

    CLOGD("[%s > event] msg received: %d.", __FUNCTION__,
          msg);

    // 返回值
    *outMsg = msg;

    // 需要? 如果是wait后再读取共享变量, 就要以确保顺序, 但是那该由应用负责吧?
    // memory barrier: read_acquire
    __DSB();

    return IC_OK;
}

int
ICFence_notify(ICFenceHandle self, uint32_t msg)
{
    // synchronization & memory barrier: write_release. full system, any-any = 15
    __DSB();

    CLOGD("[%s] ICFence @ 0x%p : ID %d, msg %d.", __FUNCTION__,
      self, self->ID, msg);

    // 通知对端: _ICFence_Notify_In_handler, ep3_1
    // 约定msg参数在: 0起始, uint32.
    urpc_frame buf;
    urpc_frame *frame = &buf;

    // 写入: fence obj ID. 参数定义: 起始0, uint32.
    write_u32((uint8_t *)(& (frame->rpc.payload[0])), self->ID);
    // 写入: fence notify 所带的 msg. 参数定义: 起始4, uint32.
    write_u32((uint8_t *)(& (frame->rpc.payload[4])), msg);

    frame->header.eps = RPC_SERVER_EP_3;
    frame->rpc.dst_id = 0+1;
    frame->rpc.src_id = 1+1;  // response handle id
    urpc_send_async_client(RPC_Client_stub, frame);

    return IC_OK;
}

// 对端(RPC Server) Notify的 rpc push handler, ap<-cp, 接收RPC调用, 并通知给app任务上下文的Fence对象.
// 运行于: rpc后台task
// rpc类型: push.
static void
_ICFence_Notify_In_PushResponser(urpc_frame *frame)
{
    (void) frame;

    CLOGD("[%s] .", __FUNCTION__);

    // 读取: fence obj ID. 参数定义: 起始0, uint32.
    uint32_t objID = read_u32((uint8_t *)(& (frame->rpc.payload[0])));

    // 读取: fence notify 所带的 msg. 参数定义: 起始4, uint32.
    uint32_t notifyMsg = read_u32((uint8_t *)(& (frame->rpc.payload[4])));

    CLOGD("[%s > event] notify objID:%d with msg:%d.", __FUNCTION__,
          objID, notifyMsg);

    BaseType_t ret;
    // 发送消息, 唤醒阻塞于Fence_Wait的task
    ret = lsf_queue_send(_Fence_Objs[objID]->msgQ_wait, & notifyMsg, lsf_max_delay);
    // TODO: 目前总假定先有task wait, 所以必定成功.
    IC_ASSERT(lsf_task_pass == ret);

    return;
}

// Notify对端的async response handler, ap<-cp
// 运行于: rpc后台task
// rpc类型: async response.
static void
_ICFence_Notify_Out_Async_Responser(urpc_frame * frame)
{
    // 无实际操作
    (void) frame;

    CLOGD("[%s] .", __FUNCTION__);
}

static void
_ICFence_err_handle(urpc_frame* frame)
{
    CLOGD("[%s] %d.", __FUNCTION__, frame->header.magic);
}

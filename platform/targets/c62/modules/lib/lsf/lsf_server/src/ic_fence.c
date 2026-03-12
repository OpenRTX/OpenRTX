// 本模块
#include "ic_common.h"
#include "ic_fence.h"

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
    XosMsgQueue* msgQ_wait;

    // 与对端同步用, 以确认已经连接到对端. 同步点后, 两端才开始wait/notify.
    XosSem sema_SyncPoint;
};

// 对象池
#define ICFENCE_INSTANCE_COUNT 4
static int _instanceCounter = 0;
static struct ICFence _instances[ICFENCE_INSTANCE_COUNT];

// 消息队列预分配内存
#define ICFENCE_QUEUE_LENGTH    1
#define ICFENCE_QUEUE_ITEM_SIZE (sizeof(uint32_t))

//XOS_MSGQ_ALLOC(_obj0_Queue, ICFENCE_QUEUE_LENGTH, ICFENCE_QUEUE_ITEM_SIZE);
//XOS_MSGQ_ALLOC(_obj1_Queue, ICFENCE_QUEUE_LENGTH, ICFENCE_QUEUE_ITEM_SIZE);
static uint8_t _obj0_Queue_buf[ sizeof(XosMsgQueue) + ((ICFENCE_QUEUE_LENGTH) * (ICFENCE_QUEUE_ITEM_SIZE)) ];
XosMsgQueue * _obj0_Queue = (XosMsgQueue *) _obj0_Queue_buf;
static uint8_t _obj1_Queue_buf[ sizeof(XosMsgQueue) + ((ICFENCE_QUEUE_LENGTH) * (ICFENCE_QUEUE_ITEM_SIZE)) ];
XosMsgQueue * _obj1_Queue = (XosMsgQueue *) _obj1_Queue_buf;
static uint8_t _obj2_Queue_buf[ sizeof(XosMsgQueue) + ((ICFENCE_QUEUE_LENGTH) * (ICFENCE_QUEUE_ITEM_SIZE)) ];
XosMsgQueue * _obj2_Queue = (XosMsgQueue *) _obj2_Queue_buf;
static uint8_t _obj3_Queue_buf[ sizeof(XosMsgQueue) + ((ICFENCE_QUEUE_LENGTH) * (ICFENCE_QUEUE_ITEM_SIZE)) ];
XosMsgQueue * _obj3_Queue = (XosMsgQueue *) _obj3_Queue_buf;

XosMsgQueue * _msgQueue_pool[4] = {
    (XosMsgQueue *) _obj0_Queue_buf,
    (XosMsgQueue *) _obj1_Queue_buf,
    (XosMsgQueue *) _obj2_Queue_buf,
    (XosMsgQueue *) _obj3_Queue_buf,
};
// \------------------------------------------------------------

// \------------------------------------------------------------
// 类静态数据和类静态方法
// TODO: 与远程通信相关的操作似乎可以抽出在通信基类?

// 类初始化计数
static bool _ICFence_class_isInited = false;
static int _ICFence_class_init(void);

// 类的全部对象, 索引分派rpc用. TODO: 目前场景2个够用, 可改为根据宏定义, 或者动态分配更灵活透明
static ICFenceHandle _Fence_Objs[4];

static void
_ICFence_err_handle(urpc_frame* frame);

// 对端发起Notify的handler, ap->cp
static void _ICFence_Notify_In_handler(urpc_frame *frame);
// 用于Notify对端的 push注册的handler, ap->cp
static void _ICFence_Notify_Out_registerPush_handler(urpc_frame *frame);

// 记录的rpc push消息模板, 一个足够, 再通过objID区分目标
static urpc_frame _push_frame_template;

// ep3. rpc handler注册结构
// TODO: 第0项将改为预定的错误处理, 需要让出.
static urpc_handle _rpc_server_ep3_handles[] = {
        {_ICFence_err_handle,
            URPC_FLAG_REQUEST | URPC_FLAG_ASYNC, URPC_DESC("ep3_e")},

        {_ICFence_Notify_In_handler,
            URPC_FLAG_REQUEST | URPC_FLAG_ASYNC, URPC_DESC("ep3_1")},
        {_ICFence_Notify_Out_registerPush_handler,
            URPC_FLAG_PUSH | URPC_FLAG_REQUEST, URPC_DESC("ep3_2")}
};
// \------------------------------------------------------------

// \------------------------------------------------------------
// 函数实现
// TODO: 所有函数加assert

// 获取类的指定对象实例
int
ICFence_Class_getObjID(int objID)
{
    // TODO: 检查参数

    return  _Fence_Objs[objID]->ID;
}


static int
_ICFence_Class_init(void)
{
    // 若已初始化过, 则忽略后续
    if (_ICFence_class_isInited) return 0;

    _ICFence_class_isInited = true;

    // 初始化rpc client相关
    // 配置"核间Fence"所需handler, 注册到rpc, 以便接收和发起远程调用.
    RPC_Server_eps[RPC_SERVER_EP_3].handles = _rpc_server_ep3_handles;
    RPC_Server_eps[RPC_SERVER_EP_3].handles_cnt =
        sizeof(_rpc_server_ep3_handles) / sizeof(urpc_handle);

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

// 基于已分配对象, 构造.
// 私有, 用户只能通过"factory method"产生对象
static ICFenceHandle
ICFence_ctor(ICFenceHandle self, int objID)
{
    int32_t ret = XOS_OK;

    self->ID = objID;

    // xos new msgQ(item: uint32, 长度: 1)
    ret = xos_msgq_create(_msgQueue_pool[objID],
                            ICFENCE_QUEUE_LENGTH, ICFENCE_QUEUE_ITEM_SIZE, 0);
    self->msgQ_wait = _msgQueue_pool[objID];
    IC_ASSERT(NULL != self->msgQ_wait);

    // xos new sema: binary semaphore. 初始0, 需要对端激发.
    ret = xos_sem_create(& self->sema_SyncPoint, XOS_SEM_WAIT_PRIORITY, 0);
    IC_ASSERT( XOS_OK == ret );

    // 本对象加入类的对象实例索引
    _Fence_Objs[objID] = self;

    CLOGD("[%s] ICFence @ 0x%p: ID %d", __FUNCTION__,
          self, self->ID);

    return self;
}

// [对象工厂] 类静态函数, 新建对象, 对应于对端对象.
ICFenceHandle
ICFence_Creator_createObj(int objID)
{
    // 先类初始化
    _ICFence_Class_init();

    // 再建立对象
    // 实例分配
    ICFenceHandle handle = ICFence_newInstance();
    if (handle == NULL) return NULL;
    // 构造
    handle = ICFence_ctor(handle, objID);

    return handle;
}

// TODO: 暂时用不到, 非正式实现
int
ICFence_dtor(ICFenceHandle self)
{
    // TODO:
    // 若是最后一个本类对象, 从rpc中去掉
    // 从类对象索引中去掉
    // 将实例空间还给对象池

    return IC_OK;
}

// app线程使用, 对象级的核间同步
int
ICFence_syncWithRemote(ICFenceHandle self)
{
    int32_t ret;

    CLOGD("[%s] ICFence @ 0x%p : ID %d.", __FUNCTION__,
          self, self->ID);

    // 以此为对象级的核间同步标志.
    ret = xos_sem_get(& self->sema_SyncPoint);
    IC_ASSERT(XOS_OK == ret);

    CLOGD("[%s > event] got sema_SyncPoint.", __FUNCTION__);

    return IC_OK;
}

// 运行于: app线程. 由远端经由rpc handler唤醒
// 等待对端通知, 如果对端已经有通知, 则立即返回, 否则阻塞等待让出调度
int
ICFence_wait(ICFenceHandle self, uint32_t *outMsg)
{
    int32_t ret;

    uint32_t msg;

    CLOGD("[%s] ICFence @ 0x%p : ID %d.", __FUNCTION__,
          self, self->ID);

    // 等待handler中转来的消息
    ret = xos_msgq_get(self->msgQ_wait, & msg);
    IC_ASSERT(XOS_OK == ret);

    CLOGD("[%s > event] msg received: %d.", __FUNCTION__,
          msg);

    // 返回值
    *outMsg = msg;

    // 需要? 如果是wait后再读取共享变量, 就要以确保顺序, 但是那该由应用负责吧?
    // memory barrier: read_acquire
#pragma flush_memory

    return IC_OK;
}

int
ICFence_notify(ICFenceHandle self, uint32_t msg)
{
    // memory barrier: write_release
#pragma flush_memory

    CLOGD("[%s] ICFence @ 0x%p : ID %d, msg %d.", __FUNCTION__,
      self, self->ID, msg);

    // push的方式发出server端通知, 对端已经注册push服务, 解除对端的可能阻塞.
    // 通知对端: _ICFence_Notify_In_PushResponser, ep3_0

    // 基于已经记录的push消息模板, 填充: objID, msg
    // 写入: fence obj ID. 参数定义: 起始0, uint32.
    write_u32((uint8_t *)(& (_push_frame_template.rpc.payload[0])), self->ID);
    // 写入: fence notify 所带的 msg. 参数定义: 起始4, uint32.
    write_u32((uint8_t *)(& (_push_frame_template.rpc.payload[4])), msg);
    // 使用push注册时记录的frame信息, 发出通知
    urpc_push_event_server(RPC_Server_stub, & _push_frame_template);

    return IC_OK;
}

// 对端Notify的handler, ap->cp, 接收ap的RPC调用, 并通知给app任务上下文的流对象.
// rpc类型: async. client端的async handler空函数, client发通知后不阻塞.
// 运行于: rpc后台线程
static void
_ICFence_Notify_In_handler(urpc_frame *frame)
{
    IC_ASSERT(NULL != frame);

    // 读取: fence obj ID. 参数定义: 起始0, uint32.
    int objID = read_u32((uint8_t *)(& (frame->rpc.payload[0])));

    // 读取: fence notify 所带的 msg. 参数定义: 起始4, uint32.
    uint32_t notifyMsg = read_u32((uint8_t *)(& (frame->rpc.payload[4])));

    CLOGD("[%s > event] notification come in: objID:%d, msg:%d.", __FUNCTION__,
          objID, notifyMsg);

    // 发送消息, 唤醒阻塞于Fence_Wait的task
    int32_t ret = xos_msgq_put(_Fence_Objs[objID]->msgQ_wait, & notifyMsg);
    // TODO: 目前总假定先有task wait, 所以必定成功.
    IC_ASSERT(XOS_OK == ret);

    // 复用frame以回复async rpc, 忽略返回值
    uint8_t id;
    // 对端发送Notify时, response handler已经记录在frame中, 此处互换即可
    id = frame->rpc.dst_id;
    frame->rpc.dst_id = frame->rpc.src_id;
    frame->rpc.src_id = id;

    urpc_send_async_server(RPC_Server_stub, frame);
}

// 对端同步时注册到本对象的push服务, ap->cp, 仅调用一次.
// 运行于: rpc后台线程
// rpc类型: push注册
static void
_ICFence_Notify_Out_registerPush_handler(urpc_frame* frame)
{
    uint8_t id;

    CLOGD("[%s] .", __FUNCTION__);

    // 暂存在rpc frame模板, 留待主动push时使用
    _push_frame_template.header = frame->header;
    // 对端发起push注册时, push的response handler已经记录在frame中, 此处互换即可
    _push_frame_template.rpc.dst_id = frame->rpc.src_id;
    _push_frame_template.rpc.src_id = frame->rpc.dst_id;

    int32_t ret;
    // 对端同步时, 注册到本端"push服务".
    // 以此为对象级的核间同步标志.
    // 对端对象同样objID, 本端同步依赖于此
    uint32_t objID = read_u32((uint8_t *)(& (frame->rpc.payload[0])));
    IC_ASSERT(objID == _Fence_Objs[objID]->ID);

    CLOGD("[%s > event] trigger sema_SyncPoint: objID:%d.", __FUNCTION__,
          objID);

    ret = xos_sem_put(& _Fence_Objs[objID]->sema_SyncPoint);
    IC_ASSERT(XOS_OK == ret);
}

static void
_ICFence_err_handle(urpc_frame* frame)
{
    CLOGD("[%s]:%d", __FUNCTION__, frame->header.magic);
}

// 本模块
#include "ic_stream.h"

#include "ic_common.h"

// 其他模块
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

#include "app_trace.h"

//------------------------------------------------
int Consumer_dataFramesIncrement_handler_Count = 0;
int ICStream_Consumer_commitRemote_urpc_push_Count = 0;

//------------------------------------------------
#define INSIDE( m, x, M) ((m <= x) && (x < M))

// TODO: 与远程通信相关的操作似乎可以抽出在通信基类?

// \------------------------------------------------------------
// 类方法, 二次分发至对象

static bool _ICStreamClass_inited = false;
static bool _ICStream_Comm_inited = false;

// 实例的起始和大小都必须是cacheline对齐, 以免双核对同一cacheline的相邻变量不可控读写.
// 以0x200c0000的64k ram 作为共享内存

#define ICSTREAMSHARE_OBJS_NUM 4
#define ICSTREAM_OBJS_NUM ICSTREAMSHARE_OBJS_NUM

__attribute__((section (".share_sram.data"), aligned(IC_MAX_DCACHE_LINESIZE)))
static unsigned char _ICStreamShare_objs[ICSTREAMSHARE_OBJS_NUM][IC_DCACHELINE_ROUNDUP_SIZE(sizeof(struct ICStreamShare))];
// static int _ICStreamShare_obj_counter = 0;

// 类全体对象跟踪, 用于远程会话的管理, 会话ID用于索引对象
static ICStream *_ICStream_objs[ICSTREAM_OBJS_NUM];
// static int _ICStream_obj_counter = 0;

// TBD: 改为从共享内存的对象池alloc(sizeof(obj))
static struct ICStreamShare *
ICStreamShare_allocObj(int objID)
{
    struct ICStreamShare *result = NULL;

    if (objID < ICSTREAMSHARE_OBJS_NUM) {
        result = (struct ICStreamShare *) _ICStreamShare_objs[objID];

        // ++ _ICStreamShare_obj_counter;
    }

    return result;
}

static int
_ICStream_Class_init(void)
{
    // 已经初始化过, 则忽略后续
    if (_ICStreamClass_inited) return 0;

    _ICStreamClass_inited = true;

    // TBD: 类全体对象跟踪, 用于远程会话的管理, 概念上应该放在ic_proxy统一记录.
    for (int i = 0; i < ICSTREAM_OBJS_NUM; ++i) {
        _ICStream_objs[i] = NULL;
    }

    return 0;
}

ICStream *
ICStream_Class_getObj(int objID)
{
    if (objID < ICSTREAM_OBJS_NUM) {
        return _ICStream_objs[objID];
    }

    return NULL;
}

static void
_ICStream_err_handle(urpc_frame* frame);

// 空帧增加handler, ap->cp
static void _Producer_emptyFramesIncrement_handler(urpc_frame * frame);
// 数据帧增加的push注册的handler, ap->cp
static void _Producer_dataFramesIncrement_registerPush_handler(urpc_frame * frame);

// 数据帧增加handler, ap->cp
static void _Consumer_dataFramesIncrement_handler(urpc_frame * frame);
// 空帧增加的push注册的handler, ap->cp
static void _Consumer_emptyFramesIncrement_registerPush_handler(urpc_frame * frame);

// consumer/producer共用ep2, 共享对象. rpc handler注册结构
// TODO: 第0项将改为预定的错误处理, 需要让出.
static urpc_handle _rpc_server_ep2_handles[] = {
        {_ICStream_err_handle,
            URPC_FLAG_REQUEST | URPC_FLAG_ASYNC, URPC_DESC("ep2_e")},

        {_Producer_emptyFramesIncrement_handler,
            URPC_FLAG_REQUEST | URPC_FLAG_ASYNC, URPC_DESC("ep2_0")},
        {_Producer_dataFramesIncrement_registerPush_handler,
            URPC_FLAG_PUSH | URPC_FLAG_REQUEST, URPC_DESC("ep2_1")},

        {_Consumer_dataFramesIncrement_handler,
            URPC_FLAG_REQUEST | URPC_FLAG_ASYNC, URPC_DESC("ep2_2")},
        {_Consumer_emptyFramesIncrement_registerPush_handler,
            URPC_FLAG_PUSH | URPC_FLAG_REQUEST, URPC_DESC("ep2_3")},
};
// \------------------------------------------------------------
static int s_bug_counter = 0;

static void
_ICStream_err_handle(urpc_frame* frame)
{
    s_bug_counter ++;

    CLOGD("%s:%d", __FUNCTION__, frame->header.magic);
}

// TODO: 所有函数加assert

ICStream *
ICStream_ctor(void *addr, ICStream_Config *config, int objID,
              void *pcmBuffer, int pcmBufferLen, int mode)
{
    // TODO: ringbuf必须为frame整数倍? 参数检查?

    // 先类初始化
    _ICStream_Class_init();

    // 对象初始化
    ICStream *self = (ICStream *) addr;

    struct ICStreamShare *share = ICStreamShare_allocObj(objID);
    if (share == NULL) return NULL;

    int fifoUnitCount = pcmBufferLen / sizeof(uint8_t);
    fifo_init(&(share->fifo), (uint8_t *) pcmBuffer, fifoUnitCount);

    share->cellSize = config->cellSize;
    share->channelCount = config->channelCount;

    share->sampleCountPerFrame = config->sampleCountPerFrame;
    share->frameCount_InternalBuf = config->frameCount_InternalBuf;

    share->frameCount_In = config->frameCount_In;
    share->frameCount_Out = config->frameCount_Out;

    share->apAddrOffset = config->apAddrOffset;

    share->in_sampleCount = share->frameCount_In * share->sampleCountPerFrame;
    share->out_sampleCount = share->frameCount_Out * share->sampleCountPerFrame;

    share->fifoUnitCountPerSample = share->channelCount * share->cellSize / sizeof(uint8_t);
    share->fifoUnitCountPerFrame = share->sampleCountPerFrame * share->fifoUnitCountPerSample;

    share->in_fifoUnitCount = share->in_sampleCount * share->fifoUnitCountPerSample;
    share->out_fifoUnitCount = share->out_sampleCount * share->fifoUnitCountPerSample;

    self->share = share;

    self->ID = objID;

    self->mode = mode;

    //\------------------------------------------------------------
    // 远程对象机制

    int32_t ret;

    if (ICSTREAM_MODE_CONSUMER == mode) {
        ICStreamFifo *q = & share->fifo;

        // 本端先放弃所有数据帧所有权, 后续接到对端通知时, 直接读取就是最新值
        // TODO: 确认head就是完全的ring buffer, 改为base更好?
        //      如有预置数据, head更好? 绕回要处理.
        xthal_dcache_region_invalidate((void *) q->head.l, q->size);

        // 数据帧数 信号量, 初始化为0
        ret = xos_sem_create(& self->sema_consumerDataFrames, 0, 0);
        IC_ASSERT(XOS_OK == ret);

        // 远程同步信号量, 初始化为0
        ret = xos_sem_create(& self->sema_consumerSyncPoint, 0, 0);
        IC_ASSERT(XOS_OK == ret);
    }
    else if (ICSTREAM_MODE_PRODUCER == mode) {
        // 空帧数信号量, 初始化为最大空帧数
        ret = xos_sem_create(& self->sema_producerEmptyFrames, 0, share->frameCount_InternalBuf);
        IC_ASSERT(XOS_OK == ret);

        // 远程同步信号量, 初始化为0
        ret = xos_sem_create(& self->sema_producerSyncPoint, 0, 0);
        IC_ASSERT(XOS_OK == ret);
    }

    ICStreamFifo *q = &(share->fifo);
    CLOGD("size of ICStreamShare: %d", sizeof(struct ICStreamShare));
    CLOGD("size of ICStreamFifo: %d at %p %p %p", sizeof(ICStreamFifo),
            &(q->head.l), &(q->tail.l), &(q->size));

    // 加入远程会话跟踪
    _ICStream_objs[objID] = self;

    // 共用通信机制初始化
    if (! _ICStream_Comm_inited) {
        // 配置"核间stream服务"所需handler, 注册到rpc, 以便接收和发起远程调用.
        RPC_Server_eps[RPC_SERVER_EP_2].handles = _rpc_server_ep2_handles;
        RPC_Server_eps[RPC_SERVER_EP_2].handles_cnt =
            sizeof(_rpc_server_ep2_handles) / sizeof(urpc_handle);

        _ICStream_Comm_inited = true;
    }

    return self;
}

// TODO: 暂时用不到, 非正式实现
int
ICStream_dtor(ICStream *self)
{
    return IC_OK;
}

// detach: 用户不再使用. 但并不析构, 保持urpc连接, 后续复用.
int ICStream_detach(ICStream *self)
{
    int32_t ret;

    if (ICSTREAM_MODE_PRODUCER == self->mode) {
        // 数据区不需要invalidate

        // 控制区不需要invalidate

        // 异步用法: 数据帧数 信号量不同步. 但仍尽量复位至预期值 = 最大空帧数,
        // 可能有异步并发+1. 不确定 frameCount_InternalBuf 次.
        // TODO: 如果之前 信号量 已经 > 最大空帧数, 则未做动作. 应该降至 = 最大空帧数.
        //       或者, 在sem_put时, 增加_max.
        do {
            ret = xos_sem_put_max(& self->sema_producerEmptyFrames,
                                  self->share->frameCount_InternalBuf);
        } while (ret == XOS_OK);
    }
    else if (ICSTREAM_MODE_CONSUMER == self->mode) {
        ICStreamFifo *q = & self->share->fifo;

        // 本端先放弃所有数据帧所有权, 后续接到对端通知时, 直接读取就是最新值
        // base: 完全的ring buffer.
        xthal_dcache_region_invalidate((void *) q->base, q->size);

        CLOGD("[ICStream_detach]: sem = %d",
              xos_sem_test(& self->sema_consumerDataFrames));
        // 控制区不需要invalidate

        // memory barrier: write_release
#pragma flush_memory

        // 异步用法: 数据帧数 信号量不同步. 但仍尽量复位至预期值 = 0,
        // 可能有异步并发+1. 不确定 frameCount_InternalBuf 次.
        do {
            ret = xos_sem_tryget(& self->sema_consumerDataFrames);
        } while (ret == XOS_OK);
    }

    return 0;
}

// re-config, 只改变传输参数, 其余不变
// 约定: 参数: 有效的ICStream
int ICStream_reconfig(ICStream *self, ICStream_Config *config,
                      void *pcmBuffer, int pcmBufferLen)
{
    // TODO: ringbuf必须为frame整数倍? 参数检查?

    struct ICStreamShare *share = self->share;

    int fifoUnitCount = pcmBufferLen / sizeof(uint8_t);
    fifo_init(&(share->fifo), (uint8_t *) pcmBuffer, fifoUnitCount);

    share->cellSize = config->cellSize;
    share->channelCount = config->channelCount;

    share->sampleCountPerFrame = config->sampleCountPerFrame;
    share->frameCount_InternalBuf = config->frameCount_InternalBuf;

    share->frameCount_In = config->frameCount_In;
    share->frameCount_Out = config->frameCount_Out;

    share->in_sampleCount = share->frameCount_In * share->sampleCountPerFrame;
    share->out_sampleCount = share->frameCount_Out * share->sampleCountPerFrame;

    share->fifoUnitCountPerSample = share->channelCount * share->cellSize / sizeof(uint8_t);
    share->fifoUnitCountPerFrame = share->sampleCountPerFrame * share->fifoUnitCountPerSample;

    share->in_fifoUnitCount = share->in_sampleCount * share->fifoUnitCountPerSample;
    share->out_fifoUnitCount = share->out_sampleCount * share->fifoUnitCountPerSample;

    // ICStreamShare刷入共享内存
    xthal_dcache_region_writeback(share, sizeof(struct ICStreamShare));

    // synchronization barrier, 确保先写入.
#pragma flush_memory

    //\------------------------------------------------------------
    // 远程对象机制

    int32_t ret;

    if (ICSTREAM_MODE_CONSUMER == self->mode) {
        ICStreamFifo *q = & share->fifo;

        // 本端先放弃所有数据帧所有权, 后续接到对端通知时, 直接读取就是最新值
        // base: 完全的ring buffer.
        xthal_dcache_region_invalidate((void *) q->base, q->size);

        // data sync
#pragma flush_memory
    }
    else if (ICSTREAM_MODE_PRODUCER == self->mode) {
        // nothing to do
    }

    return 0;
}

//------------------------------------------------
// producer模式: 实现

int
ICStream_Producer_fetchRemote(ICStream *self)
{
    ICStreamFifo *q = & (self->share->fifo);

    // 从内存更新read|head_index, read-only version
    // invalidate cache
    xthal_dcache_region_invalidate((void *) &(q->head_index.r),
                                    IC_DCACHELINE_ROUNDUP_SIZE(sizeof(FIFO_SIZE_T)));

    // memory barrier: read_acquire
#pragma flush_memory

    // 后续读取的read指针将是共享内存中的最新值

    // write指针仍然使用本端cache的

    // 空帧的数据区不管, 对端已经交出所有权, 本端产生数据后flush至内存.
    // TODO: 要根据数据由dma或cpu产生, 也要从cache invalidate, 否则出错.

    return IC_OK;
}

// #undef CLOGD
// #define CLOGD(fmt, ...)

bool
ICStream_Producer_isFull(ICStream *self)
{
    struct ICStreamShare *share = self->share;

    int nAvailEmptyFifoUnit = fifo_avail(&(share->fifo));
    // CLOGD("Producer frames: %d r %d head: %p tail: %p", nAvailEmptyFifoUnit/(share->in_fifoUnitCount),
    //         nAvailEmptyFifoUnit % (share->in_fifoUnitCount), share->fifo.head.r, share->fifo.tail.l);

    // 可用总空间(非连续)至少够1帧
    if (nAvailEmptyFifoUnit >= (share->in_fifoUnitCount)) {
        // 确认是连续空间
        int nContEmptySlots = fifo_lavail(&(share->fifo));
        IC_ASSERT(nContEmptySlots >= (share->in_fifoUnitCount));

        xos_log_write(Producer_isFull,
                      (uint32_t) 0,
                      (uint32_t) nAvailEmptyFifoUnit/(share->in_fifoUnitCount));

        // 则isFull不成立
        return false;
    }

    xos_log_write(Producer_isFull,
                  (uint32_t) 1,
                  (uint32_t) nAvailEmptyFifoUnit/(share->in_fifoUnitCount));

    // 可用总空间少于1 frame, 则isFull成立
    return true;
}

// 运行于: app线程. 由远端经由rpc handler唤醒
// 等待空帧出现, 如果已经有空帧, 则立即返回, 否则阻塞等待让出调度
int
ICStream_Producer_waitFrame(ICStream *self)
{
    int32_t ret;
    XosSem* sem = & self->sema_producerEmptyFrames;

    // 等待"空帧数量" > 0
    ret = xos_sem_get(& self->sema_producerEmptyFrames);
    IC_ASSERT(XOS_OK == ret);

    xos_log_write(Producer_waitFrame_Take,
             (uint32_t) sem,
             (uint32_t) sem->count);

    // TODO: 回复"空帧数量", releaseFrame时维护计数. 待改进.
    ret = xos_sem_put(& self->sema_producerEmptyFrames);
    IC_ASSERT(XOS_OK == ret);

    xos_log_write(Producer_waitFrame_Give,
                 (uint32_t) sem,
                 (uint32_t) sem->count);

    return IC_OK;
}

#include <xtensa/tie/xt_debug.h>
extern void XT_BREAK(immediate imms, immediate immt);

int
ICStream_Producer_acquireFrame(ICStream *self, void **out_frame)
{
    // TODO: ringbuf必须为frame整数倍
    struct ICStreamShare *share = self->share;

    ICStreamFifo *q = &(share->fifo);
    int nContEmptySlots = fifo_lavail(q);

    int nAvailEmptyFifoUnit = fifo_avail(&(share->fifo));

    if (nContEmptySlots < (share->fifoUnitCountPerFrame)) {

        xos_log_write(Producer_acquireFrame,
              (uint32_t) nContEmptySlots,
              (uint32_t) nAvailEmptyFifoUnit);

        xos_log_disable();

        XT_BREAK(1, 1);

        return IC_ERROR;
    }

    *out_frame = (void *) q->tail.l;

    xos_log_write(Producer_acquireFrame,
              (uint32_t) nContEmptySlots/(share->in_fifoUnitCount),
              (uint32_t) nAvailEmptyFifoUnit/(share->in_fifoUnitCount));

    return IC_OK;
}

// in_frame供调试校验，等于head
int
ICStream_Producer_releaseFrame(ICStream *self, void *in_frame)
{
    // TODO: check params
    struct ICStreamShare *share = self->share;

    XosSem* sem = & self->sema_producerEmptyFrames;

    ICStreamFifo *q = &(share->fifo);

    // 如果in_frame不是tail.l, 则race condition发生了, IC_ASSERT
    IC_ASSERT(in_frame == q->tail.l); // l that they do own!!!

    // writeback dcache: 新帧的数据, 就是write指针所指. 单位: bytes
    // 单帧数据区起止/大小非cacheline对齐的影响: xthal已经内部处理cache line对齐.
    // TODO: 区别dma和cpu写入, 仅当cpu写入才需要.
    // TODO: 每个单帧刷出, 似乎比较粗暴, 待改进.
    xthal_dcache_region_writeback((void *) q->tail.l,
                                  share->in_fifoUnitCount);

    int requestedUnits = share->fifoUnitCountPerFrame;
    fifo_swallow_slots (q, requestedUnits);

    int32_t ret;
    // 维护"空帧数量", 空帧数量 -= 1, 此处必然成功, 不会阻塞
    ret = xos_sem_tryget(& self->sema_producerEmptyFrames);

    xos_log_write(Producer_releaseFrame_Take,
                  (uint32_t) sem,
                  (uint32_t) sem->count);

    IC_ASSERT(XOS_OK == ret);

    return IC_OK;
}

int
ICStream_Producer_commitRemote(ICStream *self)
{
    struct ICStreamShare *share = self->share;

    ICStreamFifo *q = &(share->fifo);

    // 确保: releaseFrame中的新数据帧, 已经刷出cache
    // memory barrier: write_release
#pragma flush_memory

    // writeback dcache: write|tail_index
    xthal_dcache_region_writeback((void *) &(q->tail_index.l),
                                  IC_DCACHELINE_ROUNDUP_SIZE(sizeof(FIFO_SIZE_T)));

    // memory barrier: write_release
#pragma flush_memory

    // push的方式发出server端通知, 对端已经注册push服务, 解除对端的可能阻塞.
    // notify的增量不能被对端用作处理的次数, 比如dataFramesIncrement=2,
    // 可能对端在wait_frame前已经处理了1帧, 共享内存中变量更新的原因, wait唤醒后,
    // 虽然dataFramesIncrement=2, 但是再处理1帧即可.

    // CLOGD("%s\n", __FUNCTION__);

    // 写入: obj ID. 参数定义: [0~4); [9] = objID;
    // [0~4), 基于已经记录的push消息模板, 填充emptyFramesIncrement = 1
    write_u32((uint8_t *)(& (self->push_frame_template.rpc.payload[0])), 1);

    // objID不变, 已经填在模板

    // 使用push注册时记录的frame信息, 发出通知
    int8_t urpc_ret = urpc_push_event_server(RPC_Server_stub, & self->push_frame_template);
    IC_ASSERT(URPC_SUCCESS == urpc_ret);

    return IC_OK;
}

static uint8_t _frame_session_prev = 0xff;
int g_debug_dup_session = 0;

// 运行于: rpc后台线程
// 空帧增加handler, ap->cp, 接收ap的RPC调用, 并通知给app任务上下文的流对象.
// rpc类型: async. client端的async handler空函数, client发通知后不阻塞.
static void
_Producer_emptyFramesIncrement_handler(urpc_frame *frame)
{
    IC_ASSERT(NULL != frame);

    // 读取: 增量空帧数. 参数定义: [0~4) = uint32;  [9] = objID;
    uint32_t emptyFrame = read_u32((uint8_t *)(& (frame->rpc.payload[0])));
    int objID = frame->rpc.payload[9];

    // CLOGD("%s: %d\n", __FUNCTION__, increment);

    int32_t ret;
    // 唤醒"空帧数量信号量"
    ret = xos_sem_put(& _ICStream_objs[objID]->sema_producerEmptyFrames);
    IC_ASSERT(XOS_OK == ret);

    XosSem* sem = & _ICStream_objs[objID]->sema_producerEmptyFrames;
    xos_log_write(EmptyFramesIncrement_push_responser_Give,
                  (uint32_t) emptyFrame,
                  (uint32_t) (sem->count << 16 | frame->header.session));

    // 检测重复session
    uint8_t _frame_session = frame->header.session;
    if (_frame_session == _frame_session_prev) {
        g_debug_dup_session ++;
    }
    _frame_session_prev = _frame_session;

    // 复用frame以回复async rpc, 忽略返回值
    uint8_t id;
    id = frame->rpc.dst_id;
    frame->rpc.dst_id = frame->rpc.src_id;
    frame->rpc.src_id = id;

    // objID不变

    int8_t urpc_ret = urpc_send_async_server(RPC_Server_stub, frame);
    IC_ASSERT(URPC_SUCCESS == urpc_ret);
}

// 运行于: rpc后台线程
// consumer端同步时注册到本对象的push服务, ap->cp, 仅调用一次.
// rpc类型: push
static void
_Producer_dataFramesIncrement_registerPush_handler(urpc_frame* frame)
{
    uint8_t id;
    CLOGD("%s\n", __FUNCTION__);

    // 读取. 参数定义: [9] = objID
    int objID = frame->rpc.payload[9];

    // 暂存在rpc frame模板, 留待主动push时使用
    _ICStream_objs[objID]->push_frame_template.header = frame->header;
    // 互换目标与源
    _ICStream_objs[objID]->push_frame_template.rpc.dst_id = frame->rpc.src_id;
    _ICStream_objs[objID]->push_frame_template.rpc.src_id = frame->rpc.dst_id;

    // objID记录在模板
    _ICStream_objs[objID]->push_frame_template.rpc.payload[9] = (uint8_t) objID;

    int32_t ret;
    // 对端consumer同步时, 注册到本端producer的"可用帧增加_push服务".
    // 以此为对象级的核间同步标志.
    ret = xos_sem_put(& _ICStream_objs[objID]->sema_producerSyncPoint);
    IC_ASSERT(XOS_OK == ret);
}

// app线程使用, 对象级的核间同步
int
ICStream_Producer_syncWithConsumer(ICStream *self)
{
    int32_t ret;

    // 以此为对象级的核间同步标志.
    ret = xos_sem_get(& self->sema_producerSyncPoint);
    IC_ASSERT(XOS_OK == ret);

    return IC_OK;
}

// uint32_t
// ICStream_getObjSize(void)
// {
//     // 取共享结构的大小, 不需要取对象实例的大小, 对象实例已经cacheline安全.
//     return sizeof(struct ICStreamShare);
// }

// \------------------------------------------------------------
// consumer
// #include "venus_log.h"

int
ICStream_Consumer_fetchRemote(ICStream *self)
{
    ICStreamFifo *q = & (self->share->fifo);

    // 从内存更新write(tail_index), read-only version
    // invalidate cache
    xthal_dcache_region_invalidate((void *) &(q->tail_index.r),
                                    IC_DCACHELINE_ROUNDUP_SIZE(sizeof(FIFO_SIZE_T)));

    // memory barrier: read_acquire
#pragma flush_memory

    // 后续读取的write指针将是共享内存中的最新值

    // read指针仍然使用本端cache的

    // TODO: 此时, 仅仅invalidate write指针所指额1帧可能不够, 可能对端的write指针跨越了多帧,
    // 则需要对这多帧都invalidate. 所以还是在consumer releaseFrame处invalidate 1帧稳妥.
    // 这样, 经由对端写入数据, 再轮转回来时, 自然是提前invalidate过的.

    // 后续看见的数据是对端最新产生的.

    return IC_OK;
}

// (非连续)可用空间的大小, 按sample[c0, c1, c2...]计数
int32_t
ICStream_Consumer_availSampleCount(ICStream *self)
{
    struct ICStreamShare *share = self->share;

    // 可用总空间(非连续)的大小
    int nAvailDataFifoUnit = fifo_len(&(share->fifo));

    int32_t nAvailSampleCount = nAvailDataFifoUnit / share->fifoUnitCountPerSample;

    return nAvailSampleCount;
}

bool
ICStream_Consumer_isEmpty(ICStream *self)
{
    struct ICStreamShare *share = self->share;

    // 可用总空间(非连续)的大小
    int nAvailDataFifoUnit = fifo_len(&(share->fifo));

    // 可用总空间(非连续)至少够1帧
    if (nAvailDataFifoUnit >= (share->out_fifoUnitCount)) {
        // 确认是连续空间
        int availContDataFifoUnit = fifo_lused(& share->fifo);
        IC_ASSERT(availContDataFifoUnit >= (share->out_fifoUnitCount));

        xos_log_write(Consumer_isEmpty,
                      (uint32_t) 0,
                      (uint32_t) nAvailDataFifoUnit/(share->out_fifoUnitCount));

        // 则isEmpty不成立
        return false;
    }

    // CLOGD("Consumer_isEmpty: %d r %d head: %p tail: %p", nAvailDataFifoUnit/(share->out_fifoUnitCount),
    //     nAvailDataFifoUnit % (share->out_fifoUnitCount), share->fifo.head.l, share->fifo.tail.r);

    xos_log_write(Consumer_isEmpty,
                  (uint32_t) 1,
                  (uint32_t) nAvailDataFifoUnit/(share->out_fifoUnitCount));

    // 可用总空间少于1 frame, 则isEmpty成立
    return true;
}

// 运行于: app线程. 由远端经由rpc handler唤醒
// 等待空帧出现, 如果已经有空帧, 则立即返回, 否则阻塞等待让出调度
int
ICStream_Consumer_waitFrame(ICStream *self)
{
    int32_t ret;
    XosSem* sem = & self->sema_consumerDataFrames;

    // 等待"数据帧数量" > 0
    ret = xos_sem_get(& self->sema_consumerDataFrames);
    IC_ASSERT(XOS_OK == ret);

    xos_log_write(Consumer_waitFrame_Get,
             (uint32_t) sem,
             (uint32_t) sem->count);

    // TODO: 恢复"数据帧数量", releaseFrame时维护计数. 待改进.
    ret = xos_sem_put(& self->sema_consumerDataFrames);
    IC_ASSERT(XOS_OK == ret);

    xos_log_write(Consumer_waitFrame_Put,
                 (uint32_t) sem,
                 (uint32_t) sem->count);

    return IC_OK;
}

int
ICStream_Consumer_acquireFrame(ICStream *self, void **out_frame)
{
    struct ICStreamShare *share = self->share;

    ICStreamFifo *q = & share->fifo;
    int availContUnits = fifo_lused(q);

    if (availContUnits < (share->fifoUnitCountPerFrame)) {

        xos_log_write(Consumer_acquireFrame,
                      (uint32_t) 0,
                      (uint32_t) 0);

        // 可用总空间(非连续)的大小
        int nAvailDataFifoUnit = fifo_len(& share->fifo);
        CLOGD("Consumer frames: %d r %d head: %p tail: %p", nAvailDataFifoUnit/(share->out_fifoUnitCount),
            nAvailDataFifoUnit % (share->out_fifoUnitCount), share->fifo.head.l, share->fifo.tail.r);

        // 不足1 frame, 则
        return IC_ERROR;
    }

    *out_frame = (void *)(q->head.l);

    // 可用总空间(非连续)的大小
    int nAvailDataFifoUnit = fifo_len(& share->fifo);
    // CLOGD("Consumer frames: %d r %d head: %p tail: %p", nAvailDataFifoUnit/(share->out_fifoUnitCount),
    //     nAvailDataFifoUnit % (share->out_fifoUnitCount), share->fifo.head.l, share->fifo.tail.r);

    xos_log_write(Consumer_acquireFrame,
                  (uint32_t) nAvailDataFifoUnit/(share->out_fifoUnitCount),
                  (uint32_t) 0);

    return IC_OK;
}

// in_frame供调试校验，等于head
int
ICStream_Consumer_releaseFrame(ICStream *self, void *in_frame)
{
    struct ICStreamShare *share = self->share;

    ICStreamFifo *q = & share->fifo;

    // 如果in_frame不是head.l, 则race condition发生了, IC_ASSERT
    IC_ASSERT(in_frame == q->head.l); // l that they do own!!!

    // 本端放弃该数据帧所有权, 已用帧的数据区交给producer, 从本端cache invalidate.
    // 以避免, 当对端产生数据时, 本端cache意外flush到共享内存.
    // 单帧数据区起止/大小非cacheline对齐的影响: xthal已经内部处理cache line对齐.
    // TODO: 区别dma和cpu写入, 仅当cpu写入才需要. 用户自己做吧
    xthal_dcache_region_invalidate((void *) q->head.l,
                                    share->out_fifoUnitCount);

    // synchronization & memory barrier: read_acquire.
    // 延后至commitFrame进行

    // rotate fifo, 更新read指针(head)
    fifo_drop(q, share->fifoUnitCountPerFrame);

    int32_t ret;
    // 维护"数据帧数量", 数据帧数量 -= 1
    // 异步用法下, 信号量与实际'数据帧数量'不一致. 可能无法-1.
    ret = xos_sem_tryget(& self->sema_consumerDataFrames);
//    IC_ASSERT(XOS_OK == ret);

    XosSem* sem = & self->sema_consumerDataFrames;
    xos_log_write(Consumer_releaseFrame_TryGet,
             (uint32_t) sem,
             (uint32_t) sem->count);

    // if ((XOS_OK == ret) != true) {
    //     // 记录本失败, 作为最后一个log
    //     write_syslog(12345, 54321);

    //     // disable syslog, 打印
    //     Trace_printAll();


    //     while(1);
    // }

    return IC_OK;
}

int
ICStream_Consumer_commitRemote(ICStream *self)
{
    struct ICStreamShare *share = self->share;

    ICStreamFifo *q = &(share->fifo);

    // 确保: Consumer_releaseFrame中的已用帧数据区, 已经通过invalidate放弃
    // data synchronization barrier: DSB()
#pragma flush_memory

    // writeback dcache: read_index
    xthal_dcache_region_writeback((void *) &(q->head_index.l),
                                    IC_DCACHELINE_ROUNDUP_SIZE(sizeof(FIFO_SIZE_T)));

    // synchronization & memory barrier: write_release.
#pragma flush_memory

    // push的方式发出server端通知, 对端已经注册push服务, 解除对端的可能阻塞.
    // 通知的空帧增量不能被对端用作处理的次数, 比如emptyFramesIncrement=2,
    // 可能对端在wait_frame前已经处理了1帧, 共享内存中变量更新的原因, wait唤醒后,
    // 虽然emptyFramesIncrement=2, 但是再处理1帧即可.

    // CLOGD("%s\n", __FUNCTION__);

    // 约定参数在: [0~4), uint32, 空帧增量 = 1; [9] = objID;
    // 基于已经记录的push消息模板, 填充emptyFramesIncrement = 1
    write_u32((uint8_t *)(& (self->consumer_push_frame_template.rpc.payload[0])), 1);
    self->consumer_push_frame_template.rpc.payload[9] = (uint8_t) self->ID;

    // 使用push注册时记录的frame信息, 发出通知
    int8_t urpc_ret = urpc_push_event_server(RPC_Server_stub, & self->consumer_push_frame_template);
    IC_ASSERT(URPC_SUCCESS == urpc_ret);

    ++ICStream_Consumer_commitRemote_urpc_push_Count;

    return IC_OK;
}

// 运行于: rpc后台线程
// 数据帧增加handler, ap->cp, 接收ap的RPC调用, 并通知给app任务上下文的流对象.
// rpc类型: async. client端的async handler空函数, client发通知后不阻塞.
static void
_Consumer_dataFramesIncrement_handler(urpc_frame * frame)
{
    IC_ASSERT(NULL != frame);

    // 读取: 增量帧数. 参数定义: [0~4) = uint32;  [9] = objID;
    int increment = read_u32((uint8_t *)(& (frame->rpc.payload[0])));
    int objID = frame->rpc.payload[9];

    // CLOGD("%s: %d\n", __FUNCTION__, increment);

    int32_t ret;
    // 唤醒"数据帧数量"信号量
    ret = xos_sem_put(& _ICStream_objs[objID]->sema_consumerDataFrames);
    // 非mutex, holder不同也通过.
    IC_ASSERT(XOS_OK == ret);

    XosSem* sem = & _ICStream_objs[objID]->sema_consumerDataFrames;
    xos_log_write(DataFramesIncrement_handler_Put,
             (uint32_t) sem,
             (uint32_t) sem->count);

    // 复用frame以回复async rpc, 忽略返回值
    uint8_t id;
    id = frame->rpc.dst_id;
    frame->rpc.dst_id = frame->rpc.src_id;
    frame->rpc.src_id = id;

    int8_t urpc_ret = urpc_send_async_server(RPC_Server_stub, frame);
    IC_ASSERT(URPC_SUCCESS == urpc_ret);

    ++Consumer_dataFramesIncrement_handler_Count;
}

// 运行于: rpc后台线程
// 对端producer同步时注册到本对象的push服务, ap->cp, 仅调用一次.
// rpc类型: push
static void
_Consumer_emptyFramesIncrement_registerPush_handler(urpc_frame* frame)
{
    uint8_t id;
    CLOGD("%s\n", __FUNCTION__);

    // 读取. 参数定义: [9] = objID
    int objID = frame->rpc.payload[9];

    // 暂存在rpc frame模板, 留待主动push时使用
    _ICStream_objs[objID]->consumer_push_frame_template.header = frame->header;
    // 互换目标与源
    _ICStream_objs[objID]->consumer_push_frame_template.rpc.dst_id = frame->rpc.src_id;
    _ICStream_objs[objID]->consumer_push_frame_template.rpc.src_id = frame->rpc.dst_id;

    // objID记录在模板
    _ICStream_objs[objID]->consumer_push_frame_template.rpc.payload[9] = objID;

    int32_t ret;
    // 对端producer同步时, 注册到本端consumer的"空帧增加_push服务".
    // 以此为对象级的核间同步标志.
    ret = xos_sem_put(& _ICStream_objs[objID]->sema_consumerSyncPoint);
    IC_ASSERT(XOS_OK == ret);
}

// app线程使用, 对象级的核间同步
int
ICStream_Consumer_syncWithProducer(ICStream *self)
{
    int32_t ret;

    // 以此为对象级的核间同步标志.
    ret = xos_sem_get(& self->sema_consumerSyncPoint);
    IC_ASSERT(XOS_OK == ret);

    return IC_OK;
}

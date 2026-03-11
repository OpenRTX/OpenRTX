// 本模块
#include "ic_stream.h"

#include "ic_common.h"

// 其他模块
#include "urpc_api.h"
#include "rpc_client.h"
#include "venus_log.h"

#include "cache.h"

#include <cmsis_gcc.h>

// 标准库
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#undef xos_log_write
#define xos_log_write(param1, param2, param3)

//------------------------------------------------
int Producer_dataFramesIncrement_async_responser_Count = 0;
int ICStream_Producer_commitRemote_Count = 0;
int Producer_emptyFramesIncrement_push_responser_Count = 0;

int ICStream_Producer_waitFrame_entry = 0;
int ICStream_Producer_waitFrame_exit = 0;
int ICStream_Producer_releaseFrame_entry = 0;
int ICStream_Producer_releaseFrame_exit = 0;
int ICStream_Producer_commitRemote_entry = 0;
int ICStream_Producer_commitRemote_exit = 0;
int Producer_dataFramesIncrement_async_responser_entry = 0;
int Producer_dataFramesIncrement_async_responser_exit = 0;
//------------------------------------------------
#define INSIDE( m, x, M) ((m <= x) && (x < M))

// TODO: 与远程通信相关的操作似乎可以抽出在通信基类?

// \------------------------------------------------------------
// 类方法, 二次分发至对象

static bool _ICStreamClass_inited = false;

#define ICSTREAM_OBJS_NUM 4

// 类全体对象跟踪, 用于远程会话的管理, 会话ID用于索引对象
static ICStream *_ICStream_objs[ICSTREAM_OBJS_NUM];

static void _icstream_err_handle(urpc_frame* frame);

// 数据帧增加的push handler, ap<-cp.
static void _Consumer_dataFramesIncrement_push_responser(urpc_frame * frame);
// 空帧增加的async response handler, ap->cp再ap<-cp时.
static void _Consumer_emptyFramesIncrement_async_responser(urpc_frame * frame);

// 空帧增加push handler, ap<-cp.
static void _Producer_emptyFramesIncrement_push_responser(urpc_frame * frame);
// 数据帧增加async response handler, ap->cp再ap<-cp时.
static void _Producer_dataFramesIncrement_async_responser(urpc_frame * frame);

// consumer/producer共用ep2, 共享对象. rpc handler注册结构
// TODO: 第0项将改为预定的错误处理, 需要让出.
static urpc_handle _rpc_client_ep2_handles[] = {
        {_icstream_err_handle,
            URPC_FLAG_RESPONSE | URPC_FLAG_ASYNC, URPC_DESC("ep2_e")},

        {_Consumer_dataFramesIncrement_push_responser,
            URPC_FLAG_PUSH | URPC_FLAG_RESPONSE, URPC_DESC("ep2_0")},
        {_Consumer_emptyFramesIncrement_async_responser,
            URPC_FLAG_ASYNC | URPC_FLAG_RESPONSE, URPC_DESC("ep2_1")},

        {_Producer_emptyFramesIncrement_push_responser,
            URPC_FLAG_PUSH | URPC_FLAG_RESPONSE, URPC_DESC("ep2_2")},
        {_Producer_dataFramesIncrement_async_responser,
            URPC_FLAG_ASYNC | URPC_FLAG_RESPONSE, URPC_DESC("ep2_3")}
};

static int s_bug_counter = 0;

static void
_icstream_err_handle(urpc_frame* frame)
{
    s_bug_counter ++;

    CLOGD("%s:%d", __FUNCTION__, frame->header.magic);
}

// \------------------------------------------------------------

// 实例的起始和大小都必须是cacheline对齐, 以免双核对同一cacheline的相邻变量不可控读写.

// TODO: 所有函数加assert

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

    // 初始化rpc client相关
    // 配置"核间stream服务"所需handler, 注册到rpc, 以便接收和发起远程调用.
    RPC_Client_eps[RPC_CLIENT_EP_2].handles = _rpc_client_ep2_handles;
    RPC_Client_eps[RPC_CLIENT_EP_2].handles_cnt =
        sizeof(_rpc_client_ep2_handles) / sizeof(urpc_handle);

    return 0;
}

// consumer------------------------------------------------------------

// 类静态函数, 在反序列化时构造对象.
// 从对端对象的共享部分, 构建本端的完整对象
ICStream *
ICStreamCreator_createObj(ICStream *addr, int objID, struct ICStreamShare *remoteShare, int mode)
{
    // 先类初始化
    _ICStream_Class_init();

    ICStream *self = addr;

    struct ICStreamShare *share = remoteShare;
    if (share == NULL) return NULL;

    // TBD: 抽出: ICStream_ctor(addr, remoteShare, mode);
    self->ID = objID;
    self->mode = mode;
    self->share = share;

    ICStreamFifo *q = &(share->fifo);
    CLOGD("size of ICStream: %d", sizeof(ICStream));
    CLOGD("size of ICStreamFifo: %d at %p %p %p", sizeof(ICStreamFifo),
            &(q->head.r), &(q->tail.r), &(q->size));

    //\------------------------------------------------------------
    // 远程对象机制

    // 加入远程会话跟踪
    _ICStream_objs[objID] = self;

    if (ICSTREAM_MODE_CONSUMER == mode) {
        // 本端先放弃所有数据帧所有权, 后续接到对端通知时, 直接读取就是最新值
        // TODO: 确认head就是完全的ring buffer, 改为base更好?
        //      如有预置数据, head更好? 绕回要处理.
        unsigned long ap_head = (unsigned long) (q->head.l) + self->share->apAddrOffset;
        // addr: aligned
        // size: may not aligned to cover entire range
        dcache_invalidate_range(ap_head,
                                ap_head + IC_DCACHELINE_ROUNDUP_SIZE(q->size));

        // 数据帧数信号量, 初始化0
        // 参数uxMaxCount == 实际最大帧数, 内存占用正比于此.
        // TODO: 考虑到re-config可能出现的最大帧数, 取可能的最大数.
        // 注意: 若改成binary可避免此耦合, 但binary不支持同步用法. 老旧代码兼容性考虑. 暂不动.
        self->sema_consumerDataFrames = lsf_sem_create_counting(self->share->frameCount_InternalBuf,
                                                                0);
        IC_ASSERT(NULL != self->sema_consumerDataFrames);
    }
    else if (ICSTREAM_MODE_PRODUCER == mode) {
        // 空帧数信号量, 初始化为最大空帧数
        // 参数uxMaxCount == 实际最大帧数, 内存占用正比于此.
        // TODO: 考虑到re-config可能出现的最大帧数, 取可能的最大数.
        // 注意: 若改成binary可避免此耦合, 但binary不支持同步用法. 老旧代码兼容性考虑. 暂不动.
        self->sema_producerEmptyFrames = lsf_sem_create_counting(self->share->frameCount_InternalBuf,
                                                                 self->share->frameCount_InternalBuf);
        IC_ASSERT(NULL != self->sema_producerEmptyFrames);
    }

    return self;
}

// 暂时用不到, 非正式实现
int
ICStream_dtor(ICStream *self)
{
    return IC_OK;
}

// 从本端detach, 不再使用. 但并不析构, 后续复用.
int ICStream_detach(ICStream *self)
{
    lsf_base_type ret;

    // 异步用法: 信号量不同步. 但仍尽量复位至预期值, 可能有异步并发+1.
    if (ICSTREAM_MODE_PRODUCER == self->mode) {
        // 大约 frameCount_InternalBuf 次.
        do {
            ret = lsf_sem_give(self->sema_producerEmptyFrames);
        } while (ret == lsf_task_pass);
    }
    else if (ICSTREAM_MODE_CONSUMER == self->mode) {
        // 大约 frameCount_InternalBuf 次.
        do {
            ret = lsf_sem_take(self->sema_consumerDataFrames, 0);
        } while (ret == lsf_task_pass);
    }

    // 数据帧已经invalidate

    // 控制区invalidate
    // arm: 可能write back; invalidate cache
    dcache_invalidate_range((unsigned long) self->share,
                            ((unsigned long) self->share) \
                            + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(struct ICStreamShare)));

    // synchronization & memory barrier: write_release. full system, any-any = 15
    // TODO: 因为不确定dcache invalidate算load还是store, 保险起见. any-any.
    __DSB();

    return 0;
}

// re-config, 更新config, 只改变传输参数, 其余不变.
// 约定: 参数: 有效的ICStream
int ICStream_reconfig(ICStream *self)
{
    // 重新调整信号量 == 帧数
    if (ICSTREAM_MODE_PRODUCER == self->mode) {
        lsf_base_type ret;

        lsf_u_base_type redisue = lsf_sem_get_count(self->sema_producerEmptyFrames);

        int difference = self->share->frameCount_InternalBuf - (int)redisue;

        while (difference > 0) {
            ret = lsf_sem_give(self->sema_producerEmptyFrames);
            IC_ASSERT(ret == lsf_task_pass);
            difference--;
        }

        while (difference < 0) {
            ret = lsf_sem_take(self->sema_producerEmptyFrames, 0);
            IC_ASSERT(ret == lsf_task_pass);
            difference++;
        }
    }
    // else consumer: nothing to do

    return 0;
}

//------------------------------------------------
// consumer模式: 实现

int
ICStream_Consumer_fetchRemote(ICStream *self)
{
    ICStreamFifo *q = & (self->share->fifo);

    // 从内存更新write|tail_index, read-only version
    // invalidate cache, q->tail独占IC_MAX_DCACHE_LINESIZE
    dcache_invalidate_range((unsigned long) &(q->tail_index.r),
                            ((unsigned long) &(q->tail_index.r)) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(FIFO_SIZE_T)));

    // synchronization & memory barrier: read_acquire. full system, any-any = 15
    // TODO: 因为不确定dcache invalidate算load还是store, 保险起见. any-any.
    __DSB();

    // 后续读取的write指针将是共享内存中的最新值

    // read指针仍然使用本端cache的

    // TODO: 此时, 仅仅invalidate write指针所指额1帧可能不够, 可能对端的write指针跨越了多帧,
    // 则需要对这多帧都invalidate. 所以还是在consumer releaseFrame处invalidate 1帧稳妥.
    // 这样, 经由对端写入数据, 再轮转回来时, 自然是提前invalidate过的.

    // 后续看见的数据是对端最新产生的.

    return IC_OK;
}

bool ICStream_Consumer_isEmpty(ICStream *self)
{
    struct ICStreamShare *share = self->share;

    // 可用总空间(非连续)的大小
    int nAvailDataFifoUnit = fifo_len(&(share->fifo));

    // 可用总空间(非连续)至少够1帧
    if (nAvailDataFifoUnit >= (share->out_fifoUnitCount)) {
        // 确认是连续空间
        int availContDataFifoUnit = fifo_lused(&(share->fifo));
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
// 等待数据帧出现, 如果已经有数据帧, 则立即返回, 否则阻塞等待让出调度
int
ICStream_Consumer_waitFrame(ICStream *self)
{
    lsf_base_type ret;
    lsf_sem_t sem = self->sema_consumerDataFrames;

    // 等待"数据帧数量" > 0
    ret = lsf_sem_take(self->sema_consumerDataFrames, lsf_max_delay);
    IC_ASSERT(lsf_task_pass == ret);

    xos_log_write(Consumer_waitFrame_Get,
             (uint32_t) sem,
             (uint32_t) lsf_sem_get_count(sem));

    // TODO: 恢复"数据帧数量", releaseFrame时维护计数. 待改进.
    ret = lsf_sem_give(self->sema_consumerDataFrames);
    IC_ASSERT(lsf_task_pass == ret);

    xos_log_write(Consumer_waitFrame_Put,
             (uint32_t) sem,
             (uint32_t) lsf_sem_get_count(sem));

    return IC_OK;
}

int
ICStream_Consumer_acquireFrame(ICStream *self, void **out_frame)
{
    struct ICStreamShare *share = self->share;

    ICStreamFifo *q = & (share->fifo);
    int availContUnits = fifo_lused(q);

    if (availContUnits < (share->fifoUnitCountPerFrame)) {

        xos_log_write(Consumer_acquireFrame,
              (uint32_t) 0,
              (uint32_t) 0);

        // 不足1 frame, 则
        return IC_ERROR;
    }

    unsigned long ap_head = (unsigned long) (q->head.l) + self->share->apAddrOffset;
    *out_frame = (void *)(ap_head);

    // 可用总空间(非连续)的大小
    int nAvailDataFifoUnit = fifo_len(&(share->fifo));
    // CLOGD("Consumer frames: %d r %d head: %p tail: %p", nAvailDataFifoUnit/(share->out_fifoUnitCount),
    //     nAvailDataFifoUnit % (share->out_fifoUnitCount), share->fifo.head.l, share->fifo.tail.r);

    xos_log_write(Consumer_acquireFrame,
                  (uint32_t) nAvailDataFifoUnit/(share->out_fifoUnitCount),
                  (uint32_t) ap_head);

    return IC_OK;
}

static uint32_t _emptyFrame;

int
ICStream_Consumer_releaseFrame(ICStream *self, void *in_frame)
{
    struct ICStreamShare *share = self->share;

    ICStreamFifo *q = &(share->fifo);

    unsigned long ap_head = (unsigned long) (q->head.l) + self->share->apAddrOffset;

    // 如果in_frame不是head.w, 则race condition发生了, IC_ASSERT
    IC_ASSERT(in_frame == (void *)ap_head); // l that they do own!!!

    // 本端放弃该数据帧所有权, 已用帧的数据区交给producer, 从本端cache invalidate.
    // 以避免, 当对端产生数据时, 本端cache意外flush到共享内存.
    // TODO: 区别dma和cpu写入, 仅当cpu写入才需要. 用户自己做吧

    // addr: may not aligned to cache line
    unsigned long aligned_head = (unsigned long) IC_DCACHELINE_ROUNDDOWN(ap_head);
    // size: may not aligned to cover entire range
    unsigned long aligned_size = share->out_fifoUnitCount;
    if (ap_head != aligned_head) {
        aligned_size += ap_head - aligned_head;
    }
    aligned_size = IC_DCACHELINE_ROUNDUP_SIZE(aligned_size);
    dcache_invalidate_range(aligned_head,
                            aligned_head + aligned_size);

    // 空帧 记录
    _emptyFrame = (uint32_t) q->head.l;

    // synchronization & memory barrier: read_acquire. full system, any-any = 15
    // 延后至commitFrame进行

    // rotate fifo, 更新read指针
    fifo_drop(q, share->fifoUnitCountPerFrame);

    lsf_base_type ret;
    // 维护"数据帧数量", 数据帧数量 -= 1.
    // 异步用法下, 信号量与实际'数据帧数量'不一致. 可能无法-1.
    ret = lsf_sem_take(self->sema_consumerDataFrames, 0);
    // IC_ASSERT(lsf_task_pass == ret);

    lsf_sem_t sem = self->sema_consumerDataFrames;
    xos_log_write(Consumer_releaseFrame_TryGet,
             (uint32_t) sem,
             (uint32_t) lsf_sem_get_count(sem));

    return IC_OK;
}

int
ICStream_Consumer_commitRemote(ICStream *self)
{
    struct ICStreamShare *share = self->share;

    ICStreamFifo *q = &(share->fifo);

    // 确保: releaseFrame中的已用帧数据区, 已经通过invalidate放弃
    // synchronization barrier. full system, any-any = 15
    __DSB();

    // writeback dcache: read_index
    dcache_clean_range((unsigned long) &(q->head_index.l),
                       ((unsigned long) &(q->head_index.l)) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(FIFO_SIZE_T)));

    // synchronization & memory barrier: write_release. full system, any-any = 15
    __DSB();

    // 通知server端
    // 通知的空帧增量不能被对端用作处理的次数, 比如emptyFramesIncrement=2,
    // 可能对端在wait_frame前已经处理了1帧, 共享内存中变量更新的原因, wait唤醒后,
    // 虽然emptyFramesIncrement=2, 但是再处理1帧即可.

    // CLOGD("%s\n", __FUNCTION__);

    urpc_frame buf;
    urpc_frame *frame = &buf;
    // 约定参数在: [0~3], uint32. 空帧增量 = 1; [9] = objID;
    // GET_UINT32(& (frame->rpc.payload[0])) = 1;

    // 发送 空帧 指针
    write_u32((uint8_t *)(& (frame->rpc.payload[0])), _emptyFrame);

    frame->rpc.payload[9] = (uint8_t) self->ID;

    frame->header.eps = RPC_SERVER_EP_2;
    frame->rpc.dst_id = 0+1;
    frame->rpc.src_id = 1+1;  // response handle id

    int8_t urpc_ret;
    do {
        urpc_ret = urpc_send_async_client(RPC_Client_stub, frame);
        if (URPC_SUCCESS != urpc_ret) {
            CLOGD("urpc_ret: %d\n", urpc_ret);
        }
    } while (URPC_SUCCESS != urpc_ret);
    // IC_ASSERT(URPC_SUCCESS == urpc_ret, "ret: %d \n", urpc_ret);

    return IC_OK;
}

// app线程使用
int
ICStream_Consumer_syncWithProducer(ICStream *self)
{
    // consumer端同步时注册到对端producer的 "可用帧增加_push服务"
    // 对端的"push注册_handler"调用完毕即意味着同步完成
    urpc_frame buf;
    urpc_frame *frame = &buf;
    CLOGD("%s", __FUNCTION__);

    // 约定参数在: [0~4), uint32, ?; [9] = objID;
    write_u32((uint8_t *)(& (frame->rpc.payload[0])), 2);

    frame->rpc.payload[9] = (uint8_t) self->ID;

    frame->header.eps = RPC_SERVER_EP_2;
    frame->rpc.dst_id = 1+1;
    frame->rpc.src_id = 0+1;  // response handle id

    int8_t urpc_ret = urpc_open_event_client(RPC_Client_stub, frame);
    IC_ASSERT(URPC_SUCCESS == urpc_ret, "ret: %d \n", urpc_ret);

    // 一般而言, 初始化过程, 然后再回复确认, 双方才可正式开始协作, 以免不同步
    // 但简单场景下, 暂时省略回复确认
    // RPC_阻塞发送(AP的确认);

    return IC_OK;
}

// 数据帧增加的push handler, ap<-cp, 接收RPC调用, 并通知给app任务上下文的流对象.
// 运行于: rpc后台task
// rpc类型: push.
static void
_Consumer_dataFramesIncrement_push_responser(urpc_frame * frame)
{
    (void) frame;

    // 读取: 增量数据帧数. 参数定义: [0~4) = ...; [9] = objID;
    int increment = read_u32((uint8_t *)(& (frame->rpc.payload[0])));
    int objID = frame->rpc.payload[9];
    // CLOGD("%s: %d\n", __FUNCTION__, increment);

    lsf_base_type ret;
    // 唤醒"数据帧数量信号量"
    ret = lsf_sem_give(_ICStream_objs[objID]->sema_consumerDataFrames);
    // 非mutex, holder不同也通过.
    IC_ASSERT(lsf_task_pass == ret);

    lsf_sem_t sem = _ICStream_objs[objID]->sema_consumerDataFrames;
    xos_log_write(DataFramesIncrement_handler_Put,
             (uint32_t) sem,
             (uint32_t) lsf_sem_get_count(sem));
}

// 空帧增加的async response handler, ap<-cp
// 运行于: rpc后台task
// rpc类型: async response.
static void
_Consumer_emptyFramesIncrement_async_responser(urpc_frame * frame)
{
    // 无实际操作

    (void) frame;

    // CLOGD("%s\n", __FUNCTION__);
}

// producer------------------------------------------------------------
// #undef CLOGD
// #define CLOGD

int
ICStream_Producer_fetchRemote(ICStream *self)
{
    ICStreamFifo *q = & (self->share->fifo);

    // 从内存更新read|head_index, read-only version
    // invalidate cache
    dcache_invalidate_range((unsigned long) &(q->head_index.r),
                            ((unsigned long) &(q->head_index.r)) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(FIFO_SIZE_T)));

    // synchronization & memory barrier: read_acquire. full system, any-any = 15
    // TODO: 因为不确定dcache invalidate算load还是store, 保险起见. any-any.
    __DSB();

    // 后续读取的read指针将是共享内存中的最新值

    // write指针仍然使用本端cache的

    // 空帧的数据区不管, 对端已经交出所有权, 本端产生数据后flush至内存.
    // TODO: 要根据数据由dma或cpu产生, 也要从cache invalidate, 否则出错.

    return IC_OK;
}

// (非连续)可用空余的大小, 按sample[c0, c1, c2...]计数
int32_t
ICStream_Producer_emptySampleCount(ICStream *self)
{
    struct ICStreamShare *share = self->share;

    // 可用总空间(非连续)的大小
    int nAvailEmptyFifoUnit = fifo_avail(&(share->fifo));

    int32_t nEmptySampleCount = nAvailEmptyFifoUnit / share->fifoUnitCountPerSample;

    return nEmptySampleCount;
}

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
// 等待数据帧出现, 如果已经有数据帧, 则立即返回, 否则阻塞等待让出调度
// timeOut: 必须是有效值, [1 - max_int32] 毫秒,
int
ICStream_Producer_waitFrame_timeOut(ICStream *self, int timeOutMs)
{
    lsf_base_type ret;

    SemaphoreHandle_t sem = self->sema_producerEmptyFrames;

    // 等待"空帧数量" > 0
    ret = lsf_sem_take(self->sema_producerEmptyFrames, pdMS_TO_TICKS(timeOutMs));
    // 超时返回错误码
    if (ret != lsf_task_pass) {
        return IC_ERROR;
    }

    // 同步用法下, 回复"空帧数量", releaseFrame时维护计数.
    // 异步用法下, 信号量与实际"空帧数量"不同步, 逻辑上不需要.
    //      可能超过新建semaphore的uxMaxCount, 从而+1失败.
    //      预期+1不必然成功, 取消assert.
    ret = lsf_sem_give(self->sema_producerEmptyFrames);
    // IC_ASSERT(lsf_task_pass == ret);

    return IC_OK;
}

// 运行于: app线程. 由远端经由rpc handler唤醒
// 等待数据帧出现, 如果已经有数据帧, 则立即返回, 否则阻塞等待让出调度
int
ICStream_Producer_waitFrame(ICStream *self)
{
    lsf_base_type ret;

    ++ICStream_Producer_waitFrame_entry;

    lsf_sem_t sem = self->sema_producerEmptyFrames;

    // 等待"空帧数量" > 0
    ret = lsf_sem_take(self->sema_producerEmptyFrames, lsf_max_delay);
    IC_ASSERT(lsf_task_pass == ret);

    xos_log_write(Producer_waitFrame_Take,
             (uint32_t) sem,
             (uint32_t) lsf_sem_get_count(sem));

    // 同步用法下, 回复"空帧数量", releaseFrame时维护计数.
    // 异步用法下, 信号量与实际"空帧数量"不同步, 逻辑上不需要.
    //      可能超过新建semaphore的uxMaxCount, 从而+1失败.
    //      预期+1不必然成功, 取消assert.
    ret = lsf_sem_give(self->sema_producerEmptyFrames);
    // IC_ASSERT(lsf_task_pass == ret);

    xos_log_write(Producer_waitFrame_Give,
                 (uint32_t) sem,
                 (uint32_t) lsf_sem_get_count(sem));

    ++ICStream_Producer_waitFrame_exit;

    return IC_OK;
}

int
ICStream_Producer_acquireFrame(ICStream *self, void **out_frame)
{
    struct ICStreamShare *share = self->share;

    // TODO: ringbuf必须为frame整数倍
    ICStreamFifo *q = &(share->fifo);
    int nContEmptySlots = fifo_lavail(q);

    if (nContEmptySlots < (share->fifoUnitCountPerFrame)) {

        xos_log_write(Producer_acquireFrame,
              (uint32_t) 0,
              (uint32_t) 0);

        return IC_ERROR;
    }

    unsigned long ap_tail = (unsigned long) (q->tail.l) + self->share->apAddrOffset;
    *out_frame = (void *) (ap_tail);

    xos_log_write(Producer_acquireFrame,
              (uint32_t) nContEmptySlots/(share->in_fifoUnitCount),
              (uint32_t) 0);

    return IC_OK;
}

int
ICStream_Producer_releaseFrame(ICStream *self, void *in_frame)
{
    // TODO: check params
    ++ICStream_Producer_releaseFrame_entry;

    lsf_sem_t sem = self->sema_producerEmptyFrames;

    struct ICStreamShare *share = self->share;

    ICStreamFifo *q = &(share->fifo);

    // 如果in_frame不是tail.l, 则race condition发生了, IC_ASSERT
    unsigned long ap_tail = (unsigned long) (q->tail.l) + self->share->apAddrOffset;
    IC_ASSERT(in_frame == (void *)ap_tail); // that they do own!!!

    // writeback dcache: 新帧的数据, 就是write指针所指. 单位: bytes
    // TODO: 每个单帧刷出, 似乎比较粗暴, 待改进.

    // addr: may not aligned to cache line
    unsigned long aligned_tail = (unsigned long) IC_DCACHELINE_ROUNDDOWN(ap_tail);
    // size: may not aligned to cover entire range
    unsigned long aligned_size = share->in_fifoUnitCount;
    if (ap_tail != aligned_tail) {
        aligned_size += ap_tail - aligned_tail;
    }
    aligned_size = IC_DCACHELINE_ROUNDUP_SIZE(aligned_size);
    dcache_clean_range(aligned_tail,
                       aligned_tail + aligned_size);

    int requestedUnits = share->fifoUnitCountPerFrame;
    fifo_swallow_slots (q, requestedUnits);

    lsf_base_type ret;
    // 维护"空帧数量", 空帧数量 -= 1
    // 异步用法下, 信号量与实际'数据帧数量'不一致. 可能无法-1.
    ret = lsf_sem_take(self->sema_producerEmptyFrames, 0);

    xos_log_write(Producer_releaseFrame_Take,
         (uint32_t) sem,
         (uint32_t) lsf_sem_get_count(sem));

    // IC_ASSERT(lsf_task_pass == ret);

    ++ICStream_Producer_releaseFrame_exit;

    return IC_OK;
}

int
ICStream_Producer_commitRemote(ICStream *self)
{
    ++ICStream_Producer_commitRemote_entry;

    struct ICStreamShare *share = self->share;

    ICStreamFifo *q = &(share->fifo);

    // 确保: releaseFrame中的新数据帧, 已经刷出cache
    // memory barrier: write_release
    // synchronization barrier. full system, any-any = 15
    __DSB();

    // writeback dcache: write|tail_index
    dcache_clean_range((unsigned long) &(q->tail_index.l),
                       ((unsigned long) &(q->tail_index.l)) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(FIFO_SIZE_T)));

    // synchronization & memory barrier: write_release. full system, any-any = 15
    __DSB();

    // 通知server端
    // 通知的 数据帧增量 不能被对端用作处理的次数, 比如dataFramesIncrement=2,
    // 可能对端在wait_frame前已经处理了1帧, 共享内存中变量更新的原因, wait唤醒后,
    // 虽然dataFramesIncrement=2, 但是再处理1帧即可.

    // CLOGD("%s\n", __FUNCTION__);

    urpc_frame buf;
    urpc_frame *frame = & buf;
    // 约定参数在: [0~4) = uint32, 数据帧增量 = 1; [9] = objID;
    write_u32((uint8_t *)(& (frame->rpc.payload[0])), 1);

    frame->rpc.payload[9] = (uint8_t) self->ID;

    frame->header.eps = RPC_SERVER_EP_2;
    frame->rpc.dst_id = 2+1;
    frame->rpc.src_id = 3+1;  // response handle id

    int8_t urpc_ret;
    do {
        urpc_ret = urpc_send_async_client(RPC_Client_stub, frame);
        if (URPC_SUCCESS != urpc_ret) {
            CLOGD("urpc_ret: %d\n", urpc_ret);
        }
    } while (URPC_SUCCESS != urpc_ret);
    // IC_ASSERT(URPC_SUCCESS == urpc_ret, "ret: %d \n", urpc_ret);

    ++ICStream_Producer_commitRemote_Count;

    ++ICStream_Producer_commitRemote_exit;

    return IC_OK;
}

// app线程使用
int
ICStream_Producer_syncWithConsumer(ICStream *self)
{
    // producer端同步时注册到对端consumer的 "空帧增加_push服务"
    // 对端的"push注册_handler"调用完毕即意味着同步完成
    urpc_frame buf;
    urpc_frame *frame = &buf;
    CLOGD("%s", __FUNCTION__);

    // 约定参数在: [0~4) = uint32, ?; [9] = objID;
    write_u32((uint8_t *)(& (frame->rpc.payload[0])), 2);
    frame->rpc.payload[9] = (uint8_t) self->ID;

    frame->header.eps = RPC_SERVER_EP_2;
    frame->rpc.dst_id = 3+1;
    frame->rpc.src_id = 2+1;  // response handle id

    int8_t urpc_ret = urpc_open_event_client(RPC_Client_stub, frame);
    IC_ASSERT(URPC_SUCCESS == urpc_ret, "ret: %d \n", urpc_ret);

    // 一般而言, 初始化过程, 然后再回复确认, 双方才可正式开始协作, 以免不同步
    // 但简单场景下, 暂时省略回复确认
    // RPC_阻塞发送(AP的确认);

    return IC_OK;
}

// 空帧增加push handler, ap<-cp, 接收RPC调用, 并通知给app任务上下文的流对象.
// 运行于: rpc后台task
// rpc类型: push.
static void
_Producer_emptyFramesIncrement_push_responser(urpc_frame * frame)
{
    (void) frame;

    ++Producer_dataFramesIncrement_async_responser_entry;

    // 读取: 增量空帧数. 参数定义: [0~4) = ...; [9] = objID;
    int increment = read_u32((uint8_t *)(& (frame->rpc.payload[0])));
    int objID = frame->rpc.payload[9];
    // CLOGD("%s: %d\n", __FUNCTION__, increment);

    lsf_base_type ret;
    // 唤醒"空帧数量信号量"
    // 异步用法下, 信号量与实际"空帧数量"不同步, 逻辑上不需要.
    //      可能超过新建semaphore的uxMaxCount, 从而+1失败.
    //      预期+1不必然成功, 取消assert.
    ret = lsf_sem_give(_ICStream_objs[objID]->sema_producerEmptyFrames);

    lsf_sem_t sem = _ICStream_objs[objID]->sema_producerEmptyFrames;
    xos_log_write(EmptyFramesIncrement_push_responser_Give,
             (uint32_t) sem,
             (uint32_t) lsf_sem_get_count(sem));

    // 非mutex, holder不同也通过.
    // IC_ASSERT(lsf_task_pass == ret);

    ++Producer_emptyFramesIncrement_push_responser_Count;

    ++Producer_dataFramesIncrement_async_responser_exit;
}

// 数据帧增加的async response handler, ap<-cp
// 运行于: rpc后台task
// rpc类型: async response.
static void
_Producer_dataFramesIncrement_async_responser(urpc_frame * frame)
{
    // 无实际操作

    (void) frame;

    ++Producer_dataFramesIncrement_async_responser_Count;

    // CLOGD("%s\n", __FUNCTION__);
}

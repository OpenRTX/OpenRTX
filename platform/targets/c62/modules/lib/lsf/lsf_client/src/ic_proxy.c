#include "ic_common.h"
#include "ic_proxy.h"

#include "ic_stream.h"
#include "ic_fence.h"
#include "ic_allocator.h"

#include "rpc_client.h"
#include "urpc_api.h"

#include "cache.h"
#include "venus_log.h"

#include <cmsis_gcc.h>

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// \------------------------------------------------------------
// 单核Proxy部分, 暂时这样实现, 所以只允许一个对象实例

// \------------------------------------------------------------

// 其实不需要handler, 仅仅为了endpoint 1有内容, 否则, 似乎不符合用法: ep1 <-> ep1.
static void
_Proxy_dummy_handler(urpc_frame* frame);

static void
_Proxy_err_handle(urpc_frame* frame);

// ep1, proxy. rpc handler注册用
// 第0项改为预定的错误处理, 让出.
static urpc_handle _rpc_client_ep1_handles[] = {
        {_Proxy_err_handle, URPC_FLAG_RESPONSE | URPC_FLAG_ASYNC, URPC_DESC("ep1_e")},
        {_Proxy_dummy_handler, URPC_FLAG_RESPONSE | URPC_FLAG_ASYNC, URPC_DESC("ep1_0")},
};

// 初始化Proxy, 初始化内置rpc机制, 注册到rpc
void
IC_Proxy_init(void)
{
    // 配置"核间stream服务"所需handler, 注册到rpc, 以便接收和发起远程调用.
    RPC_Client_eps[RPC_CLIENT_EP_1].handles = _rpc_client_ep1_handles;
    RPC_Client_eps[RPC_CLIENT_EP_1].handles_cnt =
        sizeof(_rpc_client_ep1_handles) / sizeof(urpc_handle);

    return;
}

// 获取远程对象, 并由此构造出完整对象, 需要配合以类静态方法完成对象构造. (TODO: 反序列化?)
ICStream *
IC_Proxy_getRemoteICStream(ICStream *addr, int requestObjID)
{
    void * streamShare = NULL;
    uint32_t streamShareSize;
    int remoteStreamMode;
    int streamMode;

    urpc_frame buf;
    urpc_frame *frame = &buf;

    CLOGD("%s\n", __FUNCTION__);

    // 请求的ICStream对象ID. 参数定义: 起始0的uint8.
    frame->rpc.payload[0] = (uint8_t) requestObjID;
    // 对端endpoint == 本端响应endpoint
    frame->header.eps = RPC_SERVER_EP_1;
    // 对端handler序号
    frame->rpc.dst_id = 0+1;
    // 本端响应的handler序号, not used in sync send
    frame->rpc.src_id = 0+1;

    urpc_trans_sync_client(RPC_Client_stub, frame, frame);

    // 读取: ICStreamShare地址. 参数定义: 起始0的uint32.
    streamShare = (void *) read_u32((uint8_t *)(& (frame->rpc.payload[0])));
    // 读取: 对象大小(bytes). 参数定义: 起始4的uint32.
    streamShareSize = read_u32((uint8_t *)(& (frame->rpc.payload[4])));
    // 读取: 对象mode(consumer/producer). 参数定义: 起始8的uint8.
    remoteStreamMode = frame->rpc.payload[8];
    // 读取: 对象ID(consumer/producer). 参数定义: 起始9的uint8.
    int remoteObjID = frame->rpc.payload[9];

    CLOGD("ICStream: 0x%p, %d bytes\n", streamShare, streamShareSize);

    // TODO: invalidate cache, 确保后续读取到的对象同共享内存中一致.
    dcache_invalidate_range((unsigned long) streamShare,
                            ((unsigned long) streamShare) + streamShareSize);

    // synchronization barrier. full system, any-any = 15
    // TODO: 因为不确定dcache invalidate算load还是store, 保险起见. any-any.
    // 一般的, 系统寄存器的操作, 后接dsb.
    __DSB();

    // 与对端互补
    streamMode = (ICSTREAM_MODE_CONSUMER == remoteStreamMode) ?
                    ICSTREAM_MODE_PRODUCER : ICSTREAM_MODE_CONSUMER;

    // 从对端对象的共享部分, 构建本端的完整对象
    // 区别创建producer/consumer
    return ICStreamCreator_createObj(addr, remoteObjID,
                                     (struct ICStreamShare *) streamShare,
                                     streamMode);
}

// 获取远程对象, 并由此构造出完整对象, 需要配合以类静态方法完成对象构造. (TODO: 反序列化?)
IC_Allocator *IC_Proxy_getRemoteICAllocator(IC_Allocator *addr, int requestObjID)
{
    void * mpool = NULL;
    int apAddrOffset;
    uint32_t mpoolObjSize;
    int remoteMode;
    int mode;

    urpc_frame buf;
    urpc_frame *frame = &buf;

    CLOGD("[API] %s\n", __FUNCTION__);

    // 请求的对象ID. 参数定义: 起始0的uint8.
    frame->rpc.payload[0] = (uint8_t) requestObjID;
    // 对端endpoint == 本端响应endpoint
    frame->header.eps = RPC_SERVER_EP_1;
    // 对端handler序号
#define IC_PROXY_ALLOCATOR_HANDLER (3)
    frame->rpc.dst_id = IC_PROXY_ALLOCATOR_HANDLER;
    // 本端响应的handler序号, not used in sync send
    frame->rpc.src_id = 0+1;

    urpc_trans_sync_client(RPC_Client_stub, frame, frame);

    // 读取: mpool地址. 参数定义: 起始0的uint32.
    mpool = (void *) read_u32((uint8_t *)(& (frame->rpc.payload[0])));
    // 读取: mpool buf地址的偏移. 参数定义: 起始4的uint32.
    apAddrOffset = (int32_t) read_u32((uint8_t *)(& (frame->rpc.payload[4])));
    // 读取: allocator对象mode(alloc/free). 参数定义: 起始8的uint8.
    remoteMode = frame->rpc.payload[8];
    // 读取: mpool对象实例大小(bytes). 参数定义: 起始9的uint8.
    mpoolObjSize = frame->rpc.payload[9];

    CLOGD("[%s] mpool obj: 0x%p, %d bytes\n", __FUNCTION__, mpool, mpoolObjSize);

    // invalidate cache, 确保后续读取到的对象同共享内存中一致.
    dcache_invalidate_range((unsigned long) mpool,
                            ((unsigned long) mpool) + mpoolObjSize);

    // synchronization barrier. full system
    __DSB();

    // 与对端互补
    mode = (IC_ALLOCATOR_MODE_FREE == remoteMode) ?
                    IC_ALLOCATOR_MODE_ALLOC : IC_ALLOCATOR_MODE_FREE;

    // 从对端对象的共享部分, 构建本端的完整对象
    return IC_Allocator_ctor(addr, mpool, mode, apAddrOffset);
}

// 获取远程对象, 并由此构造出完整对象, 需要配合以类静态方法完成对象构造. (TODO: 反序列化?)
// 此处对象: ICFence
void *
IC_Proxy_getRemoteFence(int requestObjID)
{
    int remoteObjID;

    urpc_frame buf;
    urpc_frame *frame = &buf;

    CLOGD("%s\n", __FUNCTION__);

    // 请求的Fence对象ID. 参数定义: 起始0的uint32.
    write_u32((uint8_t *)(& (frame->rpc.payload[0])), requestObjID);

    // 对端: _Service_FenceObjPub_handler, ep1_1
    // 对端endpoint == 本端响应endpoint
    frame->header.eps = RPC_SERVER_EP_1;
    // 对端handler序号
    frame->rpc.dst_id = 1+1;
    // 本端响应的handler序号, not used in sync send
    // 暂时用0号dummy handler, 实际不被调用
    frame->rpc.src_id = 0+1;

    // 阻塞直到对端结果返回
    urpc_trans_sync_client(RPC_Client_stub, frame, frame);

    // 读取: 对端对象ID. 参数定义: 起始0的uint32.
    remoteObjID = read_u32((uint8_t *)(& (frame->rpc.payload[0])));

    CLOGD("ICFence: ID %d\n", remoteObjID);

    // synchronization barrier. full system, any-any = 15
    // TODO: 因为不确定dcache invalidate算load还是store, 保险起见. any-any.
    // 一般的, 系统寄存器的操作, 后接dsb.
    __DSB();

    // 基于预先约定的ID, 构建本端的完整对象. TODO:
    return (void *) ICFence_Creator_createObj(remoteObjID);
}

static void
_Proxy_dummy_handler(urpc_frame* frame)
{
    (void) frame;

    CLOGD("[api] %s", __FUNCTION__);
}

static void
_Proxy_err_handle(urpc_frame* frame)
{
    CLOGD("[api] %s", __FUNCTION__);
}

#include "ic_common.h"
#include "ic_service.h"

#include "ic_fence.h"
#include "ic_stream.h"
#include "ic_allocator.h"

#include "rpc_server.h"
#include "urpc_api.h"

#include "venus_log.h"

// 平台
#include <xtensa/hal.h>

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// \------------------------------------------------------------

// TODO: 统一对象发布的注册机制, 不再针对具体类

// stream对象发布服务的handler, ap->cp
static void
_Service_ObjPub_handler(urpc_frame * frame);

// Fence对象发布服务的handler, ap->cp
static void
_Service_FenceObjPub_handler(urpc_frame * frame);

// Allocator对象发布服务的handler, ap->cp
static void
_Service_AllocatorObjPub_handler(urpc_frame * frame);

static void
_Service_err_handle(urpc_frame* frame);

// ep1, service. rpc handler注册用
// 第0项为预定的错误处理.
static urpc_handle _rpc_server_ep1_handles[] = {
        {_Service_err_handle, URPC_FLAG_REQUEST | URPC_FLAG_ASYNC, URPC_DESC("ep1_e")},
        {_Service_ObjPub_handler, URPC_FLAG_REQUEST | URPC_FLAG_SYNC, URPC_DESC("ep1_1")},
        {_Service_FenceObjPub_handler, URPC_FLAG_REQUEST | URPC_FLAG_SYNC, URPC_DESC("ep1_2")},
        {_Service_AllocatorObjPub_handler, URPC_FLAG_REQUEST | URPC_FLAG_SYNC, URPC_DESC("ep1_3")},
};

// 初始化服务, 共享对象放入
void
IC_Service_init(void)
{
    // 配置"核间stream服务"所需handler, 注册到rpc, 以便接收和发起远程调用.
    RPC_Server_eps[RPC_SERVER_EP_1].handles = _rpc_server_ep1_handles;
    RPC_Server_eps[RPC_SERVER_EP_1].handles_cnt =
        sizeof(_rpc_server_ep1_handles) / sizeof(urpc_handle);

    return;
}

// 对象由各自class统一管理, 根据ID取用, 临到请求时才刷入共享内存
// 运行于: rpc后台线程
// 接收到ap侧调用时, 将已注册的stream共享对象的地址发出, 包含了r/w指针和缓冲区地址.
// 对象发布服务的handler, ap->cp, rpc类型: sync.
static void
_Service_ObjPub_handler(urpc_frame * frame)
{
    IC_ASSERT(NULL != frame);

    // 待发布的对象handle
    void *shareAddr;
    // 及其大小(bytes)
    uint32_t shareSize = sizeof(struct ICStreamShare);

    // 请求的ICStream对象ID. 参数定义: [0] uint8.
    int requestObjID = frame->rpc.payload[0];

    // 本端对应的ICStream对象ID
    ICStream *localObj = (void *) ICStream_Class_getObj(requestObjID);
    shareAddr = (void *) localObj->share;

    // ICStreamShare刷入共享内存
    xthal_dcache_region_writeback(shareAddr, shareSize);

    // synchronization barrier, 确保先写入, 再通知对端.
#pragma flush_memory

    // 写入: ICStreamShare地址. 参数定义: [0~4) uint32.
    write_u32((uint8_t *)(& (frame->rpc.payload[0])), (uint32_t) shareAddr);
    // 写入: 对象大小. 参数定义: [4~8) uint32.
    write_u32((uint8_t *)(& (frame->rpc.payload[4])), shareSize);
    // 写入: 对象mode(consumer/producer). 参数定义: [8] uint8.
    frame->rpc.payload[8] = (uint8_t) localObj->mode;
    // 写入: 对象ID. 参数定义: [9] uint8.
    frame->rpc.payload[9] = (uint8_t) localObj->ID;

    CLOGD("%s: 0x%p, %d bytes\n", __FUNCTION__, shareAddr, shareSize);

    // 对端发来时, 已经填好回应的handler号. 此处互换rpc目标和源地址即可.
    uint8_t id;
    id = frame->rpc.dst_id;
    frame->rpc.dst_id = frame->rpc.src_id;
    frame->rpc.src_id = id;

    // 发出
    urpc_send_sync_server(RPC_Server_stub, frame);
}

// Fence对象发布服务的handler, ap->cp, rpc类型: sync.
// 运行于: rpc后台线程
// 接收到ap侧调用时, 将已注册的Fence共享对象, 包含了对象ID号.
// TODO: 相当于对象序列化, 抽出公共接口.
static void
_Service_FenceObjPub_handler(urpc_frame * frame)
{
    IC_ASSERT(NULL != frame);

    // 共享对象写入共享内存

    // synchronization barrier, 确保之前的内存操作先写入, 再通知对端.
#pragma flush_memory

    // 对端请求的Fence对象ID. 参数定义: 起始0的uint32.
    int requestObjID = read_u32((uint8_t *)(& (frame->rpc.payload[0])));
    // 本端对应的Fence对象ID
    int localObjID = ICFence_Class_getObjID(requestObjID);

    // 回应: 本端对象的ID
    // 写入: 参数定义: 起始0的uint32.
    write_u32((uint8_t *)(& (frame->rpc.payload[0])), localObjID);

    CLOGD("%s: fence obj ID: %d", __FUNCTION__, localObjID);

    // 对端发起rpc时, response handler已经记录在frame中, 此处互换即可
    // 目前, 发回client, handler其实不会被调用.
    uint8_t id;
    id = frame->rpc.dst_id;
    frame->rpc.dst_id = frame->rpc.src_id;
    frame->rpc.src_id = id;

    // 发出
    urpc_send_sync_server(RPC_Server_stub, frame);
}

// 对象由各自class统一管理, 根据ID取用, 临到请求时才刷入共享内存
// 运行于: rpc后台线程
// 接收到ap侧调用时, 将已注册的allocator_pool的地址发出, 包含了r/w指针和缓冲区地址.
// 对象发布服务的handler, ap->cp, rpc类型: sync.
static void
_Service_AllocatorObjPub_handler(urpc_frame * frame)
{
    IC_ASSERT(NULL != frame);

    // 待发布的对象handle
    IC_Allocator_Pool *shareAddr;
    // 及其大小(bytes)
    uint32_t shareSize;

    // 请求的IC_Allocator对象ID. 参数定义: [0] uint8.
    int requestObjID = frame->rpc.payload[0];

    // 检索本端对应的IC_Allocator对象
    IC_Allocator *localObj = IC_Allocator_Class_getObj(requestObjID, & shareSize);
    shareAddr = (void *) localObj->pool;

    // IC_Allocator_Pool刷入共享内存
    xthal_dcache_region_writeback(shareAddr, shareSize);

    // synchronization barrier, 确保先写入, 再通知对端.
#pragma flush_memory

    // 写入: IC_Allocator_Pool地址. 参数定义: [0~4) uint32.
    write_u32((uint8_t *)(& (frame->rpc.payload[0])), (uint32_t) shareAddr);
    // 写入: apAddrOffset. 参数定义: [4~8) uint32.
    write_u32((uint8_t *)(& (frame->rpc.payload[4])), localObj->apAddrOffset);
    // 写入: 对象mode(alloc/free). 参数定义: [8] uint8.
    frame->rpc.payload[8] = (uint8_t) localObj->mode;
    // 写入: IC_Allocator_Pool对象大小. 参数定义: [9] uint8.
    frame->rpc.payload[9] = (uint8_t) shareSize;

    CLOGD("[%s] mpool obj: 0x%p, %d bytes\n", __FUNCTION__, shareAddr, shareSize);

    // 对端发来时, 已经填好回应的handler号. 此处互换rpc目标和源地址即可.
    uint8_t id;
    id = frame->rpc.dst_id;
    frame->rpc.dst_id = frame->rpc.src_id;
    frame->rpc.src_id = id;

    // 发出
    urpc_send_sync_server(RPC_Server_stub, frame);
}

static void
_Service_err_handle(urpc_frame* frame)
{
    CLOGD("%s:%d", __FUNCTION__, frame->header.magic);
}

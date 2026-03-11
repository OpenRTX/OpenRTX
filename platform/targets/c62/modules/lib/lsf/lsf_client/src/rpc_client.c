#include "rpc_client.h"

#include "venus_log.h"

#include "lsf_os_port_internal.h"

urpc_client_stub* RPC_Client_stub = NULL;

static void dummy_handle(urpc_frame* frame);

static void dummy_err_handle(urpc_frame* frame);

static urpc_handle dummy_handles[] = {
        {dummy_err_handle, URPC_FLAG_RESPONSE | URPC_FLAG_ASYNC, URPC_DESC("ep2_e")},
        {dummy_handle, URPC_FLAG_PUSH | URPC_FLAG_RESPONSE, URPC_DESC("ep2_0")},
        {dummy_handle, URPC_FLAG_ASYNC | URPC_FLAG_RESPONSE, URPC_DESC("ep2_1")},
        {dummy_handle, URPC_FLAG_PUSH | URPC_FLAG_RESPONSE, URPC_DESC("ep2_2")},
        {dummy_handle, URPC_FLAG_ASYNC | URPC_FLAG_RESPONSE, URPC_DESC("ep2_3")},
};

static urpc_handle dummy_handles_fence[] = {
        {dummy_err_handle, URPC_FLAG_RESPONSE | URPC_FLAG_ASYNC, URPC_DESC("ep3_e")},
        {dummy_handle, URPC_FLAG_PUSH | URPC_FLAG_RESPONSE, URPC_DESC("ep3_0")},
        {dummy_handle, URPC_FLAG_ASYNC | URPC_FLAG_RESPONSE, URPC_DESC("ep3_1")},
};

static urpc_handle dummy_handles_property[] = {
        {dummy_err_handle, URPC_FLAG_RESPONSE | URPC_FLAG_ASYNC, URPC_DESC("prop_e")},
        {dummy_handle, URPC_FLAG_ASYNC | URPC_FLAG_RESPONSE, URPC_DESC("prop_1")},
};

// 预留空槽, 在register-service和建立register-remoteObj时填充
urpc_endpoint RPC_Client_eps[14] = {
        //end point 0, reserve for RPC
        {NULL, 0, URPC_DESC("ctrl_s")},
        //end point 1
        {NULL, 0, URPC_DESC("proxy")},
        //end point 2. TODO: rpc运行前, 此处NULL可行? 假参数以通过参数检查.
        {dummy_handles, sizeof(dummy_handles)/sizeof(urpc_handle), URPC_DESC("icstream")},
        //end point 3. TODO: rpc运行前, 此处NULL可行? 假参数以通过参数检查.
        {dummy_handles_fence, sizeof(dummy_handles_fence)/sizeof(urpc_handle), URPC_DESC("icfence")},
        //end point 4. TODO: rpc运行前, 此处NULL可行? 假参数以通过参数检查.
        {dummy_handles_property, sizeof(dummy_handles_property)/sizeof(urpc_handle), URPC_DESC("prop")},
};

static urpc_cb rpc_client_cb = { RPC_Client_eps, sizeof(RPC_Client_eps)/sizeof(urpc_endpoint) };

// rpc初始化, handler结构在编译期已经初始化
// 似乎没什么操作, 仅仅语义完整性
void
RPC_Client_init(void)
{
    RPC_Client_stub = urpc_mbox_get_client_stub();
}

// 启动, 并与client同步.
// rpc所需的handler都已就绪, 初始化mailbox.
void
RPC_Client_Start(void)
{
    int8_t ret;

    // 该api的语义就是开始运行
    urpc_init_client(RPC_Client_stub, & rpc_client_cb);

    for(;;) {
        ret = urpc_connect(RPC_Client_stub);
        if (URPC_SUCCESS == ret) break;
    }

    CLOGD("urpc_accept succeed.\n");
}

static void
dummy_handle(urpc_frame* frame)
{
    (void) frame;
    CLOGD("[api] %s", __FUNCTION__);
}

static void
dummy_err_handle(urpc_frame* frame)
{
    CLOGD("%s:%d", __FUNCTION__, frame->header.magic);
}

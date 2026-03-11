#include "rpc_server.h"

#include "venus_log.h"

#include <xtensa/xos.h>


urpc_server_stub* RPC_Server_stub = NULL;

// 预留空槽, 在register-service和建立register-remoteObj时填充
urpc_endpoint RPC_Server_eps[14] = {
        //end point 0, reserve for RPC
        {NULL, 0, URPC_DESC("ctrl_s")},
        //end point 1
        {NULL, 0, URPC_DESC("service")},
        //end point 2
        {NULL, 0, URPC_DESC("stream")},
        //end point 3
        {NULL, 0, URPC_DESC("fence")},
        //end point 4
        {NULL, 0, URPC_DESC("prop")},
};

static urpc_cb rpc_server_cb = { RPC_Server_eps, sizeof(RPC_Server_eps)/sizeof(urpc_endpoint) };

// rpc初始化, mailbox其实在编译期已经初始化
// 似乎没什么操作, 仅仅语义完整性
void
RPC_Server_init(void)
{
    RPC_Server_stub = urpc_mbox_get_server_stub();
}

// 启动, 并与client同步.
// rpc所需的handler都已就绪, 初始化mailbox.
void
RPC_Server_Start()
{
    int8_t ret;

    // 该api的语义就是开始运行
    urpc_init_server(RPC_Server_stub, & rpc_server_cb);

    for(;;) {
        ret = urpc_accept(RPC_Server_stub);
        if (URPC_SUCCESS == ret) break;

        CLOGD("urpc_accept retry.\n");
    }

    CLOGD("urpc_accept succeed.\n");
}

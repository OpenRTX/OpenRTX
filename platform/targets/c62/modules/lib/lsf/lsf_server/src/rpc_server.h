#ifndef __RPC_SERVER_H__
#define __RPC_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "urpc_api.h"

#include <stdint.h>
#include <stddef.h>

#define RPC_SERVER_EP_1 1
#define RPC_SERVER_EP_2 2
#define RPC_SERVER_EP_3 3
#define RPC_SERVER_EP_PROPERTY 	(4)

// 声明供外部引用, 直接操作, 避免api封装
extern urpc_server_stub* RPC_Server_stub;
extern urpc_endpoint RPC_Server_eps[];

// 初始化以供挂接handler, 不启动.
void RPC_Server_init(void);

// 启动, 并与client同步.
void RPC_Server_Start(void);

#ifdef __cplusplus
}
#endif

#endif /* __RPC_SERVER_H__ */

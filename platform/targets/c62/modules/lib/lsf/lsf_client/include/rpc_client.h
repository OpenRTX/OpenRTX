#ifndef __RPC_CLIENT_H__
#define __RPC_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "urpc_api.h"

#include <stdint.h>
#include <stddef.h>

#ifndef RPC_CLIENT_EP_0
#define RPC_CLIENT_EP_0    0
#endif
#ifndef RPC_CLIENT_EP_1
#define RPC_CLIENT_EP_1    1
#endif
#ifndef RPC_CLIENT_EP_2
#define RPC_CLIENT_EP_2    2
#endif
#ifndef RPC_CLIENT_EP_3
#define RPC_CLIENT_EP_3    3
#endif
#ifndef RPC_CLIENT_EP_4
#define RPC_CLIENT_EP_4    4
#endif
#ifndef RPC_CLIENT_EP_5
#define RPC_CLIENT_EP_5    5
#endif
#ifndef RPC_CLIENT_EP_6
#define RPC_CLIENT_EP_6    6
#endif
#ifndef RPC_CLIENT_EP_7
#define RPC_CLIENT_EP_7    7
#endif
#ifndef RPC_CLIENT_EP_8
#define RPC_CLIENT_EP_8    8
#endif
#ifndef RPC_CLIENT_EP_9
#define RPC_CLIENT_EP_9    9
#endif
#ifndef RPC_CLIENT_EP_10
#define RPC_CLIENT_EP_10    10
#endif
#ifndef RPC_CLIENT_EP_11
#define RPC_CLIENT_EP_11    11
#endif
#ifndef RPC_CLIENT_EP_12
#define RPC_CLIENT_EP_12    12
#endif
#ifndef RPC_CLIENT_EP_13
#define RPC_CLIENT_EP_13    13
#endif
#ifndef RPC_CLIENT_EP_14
#define RPC_CLIENT_EP_14    14
#endif
#ifndef RPC_CLIENT_EP_15
#define RPC_CLIENT_EP_15    15
#endif


#ifndef RPC_SERVER_EP_0
#define RPC_SERVER_EP_0    0
#endif
#ifndef RPC_SERVER_EP_1
#define RPC_SERVER_EP_1    1
#endif
#ifndef RPC_SERVER_EP_2
#define RPC_SERVER_EP_2    2
#endif
#ifndef RPC_SERVER_EP_3
#define RPC_SERVER_EP_3    3
#endif
#ifndef RPC_SERVER_EP_4
#define RPC_SERVER_EP_4    4
#endif
#ifndef RPC_SERVER_EP_5
#define RPC_SERVER_EP_5    5
#endif
#ifndef RPC_SERVER_EP_6
#define RPC_SERVER_EP_6    6
#endif
#ifndef RPC_SERVER_EP_7
#define RPC_SERVER_EP_7    7
#endif
#ifndef RPC_SERVER_EP_8
#define RPC_SERVER_EP_8    8
#endif
#ifndef RPC_SERVER_EP_9
#define RPC_SERVER_EP_9    9
#endif
#ifndef RPC_SERVER_EP_10
#define RPC_SERVER_EP_10    10
#endif
#ifndef RPC_SERVER_EP_11
#define RPC_SERVER_EP_11    11
#endif
#ifndef RPC_SERVER_EP_12
#define RPC_SERVER_EP_12    12
#endif
#ifndef RPC_SERVER_EP_13
#define RPC_SERVER_EP_13    13
#endif
#ifndef RPC_SERVER_EP_14
#define RPC_SERVER_EP_14    14
#endif
#ifndef RPC_SERVER_EP_15
#define RPC_SERVER_EP_15    15
#endif


#define RPC_SERVER_EP_PROPERTY 	RPC_SERVER_EP_4
#define RPC_CLIENT_EP_PROPERTY 	RPC_CLIENT_EP_4

// 声明供外部引用, 直接操作, 避免api封装
extern urpc_client_stub* RPC_Client_stub;
extern urpc_endpoint RPC_Client_eps[];

// 初始化以供挂接handler, 不启动.
void RPC_Client_init(void);

// 启动, 并与Server同步.
void RPC_Client_Start(void);

typedef enum
{
	ap2cp_play_stream_id = 0,
    cp2ap_record_stream_id,
    cp2ap_scan_stream_id,
    cp2ap_xtts_stream_id,
    cp2ap_trans_stream_id,
    cp2ap_scan_adjust_stream_id,
} ic_stream_id_e;


#ifdef __cplusplus
}
#endif

#endif /* __RPC_CLIENT_H__ */

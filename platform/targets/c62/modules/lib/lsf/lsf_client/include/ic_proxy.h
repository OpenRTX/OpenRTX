#ifndef __IC_SERVICE_H__
#define __IC_SERVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ic_stream.h"
#include "ic_allocator.h"

// 初始化Proxy, 初始化内置rpc机制, 注册到rpc
void IC_Proxy_init(void);

// 获取远程对象
ICStream *IC_Proxy_getRemoteICStream(ICStream *addr, int requestObjID);

// 获取远程对象
IC_Allocator *IC_Proxy_getRemoteICAllocator(IC_Allocator *addr, int requestObjID);

void *IC_Proxy_getRemoteFence(int requestObjID);

#ifdef __cplusplus
}
#endif

#endif /* __IC_SERVICE_H__ */

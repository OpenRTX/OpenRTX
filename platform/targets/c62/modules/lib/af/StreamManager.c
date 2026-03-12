#include "StreamManager.h"

#include "ic_proxy.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//------------------------------------------------
// platform

// utils: log
//#define LOG_NDEBUG 0
#define LOG_TAG "StreamManager"
#include "venus_log.h"

// utils: ic_assert
//#define ASSERT_NDEBUG 0
#include "ic_common.h"

//------------------------------------------------

// \unittest_icstream_consumer---------------------------------------
// cacheable. __attribute__((section (".dram0.data"), aligned(32)))
// 以0x200c0000的64k ram 作为共享内存

#define OUTPUT_ICSTREAM_ID_BASE    (0)
#define INPUT_ICSTREAM_ID_BASE     (3)

#define STREAM_MANAGER_OUTPUT_COUNT     (2)
#define STREAM_MANAGER_INPUT_COUNT      (1)

// Output stream - producer
static ICStream s_icStream_Output[STREAM_MANAGER_OUTPUT_COUNT];

// Input stream - consumer
static ICStream s_icStream_Input[STREAM_MANAGER_INPUT_COUNT];

// \------------------------------------------------------------
// 核间数据通路连接

void StreamManager_connect(void)
{
    // 挂接到已经创建好的核间数据流通道上
    for (int i = 0; i < STREAM_MANAGER_OUTPUT_COUNT; ++i) {
        // 此时server端的ICStream对象已就绪
        IC_Proxy_getRemoteICStream(&s_icStream_Output[i], OUTPUT_ICSTREAM_ID_BASE + i);

        // 对象级的核间同步, 以确认对端已经就绪.
        ICStream_Producer_syncWithConsumer(&s_icStream_Output[i]);
    }

    // 挂接到已经创建好的核间数据流通道上
    for (int i = 0; i < STREAM_MANAGER_INPUT_COUNT; ++i) {
        // 此时server端的ICStream对象已就绪
        IC_Proxy_getRemoteICStream(&s_icStream_Input[i], INPUT_ICSTREAM_ID_BASE + i);

        // 对象级的核间同步, 以确认对端已经就绪.
        ICStream_Consumer_syncWithProducer(&s_icStream_Input[i]);
    }

    return;
}

ICStream* StreamManager_getStream(int streamType, int streamID)
{
    ICStream *result = NULL;

    if (streamType == STREAM_TYPE_OUTPUT) {
        IC_ASSERT((0 <= streamID) && (streamID < STREAM_MANAGER_OUTPUT_COUNT));

        result = &s_icStream_Output[streamID];
        ICStream_reconfig(result);

        return result;
    }

    if (STREAM_TYPE_INPUT == streamType) {
        IC_ASSERT((0 <= streamID) && (streamID < STREAM_MANAGER_INPUT_COUNT));

        return &s_icStream_Input[streamID];
    }

    return NULL;
}

int StreamManager_detachStream(ICStream* stream, int streamType, int streamID)
{
    (void) streamID;
    (void) streamType;

    return ICStream_detach(stream);
}

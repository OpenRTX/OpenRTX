#ifndef __IC_STREAM_H__
#define __IC_STREAM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ic_common.h"

// 平台
#include "lsf_os_port_internal.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

//------------------------------------------------------------
// ICStreamFifo FIFO配置
#define MACROS_TO_INLINES
#define STATIC_INLINE static inline
typedef int BOOL;
#define TRUE 1
#define FALSE 0
typedef int FIFO_SIZE_T;                // optional: some signed type
#define FIFO_T ICStreamFifo          // optional, name of queue type
typedef uint8_t FIFO_CELL_T;           // true unit in fifo = byte
#define FIFO_NO_DS
#include "fifo/fifo.h"
//------------------------------------------------------------

// 对象的核间共享部分, 用于传递对象时, 核间直接内存地址映射
struct ICStreamShare {
    ICStreamFifo fifo;
    int frameCount_InternalBuf;

    int cellSize;
    int channelCount;

    int sampleCountPerFrame;
    int frameCount_In;
    int frameCount_Out;

    // private
    int in_sampleCount; // frameCount_In * sampleCountPerFrame
    int out_sampleCount; // frameCount_Out * sampleCountPerFrame

    int fifoUnitCountPerSample;
    int fifoUnitCountPerFrame;

    int in_fifoUnitCount;
    int out_fifoUnitCount;

    int apAddrOffset;
};

enum {
    ICSTREAM_MODE_CONSUMER,
    ICSTREAM_MODE_PRODUCER,
};

typedef struct _tag_ICStream_Config
{
    int mode; // 0:consumer; 1:producer

    struct ICStreamShare *share;

    int cellSize; // in bytes
    int channelCount;
    int sampleCountPerFrame;
    int frameCount_In;
    int frameCount_Out;

    int frameCount_InternalBuf;
} ICStream_Config;

typedef struct _ICStream ICStream;

struct _ICStream {
    // 对象ID, 用来索引查找对象实例
    int ID;

    // consumer 或者 producer
    int mode;

    // 共享部分
    struct ICStreamShare *share;

    union {
        // 对象的单核consumer私有部分
        // 远程同步信号量: 数据帧数量. 本端consumer等待, 由对端通知唤醒.
        lsf_sem_t sema_consumerDataFrames;

        // 对象的单核producer私有部分
        // 远程同步信号量: 空帧数量. 本端producer等待, 由对端通知唤醒.
        lsf_sem_t sema_producerEmptyFrames;
    };
};

// 类静态函数, 在反序列化时构造对象.
ICStream *ICStreamCreator_createObj(ICStream *addr, int objID, struct ICStreamShare *remoteShare, int mode);

int ICStream_dtor(ICStream *self);

// 从本端detach, 不再使用. 但并不析构, 后续复用.
int ICStream_detach(ICStream *self);

// re-config, 更新config, 只改变传输参数, 其余不变.
// 约定: 参数: 有效的ICStream
int ICStream_reconfig(ICStream *self);

// Producer: 按frame------------------------------------------------------------

int ICStream_Producer_syncWithConsumer(ICStream *self);

int ICStream_Producer_fetchRemote(ICStream *self);

bool ICStream_Producer_isFull(ICStream *self);

// (非连续)可用空余的大小, 按sample[c0, c1, c2...]计数
int32_t ICStream_Producer_emptySampleCount(ICStream *self);

// timeOut: 必须是有效值, [1 - max_int32] 毫秒,
int ICStream_Producer_waitFrame_timeOut(ICStream *self, int timeOutMs);

int ICStream_Producer_waitFrame(ICStream *self);

int ICStream_Producer_acquireFrame(ICStream *self, void **out_frame);

// in_frame供调试校验，等于head
int ICStream_Producer_releaseFrame(ICStream *self, void *in_frame);

int ICStream_Producer_commitRemote(ICStream *self);

// Consumer: 按frame ------------------------------------------------------------

int ICStream_Consumer_syncWithProducer(ICStream *self);

int ICStream_Consumer_fetchRemote(ICStream *self);

bool ICStream_Consumer_isEmpty(ICStream *self);

int ICStream_Consumer_waitFrame(ICStream *self);

int ICStream_Consumer_acquireFrame(ICStream *self, void **out_frameBuf);

int ICStream_Consumer_releaseFrame(ICStream *self, void *in_buffer);

int ICStream_Consumer_commitRemote(ICStream *self);

#ifdef __cplusplus
}
#endif

#endif /* __IC_STREAM_H__ */

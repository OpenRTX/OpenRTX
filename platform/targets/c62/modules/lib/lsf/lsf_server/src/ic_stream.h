#ifndef __IC_STREAM_H__
#define __IC_STREAM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ic_common.h"

#include "urpc_api.h"

// 平台
#include <xtensa/hal.h>
#include <xtensa/config/core.h>
#include "xtensa/xos.h"

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
// 不能手动分配, 由ICStream构造时自动分配
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

    int apAddrOffset;
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
        // 对象的单核producer私有部分
        // 远程同步信号量: 空帧数量. 本端producer等待, 由对端通知唤醒.
        XosSem sema_producerEmptyFrames;

        // 对象的单核consumer私有部分
        // 远程同步信号量: 数据帧数量. 本端consumer等待, 由对端通知唤醒.
        XosSem sema_consumerDataFrames;
    };

    union {
        // 对象的单核producer私有部分
        // 远程同步信号量: 双端对象同步. 同步点后, 双端才开始运转.
        XosSem sema_producerSyncPoint;

        // 对象的单核consumer私有部分
        // 远程同步信号量: 双端对象同步. 同步点后, 双端才开始运转.
        XosSem sema_consumerSyncPoint;
    };

    union {
        // 对象的单核producer私有部分
        // 记录的rpc push消息模板
        urpc_frame push_frame_template;

        // 对象的单核consumer私有部分
        // 记录的rpc push消息模板
        urpc_frame consumer_push_frame_template;
    };
};

// 类静态方法
ICStream *ICStream_Class_getObj(int objID);

ICStream *ICStream_ctor(void *addr, ICStream_Config *config, int objID,
                             void *pcmBuffer, int pcmBufferLen, int mode);

int ICStream_dtor(ICStream *self);

// detach: 用户不再使用. 但并不析构, 保持urpc连接, 后续复用.
int ICStream_detach(ICStream *self);

// re-config, 只改变传输参数, 其余不变
// 约定: 参数: 有效的ICStream
int ICStream_reconfig(ICStream *self, ICStream_Config *config,
                      void *pcmBuffer, int pcmBufferLen);

// Producer: 按frame------------------------------------------------------------
int ICStream_Producer_syncWithConsumer(ICStream *self);

int ICStream_Producer_fetchRemote(ICStream *self);

bool ICStream_Producer_isFull(ICStream *self);

int ICStream_Producer_waitFrame(ICStream *self);

int ICStream_Producer_acquireFrame(ICStream *self, void **out_frame);

// in_frame供调试校验，等于head
int ICStream_Producer_releaseFrame(ICStream *self, void *in_frame);

int ICStream_Producer_commitRemote(ICStream *self);

// Consumer: 按frame ------------------------------------------------------------
int ICStream_Consumer_syncWithProducer(ICStream *self);

int ICStream_Consumer_fetchRemote(ICStream *self);

// 可用sample数(总计/非连续), 按sample[c0, c1, c2...]计数
int32_t ICStream_Consumer_availSampleCount(ICStream *self);

bool ICStream_Consumer_isEmpty(ICStream *self);

int ICStream_Consumer_waitFrame(ICStream *self);

int ICStream_Consumer_acquireFrame(ICStream *self, void **out_frameBuf);

int ICStream_Consumer_releaseFrame(ICStream *self, void *in_buffer);

int ICStream_Consumer_commitRemote(ICStream *self);

#ifdef __cplusplus
}
#endif

#endif /* __IC_STREAM_H__ */

#include "mem_src.h"

#include <stdio.h>
#include <string.h>


MEMSrc *
MEMSrc_ctor(void* addr, MEMSrc_Config *config)
{
    if (addr == NULL) {
        return NULL;
    }

    MEMSrc* self = (MEMSrc *) addr;

    self->data = config->data;
    self->dataSize = config->dataSize;
    self->frameSize = config->frameSize;

    // 内存固定分片, 总片数
    self->roundIndexMax = self->dataSize / config->frameSize;
    // 内存固定分片范围, 0 -> (总片数-1)
    self->roundIndex = 0;

    return self;
}

void
MEMSrc_dtor(MEMSrc *self)
{
    memset(self, 0, sizeof(*self));

    return;
}

// 从内存pcm块循环产生数据
int
MEMSrc_getNextFrame(MEMSrc *self, void **out_frame)
{
    // 已获取到最后一片了, 不允许继续获取
    if (self->roundIndex == self->roundIndexMax) {
        *out_frame = NULL;
        return 1;
    }

    // 读指标
    int readIndex = self->roundIndex;

    unsigned char *frame = self->data + (readIndex * self->frameSize);
    *out_frame = frame;

    self->roundIndex++;

    return 0;
}

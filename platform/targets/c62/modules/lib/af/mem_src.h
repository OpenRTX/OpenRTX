#ifndef _MEMSrc_H
#define _MEMSrc_H

typedef struct _MEMSrc_Config {
    unsigned char *data;
    int dataSize; // bytes

    int frameSize; // bytes
} MEMSrc_Config;

typedef struct _MEMSrc MEMSrc;

struct _MEMSrc
{
    unsigned char *data;
    int dataSize;
    int frameSize;

    int roundIndexMax;
    int roundIndex;
};

MEMSrc *
MEMSrc_ctor(void* addr, MEMSrc_Config *config);

// TODO: 使用前提, 确保源是frameSize对齐
// 获取1帧, 并推进读指针
// 返回值: 0, 成功; 1, 失败;
int
MEMSrc_getNextFrame(MEMSrc *self, void **out_frame);

// // 恢复初始状态, 可以从头getNextFrame
// int
// MEMSrc_reset(MEMSrc *self, void **out_frame);

void
MEMSrc_dtor(MEMSrc *self);


#endif // _MEMSrc_H

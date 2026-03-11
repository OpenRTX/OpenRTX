#ifndef _AUDIORECORD_H
#define _AUDIORECORD_H

#include "base.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "IAudioRecord.h"
#include "AudioTrackShared.h"
#include "ic_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AudioRecord AudioRecord;

struct AudioRecord {
    IAudioRecord*       mAudioRecord;

    ICStream*           mICStream;
    audio_track_cblk_t* mCblk;

    uint32_t            mFrameCount;

    uint32_t            mChannels;
    uint32_t            mStatus;

    void*               mStreamFrame;

    // FIXME: 暂时由AP侧负责通道数的转换，将来应当在CP侧完成
    uint8_t             mChannelOutIdx[6];
    uint32_t            mChannelOutCnt;
};

status_t    AudioRecord_ctor(   AudioRecord *self           ,
                                int inputSource             ,
                                uint32_t sampleRate         ,
                                int format                  ,
                                uint32_t channels           ,
                                int frameCount              ,
                                void* _reserved)            ;

status_t    AudioRecord_dtor(   AudioRecord *self);

status_t    AudioRecord_set(    AudioRecord *self           ,
                                int inputSource             ,
                                uint32_t sampleRate         ,
                                int format                  ,
                                uint32_t channels           ,
                                int frameCount              ,
                                void* _reserved)            ;

void        AudioRecord_start(AudioRecord *self);

void        AudioRecord_stop(AudioRecord *self);

ssize_t     AudioRecord_read(AudioRecord *self, void *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif  // _AUDIORECORD_H

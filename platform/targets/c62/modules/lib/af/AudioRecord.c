//------------------------------------------------
// self
#include "AudioRecord.h"

// family
#include "base.h"
#include "IAudioFlinger.h"
#include "AudioTrackShared.h"
#include "AudioTrack.h"

// package: misc
#include "IServiceManager.h"
#include "AudioSystem.h"
#include "Errors.h"

// package: stream
#include "StreamManager.h"

//------------------------------------------------
// module: ic_stream
#include "ic_stream.h"

//------------------------------------------------
// platform
#include "cache.h"

// utils: log
//#define LOG_NDEBUG 0
#define LOG_TAG "AudioTrack"
#include "venus_log.h"

//------------------------------------------------
// lib: clib

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

//------------------------------------------------
#define ASSERT(a) \
    if ((a) != true) { \
        CLOGD("ASSERT: %s", __func__); \
        while(1); \
    }

//------------------------------------------------
// private methods

status_t AudioRecord_openRecord(
        AudioRecord* self,
        uint32_t sampleRate,
        int format,
        int channelCount,
        int frameCount,
        uint32_t flags,
        audio_io_handle_t input);

//------------------------------------------------
// 对象

status_t    AudioRecord_ctor(   AudioRecord *self           ,
                                int inputSource             ,
                                uint32_t sampleRate         ,
                                int format                  ,
                                uint32_t channels           ,
                                int frameCount              ,
                                void* _reserved)
{
    self->mStatus = NO_INIT;

    self->mAudioRecord = NULL;
    self->mStatus = AudioRecord_set(self,
                                    inputSource,
                                    sampleRate,
                                    format,
                                    channels,
                                    frameCount,
                                    _reserved);

    return self->mStatus;
}

status_t    AudioRecord_set(    AudioRecord *self           ,
                                int inputSource             ,
                                uint32_t sampleRate         ,
                                int format                  ,
                                uint32_t channels           ,
                                int frameCount              ,
                                void* _reserved)
{
    uint32_t flags = 0;

    if (self->mAudioRecord != 0) {
        CLOGE("Record already in use");
        return INVALID_OPERATION;
    }

    // handle default values first.
    if (sampleRate == 0) {
        sampleRate = 16000;
    }
    if (format == 0) {
        format = PCM_16_BIT;
    }
    if (channels == 0) {
        channels = CHANNEL_IN_STEREO;
    }

    // TODO: 先粗略判断
    if (format != PCM_16_BIT) {
        CLOGE("Invalid format: only PCM_16_BIT is supported");
        return BAD_VALUE;
    }

    if (!AudioSystem_isInputChannel(channels)) {
        CLOGE("Unsupported input channel configuration: 0x%x", channels);
        return BAD_VALUE;
    }

    uint32_t channelCount = AudioSystem_popCount(channels);

    // FIXME: 预计算输出时需要复制的声道索引，暂时先在 AP 完成，将来移到 CP
    uint8_t chIdx = 0;
    if (channels & CHANNEL_IN_LEFT) {
        self->mChannelOutIdx[chIdx++] = 0;
    }
    if (channels & CHANNEL_IN_RIGHT) {
        self->mChannelOutIdx[chIdx++] = 1;
    }
    if (channels & CHANNEL_IN_AUX_LEFT) {
        self->mChannelOutIdx[chIdx++] = 2;
    }
    if (channels & CHANNEL_IN_AUX_RIGHT) {
        self->mChannelOutIdx[chIdx++] = 3;
    }
    if (channels & CHANNEL_IN_ECHO_LEFT) {
        self->mChannelOutIdx[chIdx++] = 4;
    }
    if (channels & CHANNEL_IN_ECHO_RIGHT) {
        self->mChannelOutIdx[chIdx++] = 5;
    }
    self->mChannelOutCnt = channelCount;

    audio_io_handle_t input = AudioSystem_getInput(inputSource,
            sampleRate, format, channels, flags);

    // create the IAudioRecord
    status_t status = AudioRecord_openRecord(self, sampleRate, format, channelCount,
                                             frameCount, flags, input);

    if (status != NO_ERROR) {
        return status;
    }

    self->mStatus = NO_ERROR;

    self->mChannels = channels;

    // 无帧
    self->mStreamFrame = NULL;

    return NO_ERROR;
}

status_t AudioRecord_openRecord(
        AudioRecord* self,
        uint32_t sampleRate,
        int format,
        int channelCount,
        int frameCount,
        uint32_t flags,
        audio_io_handle_t input)
{
    status_t status;
    IAudioFlinger* audioFlinger = AudioSystem_get_audio_flinger();

    if (audioFlinger == 0) {
        CLOGE("Could not get audioflinger");
        return NO_INIT;
    }

    IAudioRecord* record = audioFlinger->openRecord(audioFlinger,
                                                    0, // pid
                                                    input,
                                                    sampleRate,
                                                    format,
                                                    channelCount,
                                                    frameCount,
                                                    flags,
                                                    &status);

    if (record == 0) {
        CLOGE("AudioFlinger could not open record, sattus: %d", status);
        return status;
    }
    void* cblk = record->getCblk(record);
    if (cblk == 0) {
        CLOGE("Could not get control block");
        return NO_INIT;
    }

    self->mAudioRecord = record;
    self->mCblk = (audio_track_cblk_t*) cblk;

    dcache_invalidate_range((unsigned long) self->mCblk,
                            ((unsigned long) self->mCblk) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(audio_track_cblk_t)));

    __DSB();

    self->mICStream = StreamManager_getStream(STREAM_TYPE_INPUT, self->mCblk->streamID);

    self->mCblk->out = 0;

    dcache_clean_range((unsigned long) &(self->mCblk->out),
                       ((unsigned long) &(self->mCblk->out)) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(int)));

    __DSB();

    return NO_ERROR;
}

void AudioRecord_start(AudioRecord *self)
{
    IAudioRecord_start(self->mAudioRecord);
}

void AudioRecord_stop(AudioRecord *self)
{
    IAudioRecord_stop(self->mAudioRecord);
}

status_t AudioRecord_dtor(AudioRecord *self)
{
    if (self->mStatus == NO_ERROR) {
        AudioRecord_stop(self);

        if (self->mICStream != NULL) {
            StreamManager_detachStream(self->mICStream,
                                    STREAM_TYPE_INPUT,
                                    self->mCblk->streamID);
        }

        if (self->mCblk != NULL) {
            audio_track_cblk_t_detach(self->mCblk);
        }

        if (self->mAudioRecord != NULL) {
            RefBase_decStrong((RefBase*)self->mAudioRecord);
        }
    }

    return 0;
}

static inline uint32_t AudioRecord_copySamples(AudioRecord *self, int16_t *dst, int16_t *src, uint32_t frames)
{
    uint32_t targetChannels = self->mChannelOutCnt;
    uint32_t sourceChannels = self->mICStream->share->channelCount;

    for (uint32_t i = 0; i < frames; i++) {
        for (uint32_t ch = 0; ch < targetChannels; ch++) {
            dst[i * targetChannels + ch] = src[i * sourceChannels + self->mChannelOutIdx[ch]];
        }
    }

    return frames * targetChannels * sizeof(int16_t);
}

ssize_t AudioRecord_read(AudioRecord *self, void *buffer, size_t size)
{
    int ret;
    ssize_t read = 0;

    do {
        ICStream_Consumer_fetchRemote(self->mICStream);

        if (ICStream_Consumer_isEmpty(self->mICStream)) {
            ICStream_Consumer_waitFrame(self->mICStream);
            ICStream_Consumer_fetchRemote(self->mICStream);
        } else {
            ICStream_Consumer_waitFrame(self->mICStream);
        }

        ret = ICStream_Consumer_acquireFrame(self->mICStream, &self->mStreamFrame);
        if (ret != IC_OK) {
            CLOGE("ICStream_Consumer_acquireFrame failed: %d", ret);
            return ret;
        }

        // TODO: 暂时由AP侧负责通道数的转换，将来应当在CP侧完成
        int16_t *src = (int16_t *)self->mStreamFrame;
        int16_t *dst = (int16_t *)((uint8_t *)buffer + read);
        read += AudioRecord_copySamples(self, dst, src, self->mCblk->frameCount);

        ret = ICStream_Consumer_releaseFrame(self->mICStream, self->mStreamFrame);
        if (ret != IC_OK) {
            CLOGE("ICStream_Consumer_releaseFrame failed: %d", ret);
            return ret;
        }

        ret = ICStream_Consumer_commitRemote(self->mICStream);
        if (ret != IC_OK) {
            CLOGE("ICStream_Consumer_commitRemote failed: %d", ret);
            return ret;
        }
    } while (read < size);

    return read;
}

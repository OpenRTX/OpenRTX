//------------------------------------------------
// self
#include "AudioTrack.h"

// family
#include "base.h"
#include "IAudioFlinger.h"
#include "IAudioTrack.h"
#include "AudioTrackShared.h"

// package: misc
#include "IServiceManager.h"
#include "AudioSystem.h"
#include "Errors.h"

// package: stream
#include "StreamManager.h"

//------------------------------------------------
// module: ic_stream

#include "ic_stream.h"
#include "ic_proxy.h"
#include "rpc_client.h"

//------------------------------------------------
// platform
#include "cache.h"

// os
#include "FreeRTOS.h"
#include "queue.h"

// utils: log
//#define LOG_NDEBUG 0
#define LOG_TAG "AudioTrack"
#include "venus_log.h"

//------------------------------------------------
// lib: clib

#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <string.h>

//------------------------------------------------
#define ASSERT(a) \
    if ((a) != true) { \
        CLOGD("ASSERT: %s", __func__); \
        while(1); \
    }

//------------------------------------------------
// 约定: AAC帧
#define AAC_FRAME_COUNT (160)

//------------------------------------------------
// private methods

status_t AudioTrack_createTrack(
        AudioTrack* self,
        int streamType,
        uint32_t sampleRate,
        int format,
        int channelCount,
        int frameCount,
        uint32_t flags,
        void** sharedBuffer,
        audio_io_handle_t output);

//------------------------------------------------
// 状态

//------------------------------------------------
// 对象

status_t AudioTrack_ctor(   AudioTrack* self,
                            uint32_t sampleRate,
                            int format,
                            int channels,
                            int frameCount,
                            audio_offload_info_t* offload_info)
{
    self->mStatus = NO_INIT;

    self->mAudioTrack = NULL;

    self->mStatus = AudioTrack_set(self,
                                   sampleRate,
                                   format,
                                   channels,
                                   frameCount,
                                   offload_info);

    return self->mStatus;
}

status_t    AudioTrack_set( AudioTrack* self,
                            uint32_t sampleRate,
                            int format,
                            int channels,
                            int frameCount,
                            audio_offload_info_t* offload_info)
{
    int streamType = AudioSystem_DEFAULT;
    uint32_t flags = 0;
    void* sharedBuffer = NULL;

    if (self->mAudioTrack != 0) {
        CLOGE("Track already in use");
        return INVALID_OPERATION;
    }

    int afSampleRate;
    if (AudioSystem_getOutputSamplingRate(&afSampleRate, streamType) != NO_ERROR) {
        return NO_INIT;
    }
    int afFrameCount;
    if (AudioSystem_getOutputFrameCount(&afFrameCount, streamType) != NO_ERROR) {
        return NO_INIT;
    }
    uint32_t afLatency;
    if (AudioSystem_getOutputLatency(&afLatency, streamType) != NO_ERROR) {
        return NO_INIT;
    }

    // handle default values first.
    if (streamType == AudioSystem_DEFAULT) {
        streamType = AudioSystem_MUSIC;
    }
    if (sampleRate == 0) {
        sampleRate = afSampleRate;
    }
    // these below should probably come from the audioFlinger too...
    if (format == 0) {
        format = PCM_16_BIT;
    }

    if (channels == 0) {
        channels = CHANNEL_OUT_STEREO;
    }

    // validate parameters
    if (!AudioSystem_isValidFormat(format)) {
        CLOGE("Invalid format");
        return BAD_VALUE;
    }

    // force direct flag if format is not linear PCM
    if (!AudioSystem_isLinearPCM(format)) {
        flags |= AudioSystem_OUTPUT_FLAG_DIRECT;
    }

    if (!AudioSystem_isOutputChannel(channels)) {
        CLOGE("Invalid channel mask");
        return BAD_VALUE;
    }

    uint32_t channelCount = AudioSystem_popCount(channels);

    audio_io_handle_t output = AudioSystem_getOutput((AudioSystem_stream_type)streamType,
            sampleRate, format, channels, (AudioSystem_output_flags)flags);

    if (output == 0) {
        CLOGE("Could not get audio output for stream type %d", streamType);
        return BAD_VALUE;
    }

    if (!AudioSystem_isLinearPCM(format)) {
        // offload 约定: 160 bytes * 2 buffer
        frameCount = AAC_FRAME_COUNT * 2;
        channelCount = 1;
    } else {
        // Ensure that buffer depth covers at least audio hardware latency
        uint32_t minBufCount = afLatency / ((1000 * afFrameCount)/afSampleRate);
        if (minBufCount < 2) minBufCount = 2;

        int minFrameCount = (afFrameCount*sampleRate*minBufCount)/afSampleRate;

        if (sharedBuffer == 0) {
            if (frameCount == 0) {
                frameCount = minFrameCount;
            }
            if (frameCount < minFrameCount) {
                CLOGE("Invalid buffer size: minFrameCount %d, frameCount %d", minFrameCount, frameCount);
                return BAD_VALUE;
            }
        } else {
            // no static mode
        }
    }

    self->mVolume[AudioTrack_LEFT] = 1.0f;
    self->mVolume[AudioTrack_RIGHT] = 1.0f;

    // create the IAudioTrack
    status_t status = AudioTrack_createTrack(self, streamType, sampleRate, format, channelCount,
                                             frameCount, flags, &sharedBuffer, output);

    if (status != NO_ERROR) {
        return status;
    }

    self->mStatus = NO_ERROR;

    self->mStreamType = streamType;
    self->mFormat = format;
    self->mChannels = channels;
    self->mChannelCount = channelCount;
    self->mSharedBuffer = sharedBuffer;
    self->mMuted = false;
    self->mActive = ATOMIC_VAR_INIT(0);
    self->mLatency = afLatency + (1000*self->mFrameCount) / sampleRate;
    self->mFlags = flags;

    // 无帧
    self->mStreamFrame = NULL;
    self->mStreamFrameAvail = 0;

    return NO_ERROR;
}

status_t AudioTrack_createTrack(
        AudioTrack* self,
        int streamType,
        uint32_t sampleRate,
        int format,
        int channelCount,
        int frameCount,
        uint32_t flags,
        void** sharedBuffer,
        audio_io_handle_t output)
{
    status_t status;
    IAudioFlinger* audioFlinger = AudioSystem_get_audio_flinger();

    if (audioFlinger == 0) {
       CLOGE("Could not get audioflinger");
       return NO_INIT;
    }

    IAudioTrack* track = audioFlinger->createTrack(audioFlinger,
                                                   0, // pid
                                                   streamType,
                                                   sampleRate,
                                                   format,
                                                   channelCount,
                                                   frameCount,
                                                   ((uint16_t)flags) << 16,
                                                   sharedBuffer,
                                                   output,
                                                   &status);

    if (track == 0) {
        CLOGE("AudioFlinger could not create track, status: %d", status);
        return status;
    }
    void* cblk = track->getCblk(track);
    if (cblk == 0) {
        CLOGE("Could not get control block");
        return NO_INIT;
    }

    self->mAudioTrack = track;
    self->mCblk = (audio_track_cblk_t*) cblk;

    // 从内存更新read|head_index, read-only version
    // invalidate cache
    dcache_invalidate_range((unsigned long) self->mCblk,
                            ((unsigned long) self->mCblk) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(audio_track_cblk_t)));

    // synchronization & memory barrier: read_acquire. full system, any-any = 15
    // TODO: 因为不确定dcache invalidate算load还是store, 保险起见. any-any.
    __DSB();

    CLOGD("[%s] AudioTrack streamID: %d\r\n", __func__, self->mCblk->streamID);

    self->mICStream = StreamManager_getStream(STREAM_TYPE_OUTPUT, self->mCblk->streamID);

    // TODO: cblk的读写一致性

    self->mCblk->out = 1;

    // Update buffer size in case it has been limited by AudioFlinger during track creation
    self->mFrameCount = self->mCblk->frameCount;

    self->mCblk->volumeLR = ((int32_t)((int16_t)(self->mVolume[AudioTrack_LEFT] * 0x1000)) << 16) \
                            | (int16_t)(self->mVolume[AudioTrack_RIGHT] * 0x1000);
    self->mCblk->bufferTimeoutMs = MAX_STARTUP_TIMEOUT_MS;
    self->mCblk->waitTimeMs = 0;

    // synchronization barrier. full system, any-any = 15
    __DSB();

    // writeback dcache
    dcache_clean_range((unsigned long) &(self->mCblk->out),
                       ((unsigned long) &(self->mCblk->out)) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(int)));

    dcache_clean_range((unsigned long) &(self->mCblk->volumeLR),
                       ((unsigned long) &(self->mCblk->volumeLR)) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(int)));

    dcache_clean_range((unsigned long) &(self->mCblk->bufferTimeoutMs),
                       ((unsigned long) &(self->mCblk->bufferTimeoutMs)) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(int)));

    dcache_clean_range((unsigned long) &(self->mCblk->waitTimeMs),
                       ((unsigned long) &(self->mCblk->waitTimeMs)) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(int)));

    // synchronization & memory barrier: write_release. full system, any-any = 15
    __DSB();

    return NO_ERROR;
}

void AudioTrack_start(AudioTrack* self)
{
    CLOGD("start %p", self);

    if (atomic_fetch_or(&(self->mActive), 1) == 0) {
        self->mCblk->bufferTimeoutMs = MAX_STARTUP_TIMEOUT_MS;
        self->mCblk->waitTimeMs = 0;

        // synchronization barrier. full system, any-any = 15
        __DSB();
        // writeback dcache
        dcache_clean_range((unsigned long) &(self->mCblk->bufferTimeoutMs),
                           ((unsigned long) &(self->mCblk->bufferTimeoutMs)) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(int)));
        // synchronization & memory barrier: write_release. full system, any-any = 15
        __DSB();

        vTaskPrioritySet(xTaskGetCurrentTaskHandle(), THREAD_PRIORITY_AUDIO_CLIENT);

        status_t status = IAudioTrack_start(self->mAudioTrack);

        if (status != NO_ERROR) {
            CLOGE("start() failed");
            atomic_fetch_and(&(self->mActive), ~1);

            vTaskPrioritySet(xTaskGetCurrentTaskHandle(), DROID_PRIORITY_NORMAL);
        }
    }
}

void AudioTrack_stop(AudioTrack* self)
{
    int ret;

    CLOGD("stop %p", self);

    // 置位: 无帧
    if (self->mStreamFrame) {
        // 失败则返回出错码, 无关远程
        ret = ICStream_Producer_releaseFrame(self->mICStream, self->mStreamFrame);
        ASSERT(IC_OK == ret);

        // 发布最新状态至共享, 通信机制出错码
        ret = ICStream_Producer_commitRemote(self->mICStream);
        ASSERT(IC_OK == ret);

        // 无帧
        self->mStreamFrame = NULL;
        self->mStreamFrameAvail = 0;
    }

    if (atomic_fetch_and(&(self->mActive), ~1) == 1) {


        IAudioTrack_stop(self->mAudioTrack);

        vTaskPrioritySet(xTaskGetCurrentTaskHandle(), DROID_PRIORITY_NORMAL);
    }

    return;
}

bool AudioTrack_stopped(AudioTrack* self)
{
    return !atomic_load(&(self->mActive));
}

void AudioTrack_flush(AudioTrack* self)
{
    CLOGD("flush");

    if (!atomic_load(&(self->mActive))) {
        IAudioTrack_flush(self->mAudioTrack);
    }
}

void AudioTrack_pause(AudioTrack* self)
{
    CLOGD("pause");
    if (atomic_fetch_and(&(self->mActive), ~1) == 1) {
        IAudioTrack_pause(self->mAudioTrack);

        vTaskPrioritySet(xTaskGetCurrentTaskHandle(), DROID_PRIORITY_NORMAL);
    }

    return;
}

void AudioTrack_mute(AudioTrack* self, bool e)
{
    IAudioTrack_mute(self->mAudioTrack, e);
    self->mMuted = e;

    return;
}

bool AudioTrack_muted(AudioTrack* self)
{
    return self->mMuted;
}

void AudioTrack_setVolume(AudioTrack* self, float left, float right)
{
    if (left < 0.0f) left = 0.0f;
    if (right < 0.0f) right = 0.0f;

    self->mVolume[AudioTrack_LEFT] = left;
    self->mVolume[AudioTrack_RIGHT] = right;

    // 从内存更新read|head_index, read-only version
    // invalidate cache
    dcache_invalidate_range((unsigned long) &(self->mCblk->volumeLR),
                            ((unsigned long) &(self->mCblk->volumeLR)) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(uint32_t)));

    // synchronization & memory barrier: read_acquire. full system, any-any = 15
    // TODO: 因为不确定dcache invalidate算load还是store, 保险起见. any-any.
    __DSB();

    self->mCblk->volumeLR = ((int32_t)((int16_t)(self->mVolume[AudioTrack_LEFT] * 0x1000)) << 16) \
                            | (int16_t)(self->mVolume[AudioTrack_RIGHT] * 0x1000);

    // synchronization barrier. full system, any-any = 15
    __DSB();

    // writeback dcache
    dcache_clean_range((unsigned long) &(self->mCblk->volumeLR),
                       ((unsigned long) &(self->mCblk->volumeLR)) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(uint32_t)));

    // synchronization & memory barrier: write_release. full system, any-any = 15
    __DSB();

    return;
}

void AudioTrack_getVolume(AudioTrack* self, float* left, float* right)
{
    *left  = self->mVolume[AudioTrack_LEFT];
    *right = self->mVolume[AudioTrack_RIGHT];

    return;
}

status_t AudioTrack_dtor(AudioTrack* self)
{
    if (self->mStatus == NO_ERROR) {
        AudioTrack_stop(self);

        // detach, 流通路管理器
        if (self->mICStream != NULL) {
            StreamManager_detachStream(self->mICStream,
                                       STREAM_TYPE_OUTPUT,
                                       self->mCblk->streamID);
        }

        // 共享内存cblk: detach
        if (self->mCblk != NULL) {
            audio_track_cblk_t_detach(self->mCblk);
        }

        if (self->mAudioTrack != NULL) {
            // members clear
            RefBase_decStrong((RefBase*)self->mAudioTrack);
        }
    }

    // 对象逐级析构

    return 0;
}

int AudioTrack_format(AudioTrack* self)
{
    return self->mFormat;
}

int AudioTrack_channelCount(AudioTrack* self)
{
    return self->mChannelCount;
}

uint32_t AudioTrack_frameCount(AudioTrack* self)
{
    return self->mFrameCount;
}

int AudioTrack_frameSize(AudioTrack* self)
{
    if (AudioSystem_isLinearPCM(self->mFormat)) {
        return AudioTrack_channelCount(self) \
               * ((AudioTrack_format(self) == PCM_8_BIT) \
                  ? sizeof(uint8_t) : sizeof(int16_t));
    } else {
        return sizeof(uint8_t);
    }
}

ssize_t AudioTrack_write(AudioTrack* self, const void* buffer, size_t userSize)
{
    if (self->mSharedBuffer != 0) return INVALID_OPERATION;

    if ((ssize_t)(userSize) < 0) {
        // sanity-check. user is most-likely passing an error code.
        CLOGE("AudioTrack_write(buffer=%p, size=%u (%d)",
                buffer, userSize, userSize);
        return BAD_VALUE;
    }

    CLOGD("write %p: %d bytes, mActive=%d", self, userSize, atomic_load(&(self->mActive)));

    ssize_t written = 0;
    const int8_t *src = (const int8_t *)buffer;
    AudioTrack_Buffer audioBuffer;

    do {
        audioBuffer.frameCount = userSize/AudioTrack_frameSize(self);

        // Calling obtainBuffer() with a negative wait count causes
        // an (almost) infinite wait time.
        status_t err = AudioTrack_obtainBuffer(self, &audioBuffer, -1);
        if (err < 0) {
            // out of buffers, return #bytes written
            if (err == (status_t)NO_MORE_BUFFERS)
                break;
            return (ssize_t)(err);
        }

        size_t toWrite;

        toWrite = audioBuffer.size;
        memcpy(audioBuffer.i8, src, toWrite);
        src += toWrite;

        userSize -= toWrite;
        written += toWrite;

        AudioTrack_releaseBuffer(self, &audioBuffer);
    } while (userSize);

    return written;
}

status_t AudioTrack_obtainBuffer(AudioTrack* self, AudioTrack_Buffer* audioBuffer, int32_t waitCount)
{
    int ret;
    int active;
    status_t result;
    audio_track_cblk_t* cblk = self->mCblk;
    uint32_t framesReq = audioBuffer->frameCount;

    dcache_invalidate_range((unsigned long) &(cblk->bufferTimeoutMs),
                            ((unsigned long) &(cblk->bufferTimeoutMs)) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(int)));
    // synchronization & memory barrier: read_acquire. full system, any-any = 15
    // TODO: 因为不确定dcache invalidate算load还是store, 保险起见. any-any.
    __DSB();

    uint32_t waitTimeMs = (waitCount < 0) ? cblk->bufferTimeoutMs : WAIT_PERIOD_MS;

    audioBuffer->frameCount  = 0;
    audioBuffer->size = 0;

    uint32_t framesAvail;

    // 无帧
    if (self->mStreamFrame == NULL) {
        goto start_loop_here;
        while (self->mStreamFrame == 0) { // 无帧
            active = atomic_load(&(self->mActive));
            if (!active) {
                CLOGE("Not active and NO_MORE_BUFFERS");
                return NO_MORE_BUFFERS;
            }
            if (!waitCount) {
                return WOULD_BLOCK;
            }

            result = ICStream_Producer_waitFrame_timeOut(self->mICStream, waitTimeMs);
            if (result!=NO_ERROR) {
                cblk->waitTimeMs += waitTimeMs;
                if (cblk->waitTimeMs >= cblk->bufferTimeoutMs) {

                    CLOGD("obtainBuffer timed out (is the CPU pegged?) %p, mICStream=%p",
                          self, self->mICStream);

                    IAudioTrack_start(self->mAudioTrack);

                    cblk->waitTimeMs = 0;
                }

                if (--waitCount == 0) {
                    return TIMED_OUT;
                }
            }
            // read the server count again
        start_loop_here:
            // 获取最新的共享状态
            ICStream_Producer_fetchRemote(self->mICStream);

            // 先检查是否有空帧
            if (!ICStream_Producer_isFull(self->mICStream)) {
                // 失败则返回出错码, 无关远程
                ret = ICStream_Producer_acquireFrame(self->mICStream, & self->mStreamFrame);
                ASSERT(IC_OK == ret);

                // TBD: 160 samples:[ch0, ch1]
                self->mStreamFrameAvail = self->mICStream->share->sampleCountPerFrame;
            }
        }
    }

    cblk->waitTimeMs = 0;

    // 有帧, 余空
    framesAvail = self->mStreamFrameAvail;

    if (framesReq > framesAvail) {
        framesReq = framesAvail;
    }

    audioBuffer->flags = self->mMuted ? AudioTrack_Buffer_MUTE : 0;
    audioBuffer->channelCount = self->mChannelCount;
    audioBuffer->frameCount = framesReq;
    audioBuffer->size = framesReq * cblk->frameSize;
    if (AudioSystem_isLinearPCM(self->mFormat)) {
        audioBuffer->format = PCM_16_BIT;
    } else {
        audioBuffer->format = self->mFormat;
    }

    // offset:frames * frameSize => offset:bytes
    int offset_bytes = (self->mICStream->share->sampleCountPerFrame - self->mStreamFrameAvail) \
                       * cblk->frameSize;

    audioBuffer->raw = (int8_t *)self->mStreamFrame + offset_bytes;
    active = atomic_load(&(self->mActive));
    return active ? (status_t)(NO_ERROR) : (status_t)(STOPPED);
}

void AudioTrack_releaseBuffer(AudioTrack* self, AudioTrack_Buffer* audioBuffer)
{
    int ret;

    audio_track_cblk_t* cblk = self->mCblk;

    ASSERT(self->mStreamFrame != NULL);

    self->mStreamFrameAvail -= audioBuffer->frameCount;
    if (self->mStreamFrameAvail == 0) {
        // 失败则返回出错码, 无关远程
        ret = ICStream_Producer_releaseFrame(self->mICStream, self->mStreamFrame);
        ASSERT(IC_OK == ret);

        // 发布最新状态至共享, 通信机制出错码
        ret = ICStream_Producer_commitRemote(self->mICStream);
        ASSERT(IC_OK == ret);

        // 无帧
        self->mStreamFrame = NULL;
    }

    if (cblk->out) {
        dcache_invalidate_range((unsigned long) &(cblk->bufferTimeoutMs),
                                ((unsigned long) &(cblk->bufferTimeoutMs)) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(int)));
        // synchronization & memory barrier: read_acquire. full system, any-any = 15
        // TODO: 因为不确定dcache invalidate算load还是store, 保险起见. any-any.
        __DSB();

        // If stepServer() has been called once, switch to normal obtainBuffer() timeout period
        if (cblk->bufferTimeoutMs == MAX_STARTUP_TIMEOUT_MS-1) {
            cblk->bufferTimeoutMs = MAX_RUN_TIMEOUT_MS;

            dcache_clean_range((unsigned long) &(cblk->bufferTimeoutMs),
                               ((unsigned long) &(cblk->bufferTimeoutMs)) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(int)));

            // synchronization & memory barrier: write_release. full system, any-any = 15
            __DSB();
        }
    }

    // Clear flow control error condition as new data has been written/read to/from buffer.
    cblk->flowControlFlag = 0;

    dcache_clean_range((unsigned long) &(cblk->flowControlFlag),
                       ((unsigned long) &(cblk->flowControlFlag)) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(int)));

    // synchronization & memory barrier: write_release. full system, any-any = 15
    __DSB();

    return;
}

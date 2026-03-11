
#ifndef _AUDIOTRACK_H
#define _AUDIOTRACK_H

#include "base.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#include <sys/types.h>

#include "IAudioTrack.h"
#include "AudioTrackShared.h"
#include "ic_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------

enum AudioTrack_channel_index {
    AudioTrack_MONO   = 0,
    AudioTrack_LEFT   = 0,
    AudioTrack_RIGHT  = 1
};

enum {
    AudioTrack_Buffer_MUTE    = 0x00000001
};

// 0-copy模式
/* Create Buffer on the stack and pass it to obtainBuffer()
 * and releaseBuffer().
 */
typedef struct {

    // Flags to modify the behavior of the buffer:
    //  AudioTrack_Buffer_MUTE
    uint32_t    flags;

    // The number of channels in the audio data.
    int         channelCount;

    // The format of the audio data.
    int         format;

    // number of sample frames corresponding to size;
    // on input to obtainBuffer() it is the number of frames desired,
    // on output from obtainBuffer() it is the number of available
    size_t      frameCount;

    // input/output in bytes == frameCount * frameSize
    size_t      size;

    // input to obtainBuffer(): unused, output: pointer to buffer
    union {
        void*       raw;
        short*      i16;
        int8_t*     i8;
    };
} AudioTrack_Buffer;

typedef uint32_t audio_channel_mask_t;
typedef uint32_t audio_format_t;

// offload到CP硬件播放
typedef struct {
    uint32_t sample_rate;               // sample rate in Hz
    audio_channel_mask_t channel_mask;  // channel mask
    audio_format_t format;              // audio format: MP3/AAC/OPUS

    // TBD: AP给不出?
    uint32_t bit_rate;                  // bit rate in bits per second
    int64_t duration_us;                // duration in microseconds, -1 if unknown

    bool is_streaming;                  // true if streaming, false if local playback
} audio_offload_info_t;

//------------------------------------------------

typedef struct AudioTrack AudioTrack;

struct AudioTrack {
    IAudioTrack*        mAudioTrack;
    // ic_stream attached
    ICStream*           mICStream;
    audio_track_cblk_t* mCblk;

    float                   mVolume[2];
    uint32_t                mFrameCount;

    uint8_t                 mStreamType;
    uint32_t                mFormat;
    uint8_t                 mChannelCount;
    uint8_t                 mMuted;
    uint32_t                mChannels;
    status_t                mStatus;
    uint32_t                mLatency;

    atomic_int              mActive;

    void*                   mSharedBuffer;
    uint32_t                mFlags;

    void*                   mStreamFrame;
    int                     mStreamFrameAvail;
};

/* Creates an audio track and registers it with AudioFlinger.
 * Once created, the track needs to be started before it can be used.
 * Unspecified values are set to the audio hardware's current
 * values.
 *
 * Parameters:
 *
 * self:               A pointer to the AudioTrack instance.
 * sampleRate:         Track sampling rate in Hz.
 *                     If set to 0, then default to the Audio HAL's sample rate.
 *                     Users should check the Audio HAL for exact information.
 * format:             Audio format (e.g AudioSystem::PCM_16_BIT for signed
 *                     16 bits per sample).
 *                     If set to 0, then default to PCM_16_BIT.
 * channels:           Channel mask: see AudioSystem::audio_channels.
 *                     If set to 0, then default to CHANNEL_OUT_STEREO.
 * frameCount:         Total size of track PCM buffer in frames. This defines the
 *                     latency of the track. Frame refers to a single sample of
 *                     audio data from all channels.
 * offload_info:       Reserved for offload play, not supported now.
 */

status_t AudioTrack_ctor(   AudioTrack* self                    ,
                            uint32_t sampleRate                 ,
                            int format                          ,
                            int channels                        ,
                            int frameCount                      ,
                            audio_offload_info_t* offload_info) ;

/* Terminates the AudioTrack and unregisters it from AudioFlinger.
 * Also destroys all resources assotiated with the AudioTrack.
 */
status_t AudioTrack_dtor(   AudioTrack* self);

/* Initialize an uninitialized AudioTrack.
 * It takes the same parameters as AudioTrack_ctor.
 *
 * Returned status (from Errors.h) can be:
 *  - NO_ERROR: successful intialization
 *  - INVALID_OPERATION: AudioTrack is already intitialized
 *  - BAD_VALUE: invalid parameter (channels, format, sampleRate...)
 *  - NO_INIT: audio server or audio hardware not initialized
 * */
status_t    AudioTrack_set( AudioTrack* self                    ,
                            uint32_t sampleRate                 ,
                            int format                          ,
                            int channels                        ,
                            int frameCount                      ,
                            audio_offload_info_t* offload_info) ;

/* getters, see constructor */

/*
 * Returned value:
 *
 * format:             Audio format (e.g AudioSystem::PCM_16_BIT for signed
 *                     16 bits per sample).
 */
int         AudioTrack_format(AudioTrack* self);

/*
 * Returned value:
 *
 * count of channels in a Frame
 */
int         AudioTrack_channelCount(AudioTrack* self);

/*
 * Returned value:
 *
 * frameCount:         Total size of track PCM buffer in frames. This defines the
 *                     latency of the track. Frame refers to a single sample of
 *                     audio data from all channels.
 */
uint32_t    AudioTrack_frameCount(AudioTrack* self);

/*
 * Returned value:
 *
 * bytes of a frame:   sample size in bytes multiplied by the channel count
 */
int         AudioTrack_frameSize(AudioTrack* self);

/* After it's created the track is not active. Call start() to
 * make it active. If set, the callback will start being called.
 */
void        AudioTrack_start(AudioTrack* self);

/* Stop a track. If set, the callback will cease being called and
 * obtainBuffer returns STOPPED. Note that obtainBuffer() still works
 * and will fill up buffers until the pool is exhausted.
 */
void        AudioTrack_stop(AudioTrack* self);
bool        AudioTrack_stopped(AudioTrack* self);

/* flush a stopped track. All pending buffers are discarded.
 * This function has no effect if the track is not stoped.
 */
void        AudioTrack_flush(AudioTrack* self);

/* Pause a track. If set, the callback will cease being called and
 * obtainBuffer returns STOPPED. Note that obtainBuffer() still works
 * and will fill up buffers until the pool is exhausted.
 */
void        AudioTrack_pause(AudioTrack* self);

/* mute or unmutes this track.
 * While mutted, the callback, if set, is still called.
 */
void        AudioTrack_mute(AudioTrack* self, bool);
bool        AudioTrack_muted(AudioTrack* self);


/*
 * Sets the specified left and right output gain values on the AudioTrack.
 *
 * Gain values are clamped to the closed interval [0.0, 1.0]. A value of 0.0
 * results in zero gain (silence), and a value of 1.0 means unity gain
 * (signal unchanged). The default value is 1.0 meaning unity gain.
 *
 * Parameters:
 *  left:               output gain for the left channel.
 *  right:              output gain for the right channel
 */
void        AudioTrack_setVolume(AudioTrack* self, float left, float right);
void        AudioTrack_getVolume(AudioTrack* self, float* left, float* right);

/* returns a handle on the audio output used by this AudioTrack.
 *
 * Parameters:
 *  none.
 *
 * Returned value:
 *  handle on audio hardware output
 */
// audio_io_handle_t    AudioTrack_getOutput(AudioTrack* self);

/* obtains a buffer of "frameCount" frames. The buffer must be
 * filled entirely. If the track is stopped, obtainBuffer() returns
 * STOPPED instead of NO_ERROR as long as there are buffers availlable,
 * at which point NO_MORE_BUFFERS is returned.
 * Buffers will be returned until the pool (buffercount())
 * is exhausted, at which point obtainBuffer() will either block
 * or return WOULD_BLOCK depending on the value of the "blocking"
 * parameter.
 *
 * Parameters:
 *  audioBuffer:        Used as both [in] and [out] param. Refer to AudioTrack_Buffer.
 *  waitCount:          Internal wait count if can't obtain a buffer.
 *                      Recommend value: 1.
 *
 * Returned value:
 *  Possible errors:    NO_MORE_BUFFERS, STOPPED
 *  [out] audioBuffer:  zero or the positive number of bytes that were written, if no error.
 */

enum {
    NO_MORE_BUFFERS = 0x80000001,
    STOPPED = 1
};

status_t    AudioTrack_obtainBuffer(AudioTrack* self, AudioTrack_Buffer* audioBuffer, int32_t waitCount);
void        AudioTrack_releaseBuffer(AudioTrack* self, AudioTrack_Buffer* audioBuffer);

/*
 * Writes the audio data to the audio sink for playback (streaming mode), or copies
 * audio data for later playback (static buffer mode). The format specified in the
 * AudioTrack constructor should be AudioSystem::PCM_16_BIT to correspond to the
 * data in the array.
 *
 * In streaming mode, the write will normally block until all the data has been
 * enqueued for playback, and will return a full transfer count. However, if the
 * track is stopped or paused on entry, or another thread interrupts the write by
 * calling stop or pause, or an I/O error occurs during the write, then the write
 * may return a short transfer count.
 *
 * In static buffer mode, copies the data to the buffer starting at offset 0. Note
 * that the actual playback of this data might occur after this function returns.
 *
 * This is a convenience interface to the audio buffer.
 * It is implemented on top of obtainBuffer/releaseBuffer. For best
 * performance.
 *
 * Parameters:
 *  buffer:             holds the data to play. This value cannot be null.
 *  size:               the number of bytes to write
 *
 * Returned value:
 *  Zero or the positive number of bytes that were written. The number of bytes will
 *  be a multiple of the frame size in bytes not to exceed [in] size.
 */
ssize_t     AudioTrack_write(AudioTrack* self, const void* buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif // _AUDIOTRACK_H

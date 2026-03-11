
#ifndef _AUDIOSYSTEM_H_
#define _AUDIOSYSTEM_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#include "base.h"
#include "String8.h"

//------------------------------------------------
// forward declarations

typedef struct _IAudioFlinger IAudioFlinger;

//------------------------------------------------
typedef int audio_io_handle_t;

typedef enum {
    AudioSystem_DEFAULT          =-1,
    AudioSystem_SYSTEM           = 1,
    AudioSystem_MUSIC            = 3,
    AudioSystem_ALARM            = 4,
    AudioSystem_NOTIFICATION     = 5,
    AudioSystem_ENFORCED_AUDIBLE = 7, // Sounds that cannot be muted by user and must be routed to speaker
    AudioSystem_TTS              = 9,
    AudioSystem_NUM_STREAM_TYPES
} AudioSystem_stream_type;

// Audio sub formats (see AudioSystem::audio_format).
enum AudioSystem_pcm_sub_format {
    PCM_SUB_16_BIT          = 0x1, // must be 1 for backward compatibility
    PCM_SUB_8_BIT           = 0x2, // must be 2 for backward compatibility
};

// Audio format consists in a main format field (upper 8 bits) and a sub format field (lower 24 bits).
// The main format indicates the main codec type. The sub format field indicates options and parameters
// for each format. The sub format is mainly used for record to indicate for instance the requested bitrate
// or profile. It can also be used for certain formats to give informations not present in the encoded
// audio stream (e.g. octet alignement for AMR).
enum AudioSystem_audio_format {
    INVALID_FORMAT      = -1,
    FORMAT_DEFAULT      = 0,
    PCM                 = 0x00000000, // must be 0 for backward compatibility
    MP3                 = 0x01000000,
    AMR_NB              = 0x02000000,
    AMR_WB              = 0x03000000,
    AAC                 = 0x04000000,
    HE_AAC_V1           = 0x05000000,
    HE_AAC_V2           = 0x06000000,
    VORBIS              = 0x07000000,
    MAIN_FORMAT_MASK    = 0xFF000000,
    SUB_FORMAT_MASK     = 0x00FFFFFF,
    // Aliases
    PCM_16_BIT          = (PCM|PCM_SUB_16_BIT),
    PCM_8_BIT          = (PCM|PCM_SUB_8_BIT)
};

// Channel mask definitions must be kept in sync with JAVA values in /media/java/android/media/AudioFormat.java
enum AudioSystem_audio_channels {
    // output channels
    CHANNEL_OUT_FRONT_LEFT = 0x4,
    CHANNEL_OUT_FRONT_RIGHT = 0x8,
    CHANNEL_OUT_MONO = CHANNEL_OUT_FRONT_LEFT,
    CHANNEL_OUT_STEREO = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT),
    CHANNEL_OUT_ALL = (CHANNEL_OUT_FRONT_LEFT | CHANNEL_OUT_FRONT_RIGHT),

    // input channels
    CHANNEL_IN_LEFT = 0x4,
    CHANNEL_IN_RIGHT = 0x8,
    CHANNEL_IN_AUX_LEFT = 0x10,
    CHANNEL_IN_AUX_RIGHT = 0x20,
    CHANNEL_IN_ECHO_LEFT = 0x40,
    CHANNEL_IN_ECHO_RIGHT = 0x80,
    CHANNEL_IN_MONO = CHANNEL_IN_LEFT,
    CHANNEL_IN_STEREO = (CHANNEL_IN_LEFT | CHANNEL_IN_RIGHT),
    CHANNEL_IN_AUX = (CHANNEL_IN_AUX_LEFT | CHANNEL_IN_AUX_RIGHT),
    CHANNEL_IN_ECHO = (CHANNEL_IN_ECHO_LEFT | CHANNEL_IN_ECHO_RIGHT),
    CHANNEL_IN_ALL = (CHANNEL_IN_LEFT | CHANNEL_IN_RIGHT |
                      CHANNEL_IN_AUX_LEFT | CHANNEL_IN_AUX_RIGHT |
                      CHANNEL_IN_ECHO_LEFT | CHANNEL_IN_ECHO_RIGHT),
};

// request to open a direct output with getOutput() (by opposition to sharing an output with other AudioTracks)
typedef enum {
    AudioSystem_OUTPUT_FLAG_INDIRECT = 0x0,
    AudioSystem_OUTPUT_FLAG_DIRECT = 0x1
} AudioSystem_output_flags;

bool AudioSystem_isLinearPCM(uint32_t format);

uint32_t AudioSystem_popCount(uint32_t u);

//------------------------------------------------
// 初始化
int32_t AudioSystem_initialize(void);



bool AudioSystem_isValidFormat(uint32_t format);

bool AudioSystem_isOutputChannel(uint32_t channel);
bool AudioSystem_isInputChannel(uint32_t channel);

// set/get master volume
status_t AudioSystem_setMasterVolume(float volume);
status_t AudioSystem_getMasterVolume(float* volume);

// mute/unmute audio outputs
status_t AudioSystem_setMasterMute(bool mute);
status_t AudioSystem_getMasterMute(bool* mute);

status_t AudioSystem_setParameters(audio_io_handle_t ioHandle, const String8 *keyValuePairs);
String8  AudioSystem_getParameters(audio_io_handle_t ioHandle, const String8 *keys);

status_t AudioSystem_getOutputSamplingRate(int* samplingRate, int streamType);

status_t AudioSystem_getOutputFrameCount(int* frameCount, int streamType);

status_t AudioSystem_getOutputLatency(uint32_t* latency, int streamType);

//------------------------------------------------
// private methods

IAudioFlinger* AudioSystem_get_audio_flinger();

audio_io_handle_t AudioSystem_getOutput(AudioSystem_stream_type stream,
                                    uint32_t samplingRate,
                                    uint32_t format,
                                    uint32_t channels,
                                    AudioSystem_output_flags flags);

audio_io_handle_t AudioSystem_getInput(int inputSource,
                                    uint32_t samplingRate,
                                    uint32_t format,
                                    uint32_t channels,
                                    uint32_t acoustics);

//------------------------------------------------

#endif  /*_AUDIOSYSTEM_H_*/

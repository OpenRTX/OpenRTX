#ifndef DROID_IAUDIOFLINGER_H
#define DROID_IAUDIOFLINGER_H

#include "String8.h"

#include "ServiceNames.h"

#include "IAudioTrack.h"
#include "IAudioRecord.h"

#include "AudioSystem.h"

// 标准库
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

//------------------------------------------------

// typedef struct _IAudioTrack IAudioTrack;
// typedef struct _IMemory IMemory;
// typedef struct _IAudioFlingerClient IAudioFlingerClient;

typedef struct _IAudioFlinger IAudioFlinger;

struct _IAudioFlinger
{
    /* create an audio track and registers it with AudioFlinger.
     * return null if the track cannot be created.
     */
    // IAudioTrack*    (*createTrack)(IAudioFlinger *self
    IAudioTrack*    (*createTrack)(IAudioFlinger *iAudioFlinger,
                                uint32_t pid,
                                int streamType,
                                uint32_t sampleRate,
                                int format,
                                int channelCount,
                                int frameCount,
                                uint32_t flags,
                                void* sharedBuffer,
                                int output,
                                int* status);

    IAudioRecord*   (*openRecord)(IAudioFlinger *iAudioFlinger,
                                uint32_t pid,
                                int input,
                                uint32_t sampleRate,
                                int format,
                                int channelCount,
                                int frameCount,
                                uint32_t flags,
                                status_t* status);

    status_t            (*setMasterVolume)(IAudioFlinger *iAudioFlinger,
                                float volume);

    status_t            (*setMasterMute)(IAudioFlinger *iAudioFlinger,
                                bool mute);

    status_t            (*masterVolume)(IAudioFlinger *iAudioFlinger,
                                float* volume);

    status_t            (*masterMute)(IAudioFlinger *iAudioFlinger,
                                bool* mute);

    status_t            (*setParameters)(IAudioFlinger *iAudioFlinger,
                                audio_io_handle_t ioHandle,
                                const String8 *keyValuePairs);

    String8             (*getParameters)(IAudioFlinger *iAudioFlinger,
                                audio_io_handle_t ioHandle,
                                const String8 *keys);
};

//------------------------------------------------
// helper
IAudioFlinger* defaultAudioFlinger();

//------------------------------------------------
// 类静态函数
int IAudioFlinger_Class_init(void *context);

IAudioFlinger* IAudioFlinger_as_interface(uint32_t binder);

// binder => IInterface
static inline
IAudioFlinger* interface_cast_IAudioFlinger(uint32_t binder)
{

    return IAudioFlinger_as_interface(binder);
}

#endif // DROID_IAUDIOFLINGER_H

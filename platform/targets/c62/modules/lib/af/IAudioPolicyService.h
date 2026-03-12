#ifndef DROID_IAUDIOPOLICYSERVICE_H
#define DROID_IAUDIOPOLICYSERVICE_H

#include "ServiceNames.h"
#include "AudioSystem.h"

// 标准库
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

//------------------------------------------------

typedef struct _IAudioPolicyService IAudioPolicyService;

struct _IAudioPolicyService
{
    //
    // IAudioPolicyService interface (see AudioPolicyInterface for method descriptions)
    //
    audio_io_handle_t (*getOutput)(IAudioPolicyService* self,
                                   int stream,
                                   uint32_t samplingRate,
                                   uint32_t format,
                                   uint32_t channels,
                                   uint32_t flags);

    status_t (*startOutput)(IAudioPolicyService* self, audio_io_handle_t output, int stream);

    status_t (*stopOutput)(IAudioPolicyService* self, audio_io_handle_t output, int stream);

    void (*releaseOutput)(IAudioPolicyService* self, audio_io_handle_t output);

    audio_io_handle_t (*getInput)(IAudioPolicyService* self,
                                  int inputSource,
                                  uint32_t samplingRate,
                                  uint32_t format,
                                  uint32_t channels,
                                  uint32_t acoustics);

    status_t (*startInput)(IAudioPolicyService* self, audio_io_handle_t input);

    status_t (*stopInput)(IAudioPolicyService* self, audio_io_handle_t input);

    void (*releaseInput)(IAudioPolicyService* self, audio_io_handle_t input);
};

//------------------------------------------------
// helper
IAudioPolicyService* defaultAudioPolicyService();

//------------------------------------------------
// 类静态函数
int IAudioPolicyService_Class_init(void *context);

IAudioPolicyService* IAudioPolicyService_as_interface(uint32_t binder);

// binder => IInterface
static inline
IAudioPolicyService* interface_cast_IAudioPolicyService(uint32_t binder)
{

    return IAudioPolicyService_as_interface(binder);
}

#endif // DROID_IAUDIOPOLICYSERVICE_H

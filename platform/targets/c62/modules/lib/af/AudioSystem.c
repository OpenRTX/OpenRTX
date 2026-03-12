//------------------------------------------------
// self
#include "AudioSystem.h"

// package: misc
#include "IServiceManager.h"
#include "IAudioFlinger.h"
#include "IAudioPolicyService.h"
#include "Errors.h"

// package: stream
#include "StreamManager.h"

#include "IServiceManager.h"
#include "IAudioFlinger.h"

// 核间 ------------------------------------------------
#include "lsf.h"

#include "ic_stream.h"
#include "ic_proxy.h"
#include "rpc_client.h"

#include "audioflinger_property_def.h"

//------------------------------------------------
// platform

// os
#include "FreeRTOS.h"

// utils: log
//#define LOG_NDEBUG 0
#define LOG_TAG "AudioSystem"
#include "venus_log.h"

//------------------------------------------------
// private defines

static IAudioFlinger* gAudioFlinger;
static IAudioPolicyService* gAudioPolicyService;

//------------------------------------------------
// implementation

// RAII: resource acquisition is initialization
int32_t AudioSystem_initialize(void)
{
    CLOGD("[%s] ...\r\n", __func__);

    gAudioFlinger = NULL;
    gAudioPolicyService = NULL;

    // TODO: 数据流在此处才挂接, 是否可能cp的数据流来的太快, 而ap client端还未运行?
    // 同步阻塞, 核间数据通路 连接: ap <- cp
    StreamManager_connect();

    // 属性服务连接后, 由service模块导入 核间属性 声明.
    // 此前, 本端查询不到 核间属性组
    lsf_import_properties(PROPERTY_GROUP_ID_AUDIOFLINGER, audioflinger_property_defs,
                          sizeof(audioflinger_property_defs)/sizeof(audioflinger_property_defs[0]));

    return 0;
}

// establish binder interface to AudioFlinger service
IAudioFlinger* AudioSystem_get_audio_flinger()
{
    if (gAudioFlinger == 0) {
        IServiceManager* sm = defaultServiceManager();

        uint32_t binder = sm->getService(sm, AUDIOFLINGER_SERVICENAME);

        gAudioFlinger = interface_cast_IAudioFlinger(binder);

        CLOGD("[%s] binder: %d - group: %d, property: %d\r\n", __func__,
              binder, GROUP_ID(binder), PROPERTY_ID(binder));
    }
    if (gAudioFlinger==0) {
        CLOGD("no AudioFlinger!?");
    }

    return gAudioFlinger;
}

// establish binder interface to AudioPolicyService
IAudioPolicyService* AudioSystem_get_audio_policy_service()
{
    if (gAudioPolicyService == 0) {
        IServiceManager* sm = defaultServiceManager();

        uint32_t binder = sm->getService(sm, AUDIOPOLICYSERVICE_SERVICENAME);

        gAudioPolicyService = interface_cast_IAudioPolicyService(binder);

        CLOGD("[%s] binder: %d - group: %d, property: %d\r\n", __func__,
              binder, GROUP_ID(binder), PROPERTY_ID(binder));
    }
    if (gAudioPolicyService==0) {
        CLOGD("no AudioPolicyService!?");
    }

    return gAudioPolicyService;
}


bool AudioSystem_isLinearPCM(uint32_t format)
{
    switch (format) {
    case         PCM_16_BIT:
    case         PCM_8_BIT:
        return true;
    default:
        return false;
    }
}

// use emulated popcount optimization
// http://www.df.lth.se/~john_e/gems/gem002d.html
uint32_t AudioSystem_popCount(uint32_t u)
{
    u = ((u&0x55555555) + ((u>>1)&0x55555555));
    u = ((u&0x33333333) + ((u>>2)&0x33333333));
    u = ((u&0x0f0f0f0f) + ((u>>4)&0x0f0f0f0f));
    u = ((u&0x00ff00ff) + ((u>>8)&0x00ff00ff));
    u = ( u&0x0000ffff) + (u>>16);
    return u;
}

bool AudioSystem_isValidFormat(uint32_t format)
{
    switch (format & MAIN_FORMAT_MASK) {
    case         PCM:
    case         MP3:
    case         AMR_NB:
    case         AMR_WB:
    case         AAC:
    case         HE_AAC_V1:
    case         HE_AAC_V2:
    case         VORBIS:
        return true;
    default:
        return false;
    }
}

bool AudioSystem_isInputChannel(uint32_t channel)
{
    if ((channel & ~CHANNEL_IN_ALL) == 0) {
        return true;
    } else {
        return false;
    }
}

bool AudioSystem_isOutputChannel(uint32_t channel)
{
    if ((channel & ~CHANNEL_OUT_ALL) == 0) {
        return true;
    } else {
        return false;
    }
}

status_t AudioSystem_setMasterVolume(float volume)
{
    IServiceManager* sm = defaultServiceManager();

    uint32_t binder = sm->getService(sm, AUDIOFLINGER_SERVICENAME);

    IAudioFlinger* af = interface_cast_IAudioFlinger(binder);

    return af->setMasterVolume(af, volume);
}

status_t AudioSystem_getMasterVolume(float* volume)
{
    IServiceManager* sm = defaultServiceManager();

    uint32_t binder = sm->getService(sm, AUDIOFLINGER_SERVICENAME);

    IAudioFlinger* af = interface_cast_IAudioFlinger(binder);

    return af->masterVolume(af, volume);
}

status_t AudioSystem_setMasterMute(bool mute)
{
    IServiceManager* sm = defaultServiceManager();

    uint32_t binder = sm->getService(sm, AUDIOFLINGER_SERVICENAME);

    IAudioFlinger* af = interface_cast_IAudioFlinger(binder);

    return af->setMasterMute(af, mute);
}

status_t AudioSystem_getMasterMute(bool* mute)
{
    IServiceManager* sm = defaultServiceManager();

    uint32_t binder = sm->getService(sm, AUDIOFLINGER_SERVICENAME);

    IAudioFlinger* af = interface_cast_IAudioFlinger(binder);

    return af->masterMute(af, mute);
}

status_t AudioSystem_setParameters(audio_io_handle_t ioHandle, const String8 *keyValuePairs)
{
    IServiceManager* sm = defaultServiceManager();

    uint32_t binder = sm->getService(sm, AUDIOFLINGER_SERVICENAME);

    IAudioFlinger* af = interface_cast_IAudioFlinger(binder);

    return af->setParameters(af, ioHandle, keyValuePairs);
}

String8 AudioSystem_getParameters(audio_io_handle_t ioHandle, const String8 *keys)
{
    IServiceManager* sm = defaultServiceManager();

    uint32_t binder = sm->getService(sm, AUDIOFLINGER_SERVICENAME);

    IAudioFlinger* af = interface_cast_IAudioFlinger(binder);

    return af->getParameters(af, ioHandle, keys);
}

status_t AudioSystem_getOutputSamplingRate(int* samplingRate, int streamType)
{
    audio_io_handle_t output;

    if (streamType == AudioSystem_DEFAULT) {
        streamType = AudioSystem_MUSIC;
    }

    output = AudioSystem_getOutput((AudioSystem_stream_type)streamType,
                                   0,
                                   FORMAT_DEFAULT,
                                   CHANNEL_OUT_STEREO,
                                   AudioSystem_OUTPUT_FLAG_INDIRECT);
    if (output == 0) {
        return PERMISSION_DENIED;
    }

    // TBD
    *samplingRate = 16000;

    CLOGD("getOutputSamplingRate() streamType %d, output %d, sampling rate %d", streamType, output, *samplingRate);

    return NO_ERROR;
}

audio_io_handle_t AudioSystem_getOutput(AudioSystem_stream_type stream,
                                    uint32_t samplingRate,
                                    uint32_t format,
                                    uint32_t channels,
                                    AudioSystem_output_flags flags)
{
    IAudioPolicyService* aps = AudioSystem_get_audio_policy_service();
    if (aps == 0) {
        return 0;
    }

    audio_io_handle_t output = aps->getOutput(aps,
    		                                  stream,
                                              samplingRate,
                                              format,
                                              channels,
                                              flags);

    // output = 1;
    return output;
}

audio_io_handle_t AudioSystem_getInput(int inputSource,
                                    uint32_t samplingRate,
                                    uint32_t format,
                                    uint32_t channels,
                                    uint32_t acoustics)
{
    IAudioPolicyService* aps = AudioSystem_get_audio_policy_service();
    if (aps == 0) {
        return 0;
    }

    audio_io_handle_t input = aps->getInput(aps,
                                            inputSource,
                                            samplingRate,
                                            format,
                                            channels,
                                            acoustics);

    // input = 2;
    return input;
}


status_t AudioSystem_getOutputFrameCount(int* frameCount, int streamType)
{
    audio_io_handle_t output;

    if (streamType == AudioSystem_DEFAULT) {
        streamType = AudioSystem_MUSIC;
    }

    output = AudioSystem_getOutput((AudioSystem_stream_type)streamType,
                                   0,
                                   FORMAT_DEFAULT,
                                   CHANNEL_OUT_STEREO,
                                   AudioSystem_OUTPUT_FLAG_INDIRECT);
    if (output == 0) {
        return PERMISSION_DENIED;
    }

    // TBD
    // 与AudioHardwareGeneric 一致, 按stereo呈现: 按10ms 1个buffer, 10ms的stereo数据.
    // bytes = 160 samples * 2 channel * frame size
    *frameCount = 160;

    CLOGD("getOutputFrameCount() streamType %d, output %d, frameCount %d", streamType, output, *frameCount);

    return NO_ERROR;
}

status_t AudioSystem_getOutputLatency(uint32_t* latency, int streamType)
{
    audio_io_handle_t output;

    if (streamType == AudioSystem_DEFAULT) {
        streamType = AudioSystem_MUSIC;
    }

    output = AudioSystem_getOutput((AudioSystem_stream_type)streamType,
                                   0,
                                   FORMAT_DEFAULT,
                                   CHANNEL_OUT_STEREO,
                                   AudioSystem_OUTPUT_FLAG_INDIRECT);
    if (output == 0) {
        return PERMISSION_DENIED;
    }

    // TBD: 20ms
    *latency = 20;

    CLOGD("getOutputLatency() streamType %d, output %d, latency %d", streamType, output, *latency);

    return NO_ERROR;
}

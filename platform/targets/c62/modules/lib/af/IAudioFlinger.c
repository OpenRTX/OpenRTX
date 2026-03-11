
#define LOG_TAG "AudioFlinger"

#include "IAudioFlinger.h"

#include "base.h"

#include "IServiceManager.h"

//------------------------------------------------
#include "lsf.h"
#include "audioflinger_property_def.h"

//------------------------------------------------
#include "ic_common.h"
#include "venus_log.h"

//------------------------------------------------
// function IDs
enum {
    IAUDIOFLINGER_CREATE_TRACK,
    IAUDIOFLINGER_OPEN_RECORD,
    IAUDIOFLINGER_SAMPLE_RATE,
    IAUDIOFLINGER_CHANNEL_COUNT,
    IAUDIOFLINGER_FORMAT,
    IAUDIOFLINGER_FRAME_COUNT,
    IAUDIOFLINGER_LATENCY,
    IAUDIOFLINGER_SET_MASTER_VOLUME,
    IAUDIOFLINGER_SET_MASTER_MUTE,
    IAUDIOFLINGER_MASTER_VOLUME,
    IAUDIOFLINGER_MASTER_MUTE,
    IAUDIOFLINGER_SET_STREAM_VOLUME,
    IAUDIOFLINGER_SET_STREAM_MUTE,
    IAUDIOFLINGER_STREAM_VOLUME,
    IAUDIOFLINGER_STREAM_MUTE,
    IAUDIOFLINGER_SET_MODE,
    IAUDIOFLINGER_SET_MIC_MUTE,
    IAUDIOFLINGER_GET_MIC_MUTE,
    IAUDIOFLINGER_IS_MUSIC_ACTIVE,
    IAUDIOFLINGER_SET_PARAMETERS,
    IAUDIOFLINGER_GET_PARAMETERS,
    IAUDIOFLINGER_REGISTER_CLIENT,
    IAUDIOFLINGER_GET_INPUTBUFFERSIZE,
    IAUDIOFLINGER_OPEN_OUTPUT,
    IAUDIOFLINGER_OPEN_DUPLICATE_OUTPUT,
    IAUDIOFLINGER_CLOSE_OUTPUT,
    IAUDIOFLINGER_SUSPEND_OUTPUT,
    IAUDIOFLINGER_RESTORE_OUTPUT,
    IAUDIOFLINGER_OPEN_INPUT,
    IAUDIOFLINGER_CLOSE_INPUT,
    IAUDIOFLINGER_SET_STREAM_OUTPUT,
    IAUDIOFLINGER_SET_VOICE_VOLUME,
};

//------------------------------------------------
// BpAudioFlinger header

typedef struct _BpAudioFlinger BpAudioFlinger;

struct _BpAudioFlinger
{
    // interface instance
    IAudioFlinger iiAudioFlinger;

    uint32_t binder;
};

// singleton
static BpAudioFlinger sBpAudioFlinger;

// BpAudioFlinger::Interface转换辅助函数
static inline
IAudioFlinger* BpAudioFlinger_as_IAudioFlinger(BpAudioFlinger *self)
{
    return (& self->iiAudioFlinger);
}

BpAudioFlinger* BpAudioFlinger_ctor(BpAudioFlinger* self, uint32_t binder);

void BpAudioFlinger_dtor(BpAudioFlinger *self);

IAudioTrack* BpAudioFlinger_createTrack(IAudioFlinger *iAudioFlinger,
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

IAudioRecord* BpAudioFlinger_openRecord(IAudioFlinger *iAudioFlinger,
                                    uint32_t pid,
                                    int input,
                                    uint32_t sampleRate,
                                    int format,
                                    int channelCount,
                                    int frameCount,
                                    uint32_t flags,
                                    status_t* status);

status_t BpAudioFlinger_setMasterVolume(IAudioFlinger *iAudioFlinger,
                                    float volume);

status_t BpAudioFlinger_setMasterMute(IAudioFlinger *iAudioFlinger,
                                    bool mute);

status_t BpAudioFlinger_masterVolume(IAudioFlinger *iAudioFlinger,
                                    float* volume);

status_t BpAudioFlinger_masterMute(IAudioFlinger *iAudioFlinger,
                                    bool* mute);

status_t BpAudioFlinger_setParameters(IAudioFlinger *iAudioFlinger,
                                    audio_io_handle_t ioHandle,
                                    const String8 *keyValuePairs);

String8 BpAudioFlinger_getParameters(IAudioFlinger *iAudioFlinger,
                                    audio_io_handle_t ioHandle,
                                    const String8 *keys);

//------------------------------------------------
// IAudioFlinger header


//------------------------------------------------
// IAudioFlinger

// IAudioFlinger::类静态函数
// 根据binder, 建立Bp/Bn实体对象, 返回interface
IAudioFlinger* IAudioFlinger_as_interface(uint32_t binder)
{
    BpAudioFlinger* obj = BpAudioFlinger_ctor(&sBpAudioFlinger, binder);

    IAudioFlinger *ret = BpAudioFlinger_as_IAudioFlinger(obj);

    return ret;
}

//------------------------------------------------
//------------------------------------------------
//------------------------------------------------
// BpAudioFlinger method

struct IAudioFlinger_ARGS {
    uint32_t target_binder;
    int funcID;
};

struct IAudioFlinger_createTrack_ARGS {
    uint32_t target_binder;
    int funcID;

    uint32_t pid;
    int streamType;
    uint32_t sampleRate;
    int format;
    int channelCount;
    int frameCount;
    uint32_t flags;
    void* sharedBuffer;
    int output;
};


struct IAudioFlinger_createTrack_RETS {
    int status;

    uint32_t trackBinder;
};

// 返回binder
IAudioTrack* BpAudioFlinger_createTrack(IAudioFlinger *iAudioFlinger,
                                    uint32_t pid,
                                    int streamType,
                                    uint32_t sampleRate,
                                    int format,
                                    int channelCount,
                                    int frameCount,
                                    uint32_t flags,
                                    void* sharedBuffer,
                                    int output,
                                    int* status)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioFlinger *self = container_of(iAudioFlinger, BpAudioFlinger, iiAudioFlinger);

    struct IAudioFlinger_createTrack_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIOFLINGER_CREATE_TRACK,

        .pid = pid,
        .streamType = streamType,
        .sampleRate = sampleRate,
        .format = format,
        .channelCount = channelCount,
        .frameCount = frameCount,
        .flags = flags,
        .sharedBuffer = sharedBuffer,
        .output = output,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf; // 类型: 内存块
    var.u.memBuf.size = sizeof(struct IAudioFlinger_createTrack_ARGS); // 内存块长度
    var.u.memBuf.pData = &args_RPC; // 填入param value

    // 远程返回 {--------------------------------------
    struct IAudioFlinger_createTrack_RETS *rets_RPC;
    rets_RPC = (struct IAudioFlinger_createTrack_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    // 远程返回值 - 读取
    CLOGD("[%s] status: %d, trackBinder: %d - group: %d, property: %d, interface: %d\r\n", __func__,
          rets_RPC->status, rets_RPC->trackBinder, GROUP_ID(rets_RPC->trackBinder),
          PROPERTY_ID(rets_RPC->trackBinder), INTERFACE_ID(rets_RPC->trackBinder));

    uint32_t trackBinder = rets_RPC->trackBinder;
    *status = rets_RPC->status;

    size_t rets_RPC_size = sizeof(struct IAudioFlinger_createTrack_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);
    // 远程返回 }--------------------------------------

    // 不涉及Variant本体的释放, 只清空其内部间接内存占用
    LsfVariant_clear(&var);

    IAudioTrack* track = NULL;

    // 远程返回值 - 使用
    if (*status == NO_ERROR) {
        track = interface_cast_IAudioTrack(trackBinder);
    }

    return track;
}

struct IAudioFlinger_openRecord_ARGS {
    uint32_t target_binder;
    int funcID;

    uint32_t pid;
    int input;
    uint32_t sampleRate;
    int format;
    int channelCount;
    int frameCount;
    uint32_t flags;
};

struct IAudioFlinger_openRecord_RETS {
    status_t status;

    uint32_t recordBinder;
};

IAudioRecord* BpAudioFlinger_openRecord(IAudioFlinger *iAudioFlinger,
                                uint32_t pid,
                                int input,
                                uint32_t sampleRate,
                                int format,
                                int channelCount,
                                int frameCount,
                                uint32_t flags,
                                status_t* status)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioFlinger *self = container_of(iAudioFlinger, BpAudioFlinger, iiAudioFlinger);

    struct IAudioFlinger_openRecord_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIOFLINGER_OPEN_RECORD,

        .pid = pid,
        .input = input,
        .sampleRate = sampleRate,
        .format = format,
        .channelCount = channelCount,
        .frameCount = frameCount,
        .flags = flags,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf;
    var.u.memBuf.size = sizeof(struct IAudioFlinger_openRecord_ARGS);
    var.u.memBuf.pData = &args_RPC;

    // 远程返回 {--------------------------------------
    struct IAudioFlinger_openRecord_RETS *rets_RPC;
    rets_RPC = (struct IAudioFlinger_openRecord_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    // 远程返回值 - 读取
    CLOGD("[%s] status: %d, recordBinder: %d - group: %d, property: %d, interface: %d\r\n", __func__,
          rets_RPC->status, rets_RPC->recordBinder, GROUP_ID(rets_RPC->recordBinder),
          PROPERTY_ID(rets_RPC->recordBinder), INTERFACE_ID(rets_RPC->recordBinder));

    uint32_t recordBinder = rets_RPC->recordBinder;
    *status = rets_RPC->status;

    size_t rets_RPC_size = sizeof(struct IAudioFlinger_openRecord_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);
    // 远程返回 }--------------------------------------

    LsfVariant_clear(&var);

    IAudioRecord* record = NULL;

    if (*status == NO_ERROR) {
        record = interface_cast_IAudioRecord(recordBinder);
    }

    return record;
}

struct IAudioFlinger_setMasterVolume_ARGS {
    uint32_t target_binder;
    int funcID;

    float volume;
};

struct IAudioFlinger_setMasterVolume_RETS {
    status_t status;
};

status_t BpAudioFlinger_setMasterVolume(IAudioFlinger *iAudioFlinger,
                                float volume)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioFlinger *self = container_of(iAudioFlinger, BpAudioFlinger, iiAudioFlinger);

    struct IAudioFlinger_setMasterVolume_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIOFLINGER_SET_MASTER_VOLUME,

        .volume = volume,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf;
    var.u.memBuf.size = sizeof(struct IAudioFlinger_setMasterVolume_ARGS);
    var.u.memBuf.pData = &args_RPC;

    struct IAudioFlinger_setMasterVolume_RETS *rets_RPC;
    rets_RPC = (struct IAudioFlinger_setMasterVolume_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    status_t status = rets_RPC->status;

    size_t rets_RPC_size = sizeof(struct IAudioFlinger_setMasterVolume_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);

    LsfVariant_clear(&var);

    return status;
}

struct IAudioFlinger_masterVolume_ARGS {
    uint32_t target_binder;
    int funcID;
};

struct IAudioFlinger_masterVolume_RETS {
    status_t status;

    float volume;
};

status_t BpAudioFlinger_masterVolume(IAudioFlinger *iAudioFlinger,
                                float* volume)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioFlinger *self = container_of(iAudioFlinger, BpAudioFlinger, iiAudioFlinger);

    struct IAudioFlinger_masterVolume_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIOFLINGER_MASTER_VOLUME,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf;
    var.u.memBuf.size = sizeof(struct IAudioFlinger_masterVolume_ARGS);
    var.u.memBuf.pData = &args_RPC;

    struct IAudioFlinger_masterVolume_RETS *rets_RPC;
    rets_RPC = (struct IAudioFlinger_masterVolume_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    status_t status = rets_RPC->status;
    *volume = rets_RPC->volume;

    size_t rets_RPC_size = sizeof(struct IAudioFlinger_masterVolume_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);

    LsfVariant_clear(&var);

    return status;
}

struct IAudioFlinger_setMasterMute_ARGS {
    uint32_t target_binder;
    int funcID;

    bool mute;
};

struct IAudioFlinger_setMasterMute_RETS {
    status_t status;
};

status_t BpAudioFlinger_setMasterMute(IAudioFlinger *iAudioFlinger,
                                bool mute)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioFlinger *self = container_of(iAudioFlinger, BpAudioFlinger, iiAudioFlinger);

    struct IAudioFlinger_setMasterMute_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIOFLINGER_SET_MASTER_MUTE,

        .mute = mute,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf;
    var.u.memBuf.size = sizeof(struct IAudioFlinger_setMasterMute_ARGS);
    var.u.memBuf.pData = &args_RPC;

    struct IAudioFlinger_setMasterMute_RETS *rets_RPC;
    rets_RPC = (struct IAudioFlinger_setMasterMute_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    status_t status = rets_RPC->status;

    size_t rets_RPC_size = sizeof(struct IAudioFlinger_setMasterMute_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);

    LsfVariant_clear(&var);

    return status;
}

struct IAudioFlinger_masterMute_ARGS {
    uint32_t target_binder;
    int funcID;
};

struct IAudioFlinger_masterMute_RETS {
    status_t status;

    bool mute;
};

status_t BpAudioFlinger_masterMute(IAudioFlinger *iAudioFlinger,
                                bool* mute)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioFlinger *self = container_of(iAudioFlinger, BpAudioFlinger, iiAudioFlinger);

    struct IAudioFlinger_masterMute_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIOFLINGER_MASTER_MUTE,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf;
    var.u.memBuf.size = sizeof(struct IAudioFlinger_masterMute_ARGS);
    var.u.memBuf.pData = &args_RPC;

    struct IAudioFlinger_masterMute_RETS *rets_RPC;
    rets_RPC = (struct IAudioFlinger_masterMute_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    status_t status = rets_RPC->status;
    *mute = rets_RPC->mute;

    size_t rets_RPC_size = sizeof(struct IAudioFlinger_masterMute_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);

    LsfVariant_clear(&var);

    return status;
}

struct IAudioFlinger_setParameters_ARGS {
    uint32_t target_binder;
    int funcID;

    audio_io_handle_t ioHandle;
    size_t keyValuePairs_bytes;
};

struct IAudioFlinger_setParameters_RETS {
    status_t status;
};

status_t BpAudioFlinger_setParameters(IAudioFlinger *iAudioFlinger,
                                audio_io_handle_t ioHandle,
                                const String8 *keyValuePairs)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioFlinger *self = container_of(iAudioFlinger, BpAudioFlinger, iiAudioFlinger);

    size_t args_RPC_size = sizeof(struct IAudioFlinger_setParameters_ARGS);
    size_t args_buf_size = String8_bytes(keyValuePairs);
    uint8_t *args = malloc(args_RPC_size + args_buf_size);

    struct IAudioFlinger_setParameters_ARGS *args_RPC = (struct IAudioFlinger_setParameters_ARGS *)args;
    args_RPC->target_binder = self->binder;
    args_RPC->funcID = IAUDIOFLINGER_SET_PARAMETERS;
    args_RPC->ioHandle = ioHandle;
    args_RPC->keyValuePairs_bytes = args_buf_size;

    void *args_buf = args + args_RPC_size;
    memcpy(args_buf, String8_string(keyValuePairs), args_buf_size);

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf;
    var.u.memBuf.size = args_RPC_size + args_buf_size;
    var.u.memBuf.pData = args;

    struct IAudioFlinger_setParameters_RETS *rets_RPC;
    rets_RPC = (struct IAudioFlinger_setParameters_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    status_t status = rets_RPC->status;

    size_t rets_RPC_size = sizeof(struct IAudioFlinger_setParameters_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);

    LsfVariant_clear(&var);
    free(args);

    return status;
}

struct IAudioFlinger_getParameters_ARGS {
    uint32_t target_binder;
    int funcID;

    audio_io_handle_t ioHandle;
    size_t keys_bytes;
};

struct IAudioFlinger_getParameters_RETS {
    size_t keyValuePairs_bytes;
};

String8 BpAudioFlinger_getParameters(IAudioFlinger *iAudioFlinger,
                                audio_io_handle_t ioHandle,
                                const String8 *keys)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioFlinger *self = container_of(iAudioFlinger, BpAudioFlinger, iiAudioFlinger);

    size_t args_RPC_size = sizeof(struct IAudioFlinger_getParameters_ARGS);
    size_t args_buf_size = String8_bytes(keys);
    uint8_t *args = malloc(args_RPC_size + args_buf_size);

    struct IAudioFlinger_getParameters_ARGS *args_RPC = (struct IAudioFlinger_getParameters_ARGS *)args;
    args_RPC->target_binder = self->binder;
    args_RPC->funcID = IAUDIOFLINGER_GET_PARAMETERS;
    args_RPC->ioHandle = ioHandle;
    args_RPC->keys_bytes = args_buf_size;

    void *args_buf = args + args_RPC_size;
    memcpy(args_buf, String8_string(keys), args_buf_size);

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf;
    var.u.memBuf.size = args_RPC_size + args_buf_size;
    var.u.memBuf.pData = args;

    struct IAudioFlinger_getParameters_RETS *rets_RPC;
    rets_RPC = (struct IAudioFlinger_getParameters_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    size_t rets_RPC_size = sizeof(struct IAudioFlinger_getParameters_RETS);
    size_t rets_buf_size = rets_RPC->keyValuePairs_bytes;

    String8 keyValuePairs;
    String8_ctor_char_len(&keyValuePairs, (const char *)rets_RPC + rets_RPC_size, rets_buf_size);

    IC_Sharemem_detach(rets_RPC, rets_RPC_size + rets_buf_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);

    LsfVariant_clear(&var);
    free(args);

    return keyValuePairs;
}

// 约定: 使用者保证入参
BpAudioFlinger* BpAudioFlinger_ctor(BpAudioFlinger* self, uint32_t binder)
{
    // 接口
    self->iiAudioFlinger.createTrack = BpAudioFlinger_createTrack;
    self->iiAudioFlinger.openRecord = BpAudioFlinger_openRecord;
    self->iiAudioFlinger.setMasterVolume = BpAudioFlinger_setMasterVolume;
    self->iiAudioFlinger.setMasterMute = BpAudioFlinger_setMasterMute;
    self->iiAudioFlinger.masterVolume = BpAudioFlinger_masterVolume;
    self->iiAudioFlinger.masterMute = BpAudioFlinger_masterMute;
    self->iiAudioFlinger.setParameters = BpAudioFlinger_setParameters;
    self->iiAudioFlinger.getParameters = BpAudioFlinger_getParameters;

    // 成员
    self->binder = binder;

    return self;
}

void BpAudioFlinger_dtor(BpAudioFlinger *self)
{
    (void) self;

    return;
}

// ----------------------------------------------------------------------

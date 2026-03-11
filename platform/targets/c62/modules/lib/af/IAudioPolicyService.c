
#define LOG_TAG "AudioPolicyService"

#include "IAudioPolicyService.h"

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
    IAUDIOPOLICYSERVICE_GETOUTPUT,
    IAUDIOPOLICYSERVICE_STARTOUTPUT,
    IAUDIOPOLICYSERVICE_STOPOUTPUT,
    IAUDIOPOLICYSERVICE_RELEASEOUTPUT,
    IAUDIOPOLICYSERVICE_GETINPUT,
    IAUDIOPOLICYSERVICE_STARTINPUT,
    IAUDIOPOLICYSERVICE_STOPINPUT,
    IAUDIOPOLICYSERVICE_RELEASEINPUT,
};

//------------------------------------------------
// BpAudioPolicyService header

typedef struct _BpAudioPolicyService BpAudioPolicyService;

struct _BpAudioPolicyService
{
    // interface instance
    IAudioPolicyService iiAudioPolicyService;

    uint32_t binder;
};

// singleton
static BpAudioPolicyService sBpAudioPolicyService;

// BpAudioPolicyService::Interface转换辅助函数
static inline
IAudioPolicyService* BpAudioPolicyService_as_IAudioPolicyService(BpAudioPolicyService *self)
{
    return (& self->iiAudioPolicyService);
}

BpAudioPolicyService* BpAudioPolicyService_ctor(BpAudioPolicyService* self, uint32_t binder);

void BpAudioPolicyService_dtor(BpAudioPolicyService *self);

audio_io_handle_t BpAudioPolicyService_getOutput(IAudioPolicyService *iAudioPolicyService,
                                                 int stream,
                                                 uint32_t samplingRate,
                                                 uint32_t format,
                                                 uint32_t channels,
                                                 uint32_t flags);

status_t BpAudioPolicyService_startOutput(IAudioPolicyService* self, audio_io_handle_t output, int stream);

status_t BpAudioPolicyService_stopOutput(IAudioPolicyService* self, audio_io_handle_t output, int stream);

void BpAudioPolicyService_releaseOutput(IAudioPolicyService* self, audio_io_handle_t output);

audio_io_handle_t BpAudioPolicyService_getInput(IAudioPolicyService* self,
                                                int inputSource,
                                                uint32_t samplingRate,
                                                uint32_t format,
                                                uint32_t channels,
                                                uint32_t acoustics);

status_t BpAudioPolicyService_startInput(IAudioPolicyService* self, audio_io_handle_t input);

status_t BpAudioPolicyService_stopInput(IAudioPolicyService* self, audio_io_handle_t input);

void BpAudioPolicyService_releaseInput(IAudioPolicyService* self, audio_io_handle_t input);

//------------------------------------------------
// IAudioPolicyService header


//------------------------------------------------
// IAudioPolicyService

// IAudioPolicyService::类静态函数
// 根据binder, 建立Bp/Bn实体对象, 返回interface
IAudioPolicyService* IAudioPolicyService_as_interface(uint32_t binder)
{
    BpAudioPolicyService* obj = BpAudioPolicyService_ctor(&sBpAudioPolicyService, binder);

    IAudioPolicyService *ret = BpAudioPolicyService_as_IAudioPolicyService(obj);

    return ret;
}

//------------------------------------------------
//------------------------------------------------
//------------------------------------------------
// BpAudioPolicyService method

struct IAudioPolicyService_ARGS {
    uint32_t target_binder;
    int funcID;
};

// IAUDIOPOLICYSERVICE_GETOUTPUT
struct IAudioPolicyService_getOutput_ARGS {
    uint32_t target_binder;
    int funcID;

    int stream;
    uint32_t samplingRate;
    uint32_t format;
    uint32_t channels;
    uint32_t flags;
};

struct IAudioPolicyService_getOutput_RETS {
    int output;
};

// IAUDIOPOLICYSERVICE_STARTOUTPUT
struct IAudioPolicyService_startOutput_ARGS {
    uint32_t target_binder;
    int funcID;

    int output;
    int stream;
};

struct IAudioPolicyService_startOutput_RETS {
    int status;
};

// IAUDIOPOLICYSERVICE_STOPOUTPUT
struct IAudioPolicyService_stopOutput_ARGS {
    uint32_t target_binder;
    int funcID;

    int output;
    int stream;
};

struct IAudioPolicyService_stopOutput_RETS {
    int status;
};

// IAUDIOPOLICYSERVICE_RELEASEOUTPUT
struct IAudioPolicyService_releaseOutput_ARGS {
    uint32_t target_binder;
    int funcID;

    int output;
};

struct IAudioPolicyService_releaseOutput_RETS {
    int status;
};

// IAUDIOPOLICYSERVICE_GETINPUT
struct IAudioPolicyService_getInput_ARGS {
    uint32_t target_binder;
    int funcID;

    int inputSource;
    uint32_t samplingRate;
    uint32_t format;
    uint32_t channels;
    uint32_t acoustics;
};

struct IAudioPolicyService_getInput_RETS {
    int input;
};

// IAUDIOPOLICYSERVICE_STARTINPUT
struct IAudioPolicyService_startInput_ARGS {
    uint32_t target_binder;
    int funcID;

    int input;
};

struct IAudioPolicyService_startInput_RETS {
    int status;
};

// IAUDIOPOLICYSERVICE_STOPINPUT
struct IAudioPolicyService_stopInput_ARGS {
    uint32_t target_binder;
    int funcID;

    int input;
};

struct IAudioPolicyService_stopInput_RETS {
    int status;
};

// IAUDIOPOLICYSERVICE_RELEASEINPUT
struct IAudioPolicyService_releaseInput_ARGS {
    uint32_t target_binder;
    int funcID;

    int input;
};

struct IAudioPolicyService_releaseInput_RETS {
    int status;
};

audio_io_handle_t BpAudioPolicyService_getOutput(IAudioPolicyService *iAudioPolicyService,
                                                 int stream,
                                                 uint32_t samplingRate,
                                                 uint32_t format,
                                                 uint32_t channels,
                                                 uint32_t flags)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioPolicyService *self = container_of(iAudioPolicyService, BpAudioPolicyService, iiAudioPolicyService);

    struct IAudioPolicyService_getOutput_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIOPOLICYSERVICE_GETOUTPUT,

        .stream = stream,
        .samplingRate = samplingRate,
        .format = format,
        .channels = channels,
        .flags = flags,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf; // 类型: 内存块
    var.u.memBuf.size = sizeof(struct IAudioPolicyService_getOutput_ARGS); // 内存块长度
    var.u.memBuf.pData = &args_RPC; // 填入param value

    // 远程返回 {--------------------------------------
    struct IAudioPolicyService_getOutput_RETS *rets_RPC;
    rets_RPC = (struct IAudioPolicyService_getOutput_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    // 远程返回值 - 读取
    CLOGD("[%s] output: %d\r\n", __func__, rets_RPC->output);

    audio_io_handle_t output = rets_RPC->output;

    size_t rets_RPC_size = sizeof(struct IAudioPolicyService_getOutput_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);
    // 远程返回 }--------------------------------------

    // 不涉及Variant本体的释放, 只清空其内部间接内存占用
    LsfVariant_clear(&var);

    // 远程返回值 - 使用
    return output;
}

status_t BpAudioPolicyService_startOutput(IAudioPolicyService *self,
                                          audio_io_handle_t output, int stream)
{
    return 0;
}

status_t BpAudioPolicyService_stopOutput(IAudioPolicyService *self,
                                         audio_io_handle_t output, int stream)
{
    return 0;
}

void BpAudioPolicyService_releaseOutput(IAudioPolicyService *self, audio_io_handle_t output)
{
    return;
}

audio_io_handle_t BpAudioPolicyService_getInput(IAudioPolicyService* iAudioPolicyService,
                                                int inputSource,
                                                uint32_t samplingRate,
                                                uint32_t format,
                                                uint32_t channels,
                                                uint32_t acoustics)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioPolicyService *self = container_of(iAudioPolicyService, BpAudioPolicyService, iiAudioPolicyService);

    struct IAudioPolicyService_getInput_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIOPOLICYSERVICE_GETINPUT,

        .inputSource = inputSource,
        .samplingRate = samplingRate,
        .format = format,
        .channels = channels,
        .acoustics = acoustics,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf;
    var.u.memBuf.size = sizeof(struct IAudioPolicyService_getInput_ARGS);
    var.u.memBuf.pData = &args_RPC;

    // 远程返回 {--------------------------------------
    struct IAudioPolicyService_getInput_RETS *rets_RPC;
    rets_RPC = (struct IAudioPolicyService_getInput_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    // 远程返回值 - 读取
    CLOGD("[%s] input: %d\r\n", __func__, rets_RPC->input);

    audio_io_handle_t input = rets_RPC->input;

    size_t rets_RPC_size = sizeof(struct IAudioPolicyService_getInput_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);
    // 远程返回 }--------------------------------------

    LsfVariant_clear(&var);

    return input;
}

status_t BpAudioPolicyService_startInput(IAudioPolicyService* self, audio_io_handle_t input)
{
    return 0;
}

status_t BpAudioPolicyService_stopInput(IAudioPolicyService* self, audio_io_handle_t input)
{
    return 0;
}

void BpAudioPolicyService_releaseInput(IAudioPolicyService* self, audio_io_handle_t input)
{
    return;
}

// 约定: 使用者保证入参
BpAudioPolicyService* BpAudioPolicyService_ctor(BpAudioPolicyService* self, uint32_t binder)
{
    // 接口
    self->iiAudioPolicyService.getOutput = BpAudioPolicyService_getOutput;
    self->iiAudioPolicyService.startOutput = BpAudioPolicyService_startOutput;
    self->iiAudioPolicyService.stopOutput = BpAudioPolicyService_stopOutput;
    self->iiAudioPolicyService.releaseOutput = BpAudioPolicyService_releaseOutput;
    self->iiAudioPolicyService.getInput = BpAudioPolicyService_getInput;
    self->iiAudioPolicyService.startInput = BpAudioPolicyService_startInput;
    self->iiAudioPolicyService.stopInput = BpAudioPolicyService_stopInput;
    self->iiAudioPolicyService.releaseInput = BpAudioPolicyService_releaseInput;

    // 成员
    self->binder = binder;

    return self;
}

void BpAudioPolicyService_dtor(BpAudioPolicyService *self)
{
    (void) self;

    return;
}

// ----------------------------------------------------------------------

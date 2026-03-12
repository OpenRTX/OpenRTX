#define LOG_TAG "IAudioTrack"
//#define LOG_NDEBUG 0

#include "IAudioTrack.h"

#include "base.h"

//------------------------------------------------
#include "lsf.h"
#include "audioflinger_property_def.h"

//------------------------------------------------
#include "ic_common.h"
#include "venus_log.h"

#include "FreeRTOS.h"

//------------------------------------------------
// function IDs
enum {
    IAUDIOTRACK_GET_CBLK,
    IAUDIOTRACK_START,
    IAUDIOTRACK_STOP,
    IAUDIOTRACK_FLUSH,
    IAUDIOTRACK_MUTE,
    IAUDIOTRACK_PAUSE,
};

//------------------------------------------------
// BpAudioTrack header

typedef struct _BpAudioTrack BpAudioTrack;

struct _BpAudioTrack
{
    // interface instance
    IAudioTrack iiAudioTrack;

    uint32_t binder;
};

// BpAudioTrack::Interface转换辅助函数
static inline
IAudioTrack* BpAudioTrack_as_IAudioTrack(BpAudioTrack *self)
{
    return (& self->iiAudioTrack);
}

BpAudioTrack* BpAudioTrack_ctor(BpAudioTrack* self, uint32_t binder);

void BpAudioTrack_dtor(RefBase* selfBase);

void* BpAudioTrack_getCblk(IAudioTrack *iAudioTrack);

//------------------------------------------------
// IAudioTrack header


//------------------------------------------------
// IAudioTrack

// IAudioTrack::类静态函数
// 根据binder, 建立Bp/Bn实体对象, 返回interface
IAudioTrack* IAudioTrack_as_interface(uint32_t binder)
{
    if (binder == BINDER_INVALID_PORT) {
        return NULL;
    }

    BpAudioTrack* obj = (BpAudioTrack*) pvPortMalloc(sizeof(BpAudioTrack));
    obj = BpAudioTrack_ctor(obj, binder);

    IAudioTrack *ret = BpAudioTrack_as_IAudioTrack(obj);

    return ret;
}

//------------------------------------------------
//------------------------------------------------
//------------------------------------------------
// BpAudioTrack method

struct IAudioTrack_ARGS {
    uint32_t target_binder;
    int funcID;
};

// IAUDIOTRACK_GET_CBLK
struct IAudioTrack_getCblk_ARGS {
    uint32_t target_binder;
    int funcID;
};

struct IAudioTrack_getCblk_RETS {
    void* cblk;
};

// IAUDIOTRACK_START
struct IAudioTrack_start_ARGS {
    uint32_t target_binder;
    int funcID;
};

struct IAudioTrack_start_RETS {
    int status;
};

// IAUDIOTRACK_PAUSE
struct IAudioTrack_pause_ARGS {
    uint32_t target_binder;
    int funcID;
};

struct IAudioTrack_pause_RETS {
    int status;
};

// IAUDIOTRACK_STOP
struct IAudioTrack_stop_ARGS {
    uint32_t target_binder;
    int funcID;
};

struct IAudioTrack_stop_RETS {
    int status;
};

// IAUDIOTRACK_FLUSH
struct IAudioTrack_flush_ARGS {
    uint32_t target_binder;
    int funcID;
};

struct IAudioTrack_flush_RETS {
    int status;
};

// IAUDIOTRACK_MUTE
struct IAudioTrack_mute_ARGS {
    uint32_t target_binder;
    int funcID;

    int bMute;
};

struct IAudioTrack_mute_RETS {
    int status;
};

//------------------------------------------------
// 返回binder
void* BpAudioTrack_getCblk(IAudioTrack *iAudioTrack)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioTrack *self = container_of(iAudioTrack, BpAudioTrack, iiAudioTrack);

    struct IAudioTrack_getCblk_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIOTRACK_GET_CBLK,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf; // 类型: 内存块
    var.u.memBuf.size = sizeof(struct IAudioTrack_getCblk_ARGS); // 内存块长度
    var.u.memBuf.pData = &args_RPC; // 填入param value

    // 远程返回 {--------------------------------------
    struct IAudioTrack_getCblk_RETS *rets_RPC;
    rets_RPC = (struct IAudioTrack_getCblk_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    // 远程返回值 - 读取
    CLOGD("[%s] cblk: %p\r\n", __func__, rets_RPC->cblk);

    void *retCblk = rets_RPC->cblk;

    size_t rets_RPC_size = sizeof(struct IAudioTrack_getCblk_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);
    // 远程返回 }--------------------------------------

    // 不涉及Variant本体的释放, 只清空其内部间接内存占用
    LsfVariant_clear(&var);

    return retCblk;
}

status_t BpAudioTrack_start(IAudioTrack *iAudioTrack)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioTrack *self = container_of(iAudioTrack, BpAudioTrack, iiAudioTrack);

    struct IAudioTrack_start_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIOTRACK_START,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf; // 类型: 内存块
    var.u.memBuf.size = sizeof(struct IAudioTrack_start_ARGS); // 内存块长度
    var.u.memBuf.pData = &args_RPC; // 填入param value

    // 远程返回 {--------------------------------------
    struct IAudioTrack_start_RETS *rets_RPC;
    rets_RPC = (struct IAudioTrack_start_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    // 远程返回值 - 读取
    CLOGD("[%s] status: %d\r\n", __func__, rets_RPC->status);

    status_t status = rets_RPC->status;

    size_t rets_RPC_size = sizeof(struct IAudioTrack_start_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);
    // 远程返回 }--------------------------------------

    // 不涉及Variant本体的释放, 只清空其内部间接内存占用
    LsfVariant_clear(&var);

    return status;
}

void BpAudioTrack_stop(IAudioTrack *iAudioTrack)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioTrack *self = container_of(iAudioTrack, BpAudioTrack, iiAudioTrack);

    struct IAudioTrack_stop_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIOTRACK_STOP,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf; // 类型: 内存块
    var.u.memBuf.size = sizeof(struct IAudioTrack_stop_ARGS); // 内存块长度
    var.u.memBuf.pData = &args_RPC; // 填入param value

    // 远程返回 {--------------------------------------
    struct IAudioTrack_stop_RETS *rets_RPC;
    rets_RPC = (struct IAudioTrack_stop_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    // 远程返回值 - 读取
    CLOGD("[%s] status: %d\r\n", __func__, rets_RPC->status);

    status_t status = rets_RPC->status;

    size_t rets_RPC_size = sizeof(struct IAudioTrack_stop_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);
    // 远程返回 }--------------------------------------

    // 不涉及Variant本体的释放, 只清空其内部间接内存占用
    LsfVariant_clear(&var);

    return;
}

void BpAudioTrack_flush(IAudioTrack *iAudioTrack)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioTrack *self = container_of(iAudioTrack, BpAudioTrack, iiAudioTrack);

    struct IAudioTrack_flush_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIOTRACK_FLUSH,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf; // 类型: 内存块
    var.u.memBuf.size = sizeof(struct IAudioTrack_flush_ARGS); // 内存块长度
    var.u.memBuf.pData = &args_RPC; // 填入param value

    // 远程返回 {--------------------------------------
    struct IAudioTrack_flush_RETS *rets_RPC;
    rets_RPC = (struct IAudioTrack_flush_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    // 远程返回值 - 读取
    CLOGD("[%s] status: %d\r\n", __func__, rets_RPC->status);

    status_t status = rets_RPC->status;

    size_t rets_RPC_size = sizeof(struct IAudioTrack_flush_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);
    // 远程返回 }--------------------------------------

    // 不涉及Variant本体的释放, 只清空其内部间接内存占用
    LsfVariant_clear(&var);

    return;
}

void BpAudioTrack_pause(IAudioTrack *iAudioTrack)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioTrack *self = container_of(iAudioTrack, BpAudioTrack, iiAudioTrack);

    struct IAudioTrack_pause_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIOTRACK_PAUSE,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf; // 类型: 内存块
    var.u.memBuf.size = sizeof(struct IAudioTrack_pause_ARGS); // 内存块长度
    var.u.memBuf.pData = &args_RPC; // 填入param value

    // 远程返回 {--------------------------------------
    struct IAudioTrack_pause_RETS *rets_RPC;
    rets_RPC = (struct IAudioTrack_pause_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    // 远程返回值 - 读取
    CLOGD("[%s] status: %d\r\n", __func__, rets_RPC->status);

    status_t status = rets_RPC->status;

    size_t rets_RPC_size = sizeof(struct IAudioTrack_pause_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);
    // 远程返回 }--------------------------------------

    // 不涉及Variant本体的释放, 只清空其内部间接内存占用
    LsfVariant_clear(&var);

    return;
}

void BpAudioTrack_mute(IAudioTrack *iAudioTrack, bool bMute)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioTrack *self = container_of(iAudioTrack, BpAudioTrack, iiAudioTrack);

    struct IAudioTrack_mute_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIOTRACK_MUTE,

        .bMute = bMute,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf; // 类型: 内存块
    var.u.memBuf.size = sizeof(struct IAudioTrack_mute_ARGS); // 内存块长度
    var.u.memBuf.pData = &args_RPC; // 填入param value

    // 远程返回 {--------------------------------------
    struct IAudioTrack_mute_RETS *rets_RPC;
    rets_RPC = (struct IAudioTrack_mute_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    // 远程返回值 - 读取
    CLOGD("[%s] status: %d\r\n", __func__, rets_RPC->status);

    status_t status = rets_RPC->status;

    size_t rets_RPC_size = sizeof(struct IAudioTrack_mute_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);
    // 远程返回 }--------------------------------------

    // 不涉及Variant本体的释放, 只清空其内部间接内存占用
    LsfVariant_clear(&var);

    return;
}

// 约定: 使用者保证入参
BpAudioTrack* BpAudioTrack_ctor(BpAudioTrack* self, uint32_t binder)
{
    // base constructor
    RefBase_ctor((RefBase*)self);

    // 成员
    self->binder = binder;

    // 接口
    self->iiAudioTrack.getCblk = BpAudioTrack_getCblk;
    self->iiAudioTrack.start = BpAudioTrack_start;
    self->iiAudioTrack.stop = BpAudioTrack_stop;
    self->iiAudioTrack.flush = BpAudioTrack_flush;
    self->iiAudioTrack.mute = BpAudioTrack_mute;
    self->iiAudioTrack.pause = BpAudioTrack_pause;

    ((RefBase*)self)->dtor = BpAudioTrack_dtor;

    return self;
}

void BpAudioTrack_dtor(RefBase* selfBase)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioTrack *self = (BpAudioTrack*)selfBase;

    struct IBinder_unref_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IBINDER_FUNCID_UNREF,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf; // 类型: 内存块
    var.u.memBuf.size = sizeof(struct IBinder_unref_ARGS); // 内存块长度
    var.u.memBuf.pData = &args_RPC; // 填入param value

    // 远程返回 {--------------------------------------
    struct IBinder_unref_RETS *rets_RPC;
    rets_RPC = (struct IBinder_unref_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    // 远程返回值 - 读取
    CLOGD("[%s] status: %d\r\n", __func__, rets_RPC->status);

    status_t status = rets_RPC->status;

    size_t rets_RPC_size = sizeof(struct IBinder_unref_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);
    // 远程返回 }--------------------------------------

    // 不涉及Variant本体的释放, 只清空其内部间接内存占用
    LsfVariant_clear(&var);

    // base dtor
    // RefBase_dtor(selfBase);

    return;
}

// ----------------------------------------------------------------------

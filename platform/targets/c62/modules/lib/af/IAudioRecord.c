#define LOG_TAG "IAudioRecord"

#include "IAudioRecord.h"

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
    IAUDIORECORD_GET_CBLK,
    IAUDIORECORD_START,
    IAUDIORECORD_STOP,
};

//------------------------------------------------
// BpAudioRecord header

typedef struct _BpAudioRecord BpAudioRecord;

struct _BpAudioRecord
{
    // interface instance
    IAudioRecord iiAudioRecord;

    uint32_t binder;
};

static inline
IAudioRecord* BpAudioRecord_as_IAudioRecord(BpAudioRecord *self)
{
    return &self->iiAudioRecord;
}

BpAudioRecord* BpAudioRecord_ctor(BpAudioRecord* self, uint32_t binder);

void BpAudioRecord_dtor(RefBase* selfBase);

void* BpAudioRecord_getCblk(IAudioRecord *iAudioRecord);

//------------------------------------------------
// IAudioRecord header

//------------------------------------------------
// IAudioRecord

IAudioRecord* IAudioRecord_as_interface(uint32_t binder)
{
    if (binder == BINDER_INVALID_PORT) {
        return NULL;
    }

    BpAudioRecord* obj = (BpAudioRecord*) pvPortMalloc(sizeof(BpAudioRecord));
    obj = BpAudioRecord_ctor(obj, binder);

    IAudioRecord *ret = BpAudioRecord_as_IAudioRecord(obj);

    return ret;
}

//------------------------------------------------
// BpAudioRecord method

struct IAudioRecord_ARGS {
    uint32_t target_binder;
    int funcID;
};

// IAUDIORECORD_GET_CBLK
struct IAudioRecord_getCblk_ARGS {
    uint32_t target_binder;
    int funcID;
};

struct IAudioRecord_getCblk_RETS {
    void* cblk;
};

// IAUDIORECORD_START
struct IAudioRecord_start_ARGS {
    uint32_t target_binder;
    int funcID;
};

struct IAudioRecord_start_RETS {
    int status;
};

// IAUDIORECORD_STOP
struct IAudioRecord_stop_ARGS {
    uint32_t target_binder;
    int funcID;
};

struct IAudioRecord_stop_RETS {
    int status;
};

//------------------------------------------------

void* BpAudioRecord_getCblk(IAudioRecord *iAudioRecord)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioRecord *self = container_of(iAudioRecord, BpAudioRecord, iiAudioRecord);

    struct IAudioRecord_getCblk_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIORECORD_GET_CBLK,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf;
    var.u.memBuf.size = sizeof(struct IAudioRecord_getCblk_ARGS);
    var.u.memBuf.pData = &args_RPC;

    // 远程返回 {--------------------------------------
    struct IAudioRecord_getCblk_RETS *rets_RPC;
    rets_RPC = (struct IAudioRecord_getCblk_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    // 远程返回值 - 读取
    CLOGD("[%s] cblk: %p\r\n", __func__, rets_RPC->cblk);

    void *retCblk = rets_RPC->cblk;

    size_t rets_RPC_size = sizeof(struct IAudioRecord_getCblk_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);
    // 远程返回 }--------------------------------------

    // 不涉及Variant本体的释放, 只清空其内部间接内存占用
    LsfVariant_clear(&var);

    return retCblk;
}

status_t BpAudioRecord_start(IAudioRecord *iAudioRecord)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioRecord *self = container_of(iAudioRecord, BpAudioRecord, iiAudioRecord);

    struct IAudioRecord_start_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIORECORD_START,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf;
    var.u.memBuf.size = sizeof(struct IAudioRecord_start_ARGS);
    var.u.memBuf.pData = &args_RPC;

    // 远程返回 {--------------------------------------
    struct IAudioRecord_start_RETS *rets_RPC;
    rets_RPC = (struct IAudioRecord_start_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    // 远程返回值 - 读取
    CLOGD("[%s] status: %d\r\n", __func__, rets_RPC->status);

    status_t status = rets_RPC->status;

    size_t rets_RPC_size = sizeof(struct IAudioRecord_start_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);
    // 远程返回 }--------------------------------------

    // 不涉及Variant本体的释放, 只清空其内部间接内存占用
    LsfVariant_clear(&var);

    return status;
}

void BpAudioRecord_stop(IAudioRecord *iAudioRecord)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioRecord *self = container_of(iAudioRecord, BpAudioRecord, iiAudioRecord);

    struct IAudioRecord_stop_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IAUDIORECORD_STOP,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf;
    var.u.memBuf.size = sizeof(struct IAudioRecord_stop_ARGS);
    var.u.memBuf.pData = &args_RPC;

    // 远程返回 {--------------------------------------
    struct IAudioRecord_stop_RETS *rets_RPC;
    rets_RPC = (struct IAudioRecord_stop_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    // 远程返回值 - 读取
    CLOGD("[%s] status: %d\r\n", __func__, rets_RPC->status);

    status_t status = rets_RPC->status;

    size_t rets_RPC_size = sizeof(struct IAudioRecord_stop_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);
    // 远程返回 }--------------------------------------

    // 不涉及Variant本体的释放, 只清空其内部间接内存占用
    LsfVariant_clear(&var);

    return;
}

BpAudioRecord* BpAudioRecord_ctor(BpAudioRecord* self, uint32_t binder)
{
    // base consturctor
    RefBase_ctor((RefBase*)self);

    // 成员
    self->binder = binder;

    // 接口
    self->iiAudioRecord.getCblk = BpAudioRecord_getCblk;
    self->iiAudioRecord.start = BpAudioRecord_start;
    self->iiAudioRecord.stop = BpAudioRecord_stop;

    ((RefBase*)self)->dtor = BpAudioRecord_dtor;

    return self;
}

void BpAudioRecord_dtor(RefBase* selfBase)
{
    CLOGD("[%s]\r\n", __func__);

    BpAudioRecord *self = (BpAudioRecord*)selfBase;

    struct IBinder_unref_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = IBINDER_FUNCID_UNREF,
    };

    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf;
    var.u.memBuf.size = sizeof(struct IBinder_unref_ARGS);
    var.u.memBuf.pData = &args_RPC;

    // 远程返回 {--------------------------------------
    struct IBinder_unref_RETS *rets_RPC;
    rets_RPC = (struct IBinder_unref_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    CLOGD("[%s] status: %d\r\n", __func__, rets_RPC->status);

    status_t status = rets_RPC->status;

    size_t rets_RPC_size = sizeof(struct IBinder_unref_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);
    // 远程返回 }--------------------------------------

    LsfVariant_clear(&var);
}


#define LOG_TAG "ServiceManager"

#include "IServiceManager.h"

#include "base.h"

//------------------------------------------------
#include "lsf.h"

#include "audioflinger_property_def.h"

//------------------------------------------------
#include "venus_log.h"

//------------------------------------------------
// function IDs
enum {
    ISERVICEMANAGER_GETSERVICE,

};

//------------------------------------------------
// main开始全局初始化

int IServiceManager_Class_init(void)
{
    // TODO
    // gDefaultServiceManagerLock;

    return 0;
}

// BpServiceManager-------------------------------------

typedef struct _BpServiceManager BpServiceManager;

struct _BpServiceManager
{
    // interface instance
    IServiceManager iiServiceManager;

    uint32_t binder;
};

static BpServiceManager sBpServiceManager;

// BpServiceManager::Interface转换辅助函数
static inline
IServiceManager* BpServiceManager_as_IServiceManager(BpServiceManager *self)
{
    return (& self->iiServiceManager);
}

//------------------------------------------------

struct IServiceManager_getService_ARGS {
    uint32_t target_binder;
    int funcID;

    uint32_t name;
};

struct IServiceManager_getService_RETS {
    int status;

    uint32_t binder;
};

// 返回binder
uint32_t BpServiceManager_getService(IServiceManager *iServiceManager, uint32_t name)
{
    CLOGD("[%s] name: %d\r\n", __func__, name);

    BpServiceManager *self = container_of(iServiceManager, BpServiceManager, iiServiceManager);

    // TODO: 直接IC_Alloc => var.type(uint32)
    struct IServiceManager_getService_ARGS args_RPC = {
        .target_binder = self->binder,
        .funcID = ISERVICEMANAGER_GETSERVICE,

        .name = name,
    };
    LsfVariant var;
    LsfVariant_init(&var);
    var.variant_type = type_membuf; // 类型: 内存块
    var.u.memBuf.size = sizeof(struct IServiceManager_getService_ARGS); // 内存块长度
    var.u.memBuf.pData = &args_RPC; // 填入param value

    // 远程返回 {--------------------------------------
    struct IServiceManager_getService_RETS *rets_RPC;
    rets_RPC = (struct IServiceManager_getService_RETS *)
                lsf_set(GROUP_ID(self->binder), PROPERTY_ID(self->binder), &var);

    // 远程返回值 - 读取
    CLOGD("[%s] status: %d, binder: %d - group: %d, property: %d, interface: %d\r\n", __func__,
          rets_RPC->status, rets_RPC->binder, GROUP_ID(rets_RPC->binder),
          PROPERTY_ID(rets_RPC->binder), INTERFACE_ID(rets_RPC->binder));

    uint32_t binder = rets_RPC->binder;

    size_t rets_RPC_size = sizeof(struct IServiceManager_getService_RETS);
    IC_Sharemem_detach(rets_RPC, rets_RPC_size);
    IC_Allocator_free(g_pFreeer, rets_RPC);
    // 远程返回 }--------------------------------------

    // 不涉及Variant本体的释放, 只清空其内部间接内存占用
    LsfVariant_clear(&var);

    // 远程返回值 - 使用

    return binder;
}

// 约定: 使用者保证入参
BpServiceManager* BpServiceManager_ctor(BpServiceManager* self, uint32_t binder)
{
    // 接口
    self->iiServiceManager.getService = BpServiceManager_getService;

    // 成员
    self->binder = binder;

    return self;
}

void BpServiceManager_dtor(BpServiceManager *self)
{
    (void) self;

    return;
}

// IServiceManager::类静态函数
// 根据binder, 建立Bp/Bn实体对象, 返回interface
IServiceManager* IServiceManager_as_interface(uint32_t binder)
{
    BpServiceManager* obj = BpServiceManager_ctor(&sBpServiceManager, binder);

    IServiceManager *ret = BpServiceManager_as_IServiceManager(obj);

    return ret;
}

//------------------------------------------------
static IServiceManager* gDefaultServiceManager = NULL;

IServiceManager* defaultServiceManager()
{
    // TODO
    // AutoMutex _lock(gDefaultServiceManagerLock);

    if (gDefaultServiceManager == NULL) {
        uint32_t binder = BINDER_U32(PROPERTY_GROUP_ID_AUDIOFLINGER,
                                     ISERVICEMANAGER_PROPERTY,
                                     0);

        gDefaultServiceManager = interface_cast_IServiceManager(binder);
    }

    // TODO
    // unlock;
    
    return gDefaultServiceManager;
}



// ----------------------------------------------------------------------

#if 0

class BpServiceManager : public BpInterface<IServiceManager>
{
public:
    BpServiceManager(const sp<IBinder>& impl)
        : BpInterface<IServiceManager>(impl)
    {
    }
        
    virtual sp<IBinder> getService(const String16& name) const
    {
        unsigned n;
        for (n = 0; n < 5; n++){
            sp<IBinder> svc = checkService(name);
            if (svc != NULL) return svc;
            LOGI("Waiting for sevice %s...\n", String8(name).string());
            sleep(1);
        }
        return NULL;
    }
    
    virtual sp<IBinder> checkService( const String16& name) const
    {
        Parcel data, reply;
        data.writeInterfaceToken(IServiceManager::getInterfaceDescriptor());
        data.writeString16(name);
        remote()->transact(CHECK_SERVICE_TRANSACTION, data, &reply);
        return reply.readStrongBinder();
    }

};

#endif

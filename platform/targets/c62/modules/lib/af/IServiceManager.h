//
#ifndef ANDROID_ISERVICE_MANAGER_H
#define ANDROID_ISERVICE_MANAGER_H

// 标准库
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ----------------------------------------------------------------------

typedef struct _IServiceManager IServiceManager;

struct _IServiceManager
{
    /**
     * Retrieve an existing service, blocking for a few seconds
     * if it doesn't yet exist.
     */
    uint32_t         (*getService)(IServiceManager *iServiceManager, uint32_t name);

};

//------------------------------------------------
// helper
IServiceManager* defaultServiceManager();

//------------------------------------------------
// 类静态函数
int IServiceManager_Class_init(void);

IServiceManager* IServiceManager_as_interface(uint32_t binder);

// binder => IInterface
static inline
IServiceManager* interface_cast_IServiceManager(uint32_t binder)
{
    return IServiceManager_as_interface(binder);
}



#if 0
status_t getService(uint32_t serviceID, uint32_t outService)
{
    const sp<IServiceManager> sm = defaultServiceManager();
    if (sm != NULL) {
        *outService = interface_cast<INTERFACE>(sm->getService(name));
        if ((*outService) != NULL) return NO_ERROR;
    }
    return NAME_NOT_FOUND;
}
#endif

// ----------------------------------------------------------------------

#endif // ANDROID_ISERVICE_MANAGER_H

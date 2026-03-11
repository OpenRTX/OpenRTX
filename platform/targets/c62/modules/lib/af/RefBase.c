#include "RefBase.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>

#include "FreeRTOS.h"

//------------------------------------------------
int32_t RefBase_decStrong(RefBase* self)
{
    if (RefBase_onlyOwner(self)) {
        // Since we're the only owner, our reference count goes to zero.
        atomic_store_explicit(&self->mRefs, 0, memory_order_relaxed);

        RefBase_dtor(self);
        vPortFree((void*) self);

        // As the only owner, our previous reference count was 1.
        return 1;
    }
    // There's multiple owners, we need to use an atomic decrement.
    int32_t prevRefCount = atomic_fetch_sub_explicit(&self->mRefs, 1,
                                                     memory_order_release);
    if (prevRefCount == 1) {
        // We're the last reference, we need the acquire fence.
        atomic_thread_fence(memory_order_acquire);

        RefBase_dtor(self);
        vPortFree((void*) self);
    }
    return prevRefCount;
}

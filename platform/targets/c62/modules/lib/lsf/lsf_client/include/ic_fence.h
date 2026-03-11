#ifndef __IC_FENCE_H__
#define __IC_FENCE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ic_common.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct ICFence;

typedef struct ICFence* ICFenceHandle;

ICFenceHandle
ICFence_Creator_createObj(int remoteFenceObjID);

// ICFenceHandle
// ICFence_ctor(ICFenceHandle self, int objID);

int
ICFence_dtor(ICFenceHandle self);

int
ICFence_syncWithRemote(ICFenceHandle self);

int
ICFence_wait(ICFenceHandle self, uint32_t *outMsg);

int
ICFence_notify(ICFenceHandle self, uint32_t msg);

#ifdef __cplusplus
}
#endif

#endif /* __IC_FENCE_H__ */

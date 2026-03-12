#ifndef AUDIOSERVICE_IAUDIORECORD_H
#define AUDIOSERVICE_IAUDIORECORD_H

#include "RefBase.h"

#include <stdint.h>

#include "base.h"
#include "Errors.h"

// ----------------------------------------------------------------------------

typedef struct _IAudioRecord IAudioRecord;

struct _IAudioRecord {
    RefBase base;

    status_t    (* start)(IAudioRecord *iAudioRecord);
    void        (* stop)(IAudioRecord *iAudioRecord);

    void*       (* getCblk)(IAudioRecord *iAudioRecord);
};

static inline status_t IAudioRecord_start(IAudioRecord *iAudioRecord)
{
    return iAudioRecord->start(iAudioRecord);
}

static inline void IAudioRecord_stop(IAudioRecord *iAudioRecord)
{
    iAudioRecord->stop(iAudioRecord);
}

static inline void* IAudioRecord_getCblk(IAudioRecord *iAudioRecord)
{
    return iAudioRecord->getCblk(iAudioRecord);
}

//----------------------------------------------------------------------------
// interface helper

IAudioRecord* IAudioRecord_as_interface(uint32_t binder);

// binder => IInterface
static inline
IAudioRecord* interface_cast_IAudioRecord(uint32_t binder)
{
    return IAudioRecord_as_interface(binder);
}

#endif // AUDIOSERVICE_IAUDIORECORD_H

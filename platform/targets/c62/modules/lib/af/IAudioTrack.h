#ifndef AUDIOSERVICE_IAUDIOTRACK_H
#define AUDIOSERVICE_IAUDIOTRACK_H

#include "RefBase.h"

#include <stdint.h>

#include "base.h"
#include "Errors.h"

// ----------------------------------------------------------------------------

typedef struct _IAudioTrack IAudioTrack;

struct _IAudioTrack {
    RefBase base;

    /* After it's created the track is not active. Call start() to
     * make it active. If set, the callback will start being called.
     */
    status_t    (* start)(IAudioTrack *iAudioTrack);

    /* Stop a track. If set, the callback will cease being called and
     * obtainBuffer will return an error. Buffers that are already released
     * will be processed, unless flush() is called.
     */
    void        (* stop)(IAudioTrack *iAudioTrack);

    /* flush a stopped track. All pending buffers are discarded.
     * This function has no effect if the track is not stoped.
     */
    void        (* flush)(IAudioTrack *iAudioTrack);

    /* mute or unmutes this track.
     * While mutted, the callback, if set, is still called.
     */
    void        (* mute)(IAudioTrack *iAudioTrack, bool bMute);

    /* Pause a track. If set, the callback will cease being called and
     * obtainBuffer will return an error. Buffers that are already released
     * will be processed, unless flush() is called.
     */
    void        (* pause)(IAudioTrack *iAudioTrack);

    /* get this tracks control block */
    void *      (* getCblk)(IAudioTrack *iAudioTrack);
};

static inline status_t IAudioTrack_start(IAudioTrack *iAudioTrack)
{
    return (iAudioTrack->start(iAudioTrack));
}

static inline void IAudioTrack_stop(IAudioTrack *iAudioTrack)
{
    return (iAudioTrack->stop(iAudioTrack));
}

static inline void IAudioTrack_flush(IAudioTrack *iAudioTrack)
{
    return (iAudioTrack->flush(iAudioTrack));
}

static inline void IAudioTrack_mute(IAudioTrack *iAudioTrack, bool bMute)
{
    return (iAudioTrack->mute(iAudioTrack, bMute));
}

static inline void IAudioTrack_pause(IAudioTrack *iAudioTrack)
{
    return (iAudioTrack->pause(iAudioTrack));
}

static inline void * IAudioTrack_getCblk(IAudioTrack *iAudioTrack)
{
    return (iAudioTrack->getCblk(iAudioTrack));
}

//----------------------------------------------------------------------------
// interface helper

IAudioTrack* IAudioTrack_as_interface(uint32_t binder);

// binder => IInterface
static inline
IAudioTrack* interface_cast_IAudioTrack(uint32_t binder)
{

    return IAudioTrack_as_interface(binder);
}

#endif // AUDIOSERVICE_IAUDIOTRACK_H

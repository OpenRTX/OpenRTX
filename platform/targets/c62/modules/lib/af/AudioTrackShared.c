#include "AudioTrackShared.h"

//------------------------------------------------
// platform
#include "cache.h"

#include <cmsis_gcc.h>

// utils:
#include "ic_common.h"

//------------------------------------------------
// lib: clib
#include <stdint.h>

void audio_track_cblk_t_detach(audio_track_cblk_t* self)
{
    // arm: 可能write back; invalidate cache
    dcache_invalidate_range((unsigned long) self,
                            ((unsigned long) self) + IC_DCACHELINE_ROUNDUP_SIZE(sizeof(audio_track_cblk_t)));

    // synchronization & memory barrier: write_release. full system, any-any = 15
    // TODO: 因为不确定dcache invalidate算load还是store, 保险起见. any-any.
    __DSB();

    return;
}

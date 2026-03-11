#ifndef DROID_AUDIO_TRACK_SHARED_H
#define DROID_AUDIO_TRACK_SHARED_H

//------------------------------------------------
// utils: threads
#include "threads.h"

//------------------------------------------------
// platform

// utils: ic_assert
//#define ASSERT_NDEBUG 0
#include "ic_common.h"

//------------------------------------------------
// lib: clib
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>

#include <limits.h>

// ----------------------------------------------------------------------------

#define THREAD_PRIORITY_AUDIO_CLIENT (DROID_PRIORITY_AUDIO)

// Maximum cumulated timeout milliseconds before restarting audioflinger thread
#define MAX_STARTUP_TIMEOUT_MS  3000    // Longer timeout period at startup to cope with A2DP init time
#define MAX_RUN_TIMEOUT_MS      1000
#define WAIT_PERIOD_MS          10

typedef struct _audio_track_cblk_t audio_track_cblk_t;

struct _audio_track_cblk_t
{

    // The data members are grouped so that members accessed frequently and in the same context
    // are in the same line of data cache.
    //             Mutex       lock;
    //             Condition   cv;
    // volatile    uint32_t    user;
    // volatile    uint32_t    server;
    //             uint32_t    userBase;
    //             uint32_t    serverBase;
    // void*       buffers;
    uint32_t    __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) frameCount;
    int         __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) streamID; // ic stream ID
    // Cache line boundary
    uint32_t    __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) loopStart;
    uint32_t    __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) loopEnd;
    int         __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) loopCount;
    volatile    union {
                    uint16_t    __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) volume[2];
                    uint32_t    __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) volumeLR;
                };
                uint32_t    __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) sampleRate;
                // NOTE: audio_track_cblk_t::frameSize is not equal to AudioTrack::frameSize() for
                // 8 bit PCM data: in this case,  mCblk->frameSize is based on a sample size of
                // 16 bit because data is converted to 16 bit before being stored in buffer
                uint32_t    __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) frameSize;
                uint8_t     __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) channels;
                uint8_t     __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) flowControlFlag; // underrun (out) or overrrun (in) indication
                uint8_t     __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) out;        // out equals 1 for AudioTrack and 0 for AudioRecord

                uint8_t     __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) forceReady;
                uint16_t    __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) bufferTimeoutMs; // Maximum cumulated timeout before restarting audioflinger
                uint16_t    __attribute__ ((aligned (IC_MAX_DCACHE_LINESIZE))) waitTimeMs;      // Cumulated wait time
                // Cache line boundary (32 bytes)
};

// class method
audio_track_cblk_t* audio_track_cblk_t_allocObj(void);

// obj method
static inline
audio_track_cblk_t* audio_track_cblk_t_ctor(audio_track_cblk_t* self)
{
    self->frameCount = 0;
    self->loopStart = UINT_MAX;
    self->loopEnd = UINT_MAX;
    self->loopCount = 0;
    self->volumeLR = 0;
    self->flowControlFlag = 1;
    self->forceReady = 0;

    return self;
}

// 共享内存cblk: detach
void audio_track_cblk_t_detach(audio_track_cblk_t* self);


static inline
bool audio_track_cblk_t_stepServer(audio_track_cblk_t* self, uint32_t frameCount)
{
    (void) self;
    (void) frameCount;
    // assert(frameCount == 定长帧);

    return true;
}



// uint32_t    stepUser(uint32_t frameCount);
// bool        stepServer(uint32_t frameCount);
// void*       buffer(uint32_t offset) const;
// uint32_t    framesAvailable();
// uint32_t    framesAvailable_l();
// uint32_t    framesReady();


// ----------------------------------------------------------------------------

#endif // DROID_AUDIO_TRACK_SHARED_H

/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Core entry points the minimal HD2 bring-up cannot yet provide and for which
 * OpenRTX has no generic driver stub.  The driver-layer gaps (codeplug NVM,
 * settings NVM) are filled by platform/drivers/stubs/{cps_io,nvmem}_stub.c;
 * only these two groups have no shared stub and so live here:
 *
 *   - vp_*  : voice prompts (real impl = core/voicePrompts.c) are left out
 *             until the audio path is brought up.
 *   - rtx_* : the radio task (real impl = rtx/rtx.cpp) is left out until the
 *             baseband/RF stack is brought up.
 *
 * Each group is replaced by its real implementation as that subsystem lands.
 */

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "core/voicePrompts.h"
#include "rtx/rtx.h"

/* ---- Voice prompts (no audio path in the minimal bring-up) ---------------- */
void vp_init()
{
}
void vp_stop()
{
}
void vp_tick()
{
}
void vp_play()
{
}
void vp_flush()
{
}
bool vp_isPlaying()
{
    return false;
}
void vp_beep(uint16_t freq, uint16_t duration)
{
    (void)freq;
    (void)duration;
}
void vp_beepSeries(const uint16_t *beepSeries)
{
    (void)beepSeries;
}
void vp_queueInteger(const int value)
{
    (void)value;
}
void vp_queuePrompt(const uint16_t prompt)
{
    (void)prompt;
}
void vp_queueString(const char *string, enum vpFlags flags)
{
    (void)string;
    (void)flags;
}
void vp_queueStringTableEntry(const char *const *e)
{
    (void)e;
}

/* ---- rtx (no radio task in the minimal bring-up) -------------------------- */
extern void sleepFor(unsigned int seconds, unsigned int mseconds);

void rtx_init(pthread_mutex_t *m)
{
    (void)m;
}
void rtx_terminate()
{
}
/* rtx_threadFunc spins `while(RUNNING) rtx_task();` -- sleep so the empty
 * task yields the CPU to the UI/main threads instead of busy-looping. */
void rtx_task()
{
    sleepFor(0, 100);
}
void rtx_configure(const rtxStatus_t *cfg)
{
    (void)cfg;
}
bool rtx_rxSquelchOpen()
{
    return false;
}
rssi_t rtx_getRssi()
{
    return (rssi_t)-127;
}

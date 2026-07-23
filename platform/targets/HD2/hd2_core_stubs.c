/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Core entry points the minimal HD2 bring-up cannot yet provide and for which
 * OpenRTX has no generic driver stub.  The driver-layer gaps (codeplug NVM,
 * settings NVM) are filled by platform/drivers/stubs/{cps_io,nvmem}_stub.c;
 * only this group has no shared stub and so lives here:
 *
 *   - vp_*  : voice prompts (real impl = core/voicePrompts.c) are left out
 *             until the audio path is brought up.
 *
 * The rtx_* radio-task stubs that used to live here are gone: the real radio
 * task (rtx/rtx.cpp) and FM operating mode now build into this target -- see
 * the ORTX_RADIO group in CMakeLists.txt.
 */

#include <stdint.h>
#include <stdbool.h>
#include "core/voicePrompts.h"

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

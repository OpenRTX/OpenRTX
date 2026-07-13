/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * UI-scope stubs for the OpenRTX-on-Miosix HD2 bring-up. These let the UI +
 * keyboard milestone link without the radio / voice-prompt-player / codeplug-
 * NVM / heap-profiling subsystems. Each is replaced by a real driver in a
 * later phase (FM-RX, DMR, NVM #73, audio/voice prompts).
 */

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "interfaces/radio.h"
#include "interfaces/delays.h"
#include "core/cps.h"
#include "core/state.h"
#include "core/voicePrompts.h"

// ---- bare-metal timer: miosix owns the timebase, so this is a no-op --------
void timer_init(void) { }

// ---- rtx (radio) thread + primitives ---------------------------------------
// The rtx_* functions used to be stubbed here (rtx_getRssi() returned a fixed
// -127 -> a static S-meter bar).  They now have a real RX-only implementation
// in hd2_rtx.c (tunes the AT1846S, listens on the VFO frequency while idle,
// surfaces a live filtered RSSI).  Kept out of this file to avoid duplicate
// symbols.

// ---- settings/VFO NVM: REAL implementation now in OpenRTX's ----------------
//      platform/drivers/NVM/nvmem_settings_HD2.c (W25Q sector 0xFFF000,
//      wired in via CMake ORTX_NVM).  The old -1/-0 stubs lived here.

// ---- heap profiling (memory_profiling.cpp not built) -----------------------
unsigned int getHeapSize(void)        { return 0; }
unsigned int getCurrentFreeHeap(void) { return 0; }

// ---- voice prompts: now the REAL software path -----------------------------
//      voicePrompts.c + audio_codec.c (codec2 3200 decode) + voicePromptData.S
//      (embedded voiceprompts.vpc) are built in (CMake ORTX_VP + hd2_codec2),
//      feeding outputStream_HD2.  The vp_* stubs that used to live here are
//      gone; vp_announce* queue helpers come from voicePromptUtils.c.

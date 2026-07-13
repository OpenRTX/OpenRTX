/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HD2 FM broadcast receive -- UI bridge globals.
 *
 * The broadcast tuner (RDA5802E) is now driven by OpMode_FMBroadcast
 * (OPMODE_FM_BCAST), selected by the UI FM screen via rtx_configure (see
 * ui.c::_ui_fmBcastConfigure).  The old worker thread that drove the chip
 * directly is gone -- it fought rtx.cpp for the shared audio path.  These
 * globals are the thin UI<->driver bridge that remains:
 *   - g_fm_active     : UI is in the FM screen (render gate)
 *   - g_fm_test_freq  : tune target, Hz (UI UP/DN -> OpMode tunes rxFrequency)
 *   - g_fm_rssi       : RDA5802E status, low byte = RSSI[6:0], bit8 = lock;
 *                       updated by the tuner HAL (RDA5802E_HD2.c tuner_*)
 *   - g_fm_mode       : tuner read-back channel (diagnostic)
 */

#include <cstdint>

/* g_fm_test_freq (tune target) is defined in radio_HD2.cpp (host-pokeable). */
extern "C" {
volatile uint32_t g_fm_active = 0;             /* set by the UI FM screen (SK2) */
volatile uint32_t g_fm_rssi   = 0;             /* tuner status (HAL-updated)     */
volatile uint32_t g_fm_mode   = 0;             /* tuner read-back channel        */
}

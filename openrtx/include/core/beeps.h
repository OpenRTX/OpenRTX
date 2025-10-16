/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2020-2025 OpenRTX Contributors
 *
 * This file is part of OpenRTX.
 */

#ifndef BEEPS_H_INCLUDED
#define BEEPS_H_INCLUDED
// Duration in tenths of a second.
#define SHORT_BEEP 3
#define LONG_BEEP 7

// Beep frequencies
#define BEEP_MENU_FIRST_ITEM 500
#define BEEP_MENU_ITEM 1000
#define BEEP_FUNCTION_LATCH_ON 800
#define BEEP_FUNCTION_LATCH_OFF 400
#define BEEP_KEY_GENERIC 750
extern const uint16_t BOOT_MELODY[];

#endif // BEEPS_H_INCLUDED

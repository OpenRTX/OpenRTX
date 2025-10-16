/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef EMULATOR_H
#define EMULATOR_H

#include "interfaces/keyboard.h"
#include <stdbool.h>
#include <stdint.h>
#include "SDL2/SDL.h"

#ifndef CONFIG_SCREEN_WIDTH
#define CONFIG_SCREEN_WIDTH 160
#endif

#ifndef CONFIG_SCREEN_HEIGHT
#define CONFIG_SCREEN_HEIGHT 128
#endif

enum choices
{
    VAL_RSSI=1,
    VAL_BAT,
    VAL_MIC,
    VAL_VOL,
    VAL_CH,
    VAL_PTT,
    PRINT_STATE,
    EXIT
};

typedef struct
{
    float RSSI;
    float vbat;
    float micLevel;
    float volumeLevel;
    float chSelector;
    bool  PTTstatus;
    bool  powerOff;
}
emulator_state_t;

extern emulator_state_t emulator_state;

void emulator_start();

keyboard_t emulator_getKeys();

#endif /* EMULATOR_H */

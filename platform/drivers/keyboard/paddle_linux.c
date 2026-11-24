/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <SDL2/SDL.h>
#include "core/cw_keyer.h"
#include "emulator/emulator.h"
#include "interfaces/paddle.h"

/*
 * Emulated paddle: the comma and period keys, or the "dit" and "dah"
 * commands of the emulator shell.
 */

void paddle_init(void)
{
}

void paddle_terminate(void)
{
}

uint8_t paddle_read(void)
{
    const uint8_t *keys = SDL_GetKeyboardState(NULL);
    uint8_t paddles = 0;

    if ((keys[SDL_SCANCODE_COMMA] != 0) || emulator_state.dit)
        paddles |= CW_PADDLE_DIT;
    if ((keys[SDL_SCANCODE_PERIOD] != 0) || emulator_state.dah)
        paddles |= CW_PADDLE_DAH;

    return paddles;
}

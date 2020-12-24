/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <interfaces/keyboard.h>
#include <SDL2/SDL.h>

void kbd_init()
{
}

keyboard_t kbd_getKeys() {
    SDL_Event event;
    keyboard_t keys = 0;
    while ((SDL_PollEvent(&event)) != 0) {
        if (event.type == SDL_QUIT)
            exit(0);
        //Ignore all non-keyboard events
        if (event.type != SDL_KEYDOWN) continue;
        const uint8_t *state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_0]) keys |= KEY_0;
        if (state[SDL_SCANCODE_1]) keys |= KEY_1;
        if (state[SDL_SCANCODE_2]) keys |= KEY_2;
        if (state[SDL_SCANCODE_3]) keys |= KEY_3;
        if (state[SDL_SCANCODE_4]) keys |= KEY_4;
        if (state[SDL_SCANCODE_5]) keys |= KEY_5;
        if (state[SDL_SCANCODE_6]) keys |= KEY_6;
        if (state[SDL_SCANCODE_7]) keys |= KEY_7;
        if (state[SDL_SCANCODE_8]) keys |= KEY_8;
        if (state[SDL_SCANCODE_9]) keys |= KEY_9;
        if (state[SDLK_ASTERISK]) keys |= KEY_STAR;
        if (state[SDL_SCANCODE_ESCAPE]) keys |= KEY_ESC;
        if (state[SDL_SCANCODE_DOWN]) keys |= KEY_DOWN;
        if (state[SDL_SCANCODE_UP]) keys |= KEY_UP;
        if (state[SDL_SCANCODE_RETURN]) keys |= KEY_ENTER;
        if (state[SDL_SCANCODE_NONUSHASH]) keys |= KEY_HASH;
        if (state[SDL_SCANCODE_MINUS]) keys |= KEY_F1;
        if (state[SDLK_PLUS]) keys |= KEY_MONI;
        return keys;
    }
}


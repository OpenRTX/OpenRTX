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

#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 160
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 128
#endif


SDL_Renderer *renderer;      /* SDL renderer           */
SDL_Window *window;          /* SDL window             */
SDL_Texture *displayTexture; /* SDL rendering surface  */

enum choices {
    VAL_RSSI=1,
    VAL_BAT,
    VAL_MIC,
    VAL_VOL,
    VAL_CH,
    VAL_PTT,
    PRINT_STATE,
    EXIT
};

typedef struct {
    float RSSI;
    float Vbat;
    float micLevel;
    float volumeLevel;
    float chSelector;
    bool PttStatus;
} radio_state;

radio_state Radio_State;

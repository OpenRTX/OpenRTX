/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
 *                                Silvano Seva IU2KWO                      *
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

#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include <stdint.h>

/**
 * This function initialises the User Interface, starting the
 * Finite State Machine describing the user interaction.
 */
void ui_init();

/**
 * This function writes the OpenRTX splash screen image into the framebuffer
 * centered in the screen space.
 */
void ui_drawSplashScreen();

/**
 * This function updates the local copy of the radio state
 * the local copy is called last_state
 * and is accessible from all the UI code as extern variable.
 */
void ui_saveState();

/**
 * This function advances the User Interface FSM, basing on the
 * current radio state and the keys pressed.
 *
 * @param sync_rtx: If true RTX needs to be synchronized
 */
void ui_updateFSM(bool *sync_rtx);

/**
 * This function redraws the GUI based on the last radio state.
 *
 * @return true if GUI has been updated and a screen render is necessary.
 */
bool ui_updateGUI();

/**
 * Push an event to the UI event queue.
 *
 * @param type: event type.
 * @param data: event data.
 * @return true on success false on failure.
 */
bool ui_pushEvent(const uint8_t type, const uint32_t data);

/**
 * This function terminates the User Interface.
 */
void ui_terminate();

#endif /* UI_H */

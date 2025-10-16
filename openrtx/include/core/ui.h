/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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

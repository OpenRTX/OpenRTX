/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * \file ui_fsm.h
 * \brief Default UI finite-state machine (event queue, input handling).
 */

#ifndef UI_FSM_H
#define UI_FSM_H

#include <stdbool.h>
#include <stdint.h>

void ui_fsm_init(void);
bool ui_fsm_needRedraw(void);
void ui_fsm_clearRedraw(void);
bool ui_fsm_is_macro_menu_visible(void);

#endif /* UI_FSM_H */

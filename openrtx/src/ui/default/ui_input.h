/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * \file ui_input.h
 * \brief Text and numeric keypad input helpers for the default UI.
 */

#ifndef UI_INPUT_H
#define UI_INPUT_H

#include <stdbool.h>
#include <stdint.h>
#include "core/input.h"
#include "ui/ui_default.h"

void _ui_textInputReset(ui_state_t *s, char *buf);
void _ui_textInputKeypad(ui_state_t *s, char *buf, uint8_t max_len,
                         kbd_msg_t msg, bool callsign);
void _ui_textInputConfirm(ui_state_t *s, char *buf);
void _ui_textInputDel(ui_state_t *s, char *buf);
void _ui_numberInputKeypad(ui_state_t *s, uint32_t *num, kbd_msg_t msg);
void _ui_numberInputDel(ui_state_t *s, uint32_t *num);

#endif /* UI_INPUT_H */

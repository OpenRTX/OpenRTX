/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdint.h>
#include <string.h>
#include "ui_input.h"
#include "core/voicePromptUtils.h"
#include "interfaces/delays.h"
#include "hwconfig.h"

static const char *symbols_ITU_T_E161[] = { " 0",        ",.?1",      "abc2ABC",
                                            "def3DEF",   "ghi4GHI",   "jkl5JKL",
                                            "mno6MNO",   "pqrs7PQRS", "tuv8TUV",
                                            "wxyz9WXYZ", "-/*",       "#" };

static const char *symbols_ITU_T_E161_callsign[] = { "0 ",    "1",     "ABC2",
                                                     "DEF3",  "GHI4",  "JKL5",
                                                     "MNO6",  "PQRS7", "TUV8",
                                                     "WXYZ9", "-/",    "" };

void _ui_textInputReset(ui_state_t *s, char *buf)
{
    s->input_number = 0;
    s->input_position = 0;
    s->input_set = 0;
    s->last_keypress = 0;
    memset(buf, 0, 9);
    buf[0] = '_';
}

void _ui_textInputKeypad(ui_state_t *s, char *buf, uint8_t max_len,
                         kbd_msg_t msg, bool callsign)
{
    long long now = getTick();
    uint8_t num_key = input_getPressedChar(msg);

    bool key_timeout = ((now - s->last_keypress) >= input_longPressTimeout);
    bool same_key = s->input_number == num_key;
    uint8_t num_symbols = 0;
    if (callsign) {
        num_symbols = strlen(symbols_ITU_T_E161_callsign[num_key]);
        if (num_symbols == 0)
            return;
    } else
        num_symbols = strlen(symbols_ITU_T_E161[num_key]);

    if ((s->input_position >= max_len)
        || ((s->input_position == (max_len - 1)) && (key_timeout || !same_key)))
        return;

    if (s->last_keypress != 0) {
        if (same_key && !key_timeout) {
            s->input_set = (s->input_set + 1) % num_symbols;
        } else {
            s->input_position += 1;
            s->input_set = 0;
        }
    }
    if (callsign)
        buf[s->input_position] =
            symbols_ITU_T_E161_callsign[num_key][s->input_set];
    else {
        buf[s->input_position] = symbols_ITU_T_E161[num_key][s->input_set];
    }
    vp_announceInputChar(buf[s->input_position]);
    s->input_number = num_key;
    s->last_keypress = now;
}

void _ui_textInputConfirm(ui_state_t *s, char *buf)
{
    buf[s->input_position + 1] = '\0';
}

void _ui_textInputDel(ui_state_t *s, char *buf)
{
    if (buf[s->input_position] && buf[s->input_position] != '_')
        vp_announceInputChar(buf[s->input_position]);

    buf[s->input_position] = '\0';
    if (s->input_position > 0) {
        s->input_position--;
    } else
        s->last_keypress = 0;
    s->input_set = 0;
}

void _ui_numberInputKeypad(ui_state_t *s, uint32_t *num, kbd_msg_t msg)
{
    long long now = getTick();

#ifdef CONFIG_UI_NO_KEYBOARD
    if (msg.keys & KNOB_LEFT) {
        *num = *num + 1;
        if (*num % 10 == 0)
            *num = *num - 10;
    }

    if (msg.keys & KNOB_RIGHT) {
        if (*num == 0)
            *num = 9;
        else {
            *num = *num - 1;
            if (*num % 10 == 9)
                *num = *num + 10;
        }
    }

    if (msg.keys & KEY_ENTER)
        *num *= 10;

    vp_announceInputChar('0' + *num % 10);

    s->input_number = *num % 10;
#else
    if (s->input_position >= 10)
        return;

    uint8_t num_key = input_getPressedNumber(msg);
    *num *= 10;
    *num += num_key;

    vp_announceInputChar('0' + num_key);

    s->input_number = num_key;
#endif

    s->last_keypress = now;
}

void _ui_numberInputDel(ui_state_t *s, uint32_t *num)
{
    vp_announceInputChar('0' + *num % 10);

    if (s->input_position > 0)
        s->input_position--;
    else
        s->last_keypress = 0;

    s->input_set = 0;
}

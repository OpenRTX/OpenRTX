/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <zephyr/input/input.h>
#include "interfaces/keyboard.h"
#include "interfaces/platform.h"
#include "hwconfig.h"
#include "pmu.h"

static const struct device *const buttons_dev = DEVICE_DT_GET(DT_NODELABEL(buttons));

static int8_t  old_pos = 0;
static keyboard_t keys = 0;

// Defined in ttwrplus/platform.c to implement volume control
extern uint8_t volume_level;

static void gpio_keys_cb_handler(struct input_event *evt)
{
    uint32_t keyCode = 0;

    // Map betweek Zephyr keys and OpenRTX keys
    switch(evt->code)
    {
        case INPUT_KEY_VOLUMEDOWN:
            keyCode = KEY_VOLDOWN;
            if(evt->value != 0 && volume_level > 9)
                volume_level -= 10;
            break;

        case INPUT_BTN_START:
            keyCode = KEY_ENTER;
            break;

        case INPUT_BTN_SELECT:
            keyCode = KEY_ESC;
            break;
    }

    if(evt->value != 0)
        keys |= keyCode;
    else
        keys &= ~keyCode;
}

INPUT_CALLBACK_DEFINE(buttons_dev, gpio_keys_cb_handler);


void kbd_init()
{
    // Initialise old position
    old_pos = platform_getChSelector();
}

void kbd_terminate()
{
}

keyboard_t kbd_getKeys()
{
    keys &= ~KNOB_LEFT;
    keys &= ~KNOB_RIGHT;

    // Read rotary encoder to send KNOB_LEFT and KNOB_RIGHT events
    int8_t new_pos = platform_getChSelector() / 2;
    if (old_pos != new_pos)
    {
        int8_t delta = new_pos - old_pos;

        // Normal case: handle up/down by looking at the pulse difference
        if(delta > 0 && new_pos % 2 == 0)
            keys |= KNOB_LEFT;
        else if (delta < 0 && new_pos % 2 != 0)
            keys |= KNOB_RIGHT;

        // Corner case 1: rollover from negative (old) to positive (new) value.
        // Delta is positive but it counts as a down key.
        if((old_pos < 0) && (new_pos > 0))
        {
            keys &= ~KNOB_LEFT;
            keys |= KNOB_RIGHT;
        }

        // Corner case 2: rollover from positive (old) to negative (new) value.
        // Delta is negative but it counts as an up key.
        if((old_pos > 0) && (new_pos < 0))
        {
            keys &= ~KNOB_RIGHT;
            keys |= KNOB_LEFT;
        }

        old_pos = new_pos;
    }

    /*
     * The power key is only connected to the PMU and its state is detected through
     * the PMU interrupts.
     */
    if (pmu_pwrBtnStatus() == 1)
    {
        // Update the volume only once when key is pressed
        if (!(keys & KEY_VOLUP) && volume_level < 246)
            volume_level += 10;
        keys |= KEY_VOLUP;
    }
    else
        keys &= ~KEY_VOLUP;

    return keys;
}

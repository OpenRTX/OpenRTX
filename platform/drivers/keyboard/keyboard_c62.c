/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <interfaces/keyboard.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#define ROW1_NODE DT_NODELABEL(row1)
#define ROW2_NODE DT_NODELABEL(row2)
#define ROW3_NODE DT_NODELABEL(row3)
#define ROW4_NODE DT_NODELABEL(row4)
#define COL1_NODE DT_NODELABEL(col1)
#define COL2_NODE DT_NODELABEL(col2)
#define COL3_NODE DT_NODELABEL(col3)
#define COL4_NODE DT_NODELABEL(col4)
#define COL5_NODE DT_NODELABEL(col5)

static const struct gpio_dt_spec row1 = GPIO_DT_SPEC_GET(ROW1_NODE, gpios);
static const struct gpio_dt_spec row2 = GPIO_DT_SPEC_GET(ROW2_NODE, gpios);
static const struct gpio_dt_spec row3 = GPIO_DT_SPEC_GET(ROW3_NODE, gpios);
static const struct gpio_dt_spec row4 = GPIO_DT_SPEC_GET(ROW4_NODE, gpios);
static const struct gpio_dt_spec col1 = GPIO_DT_SPEC_GET(COL1_NODE, gpios);
static const struct gpio_dt_spec col2 = GPIO_DT_SPEC_GET(COL2_NODE, gpios);
static const struct gpio_dt_spec col3 = GPIO_DT_SPEC_GET(COL3_NODE, gpios);
static const struct gpio_dt_spec col4 = GPIO_DT_SPEC_GET(COL4_NODE, gpios);
static const struct gpio_dt_spec col5 = GPIO_DT_SPEC_GET(COL5_NODE, gpios);
/*
 * The scan drives one of the five column lines at a time and reads the four
 * row lines.
 */
static const struct gpio_dt_spec *driveLines[] = { &col1, &col2, &col3, &col4,
                                                   &col5 };
static const struct gpio_dt_spec *senseLines[] = { &row1, &row2, &row3, &row4 };

#define SIDEKEY_NODE DT_NODELABEL(sidekey)

static const struct gpio_dt_spec sidekey = GPIO_DT_SPEC_GET_OR(SIDEKEY_NODE,
                                                               gpios, { 0 });

static const uint32_t keyMap[] = { KEY_ENTER, KEY_UP,   KEY_DOWN, KEY_ESC,
                                   KEY_3,     KEY_4,    KEY_5,    KEY_6,
                                   KEY_7,     KEY_8,    KEY_9,    KEY_STAR,
                                   KEY_0,     KEY_HASH, 0,
                                   0, // 14 and 15 are unused
                                   KEY_F1,    KEY_F2,   KEY_1,    KEY_2 };

void kbd_init()
{
    for (size_t i = 0; i < ARRAY_SIZE(driveLines); i++)
        gpio_pin_configure_dt(driveLines[i], GPIO_OUTPUT_INACTIVE);

    for (size_t i = 0; i < ARRAY_SIZE(senseLines); i++)
        gpio_pin_configure_dt(senseLines[i], GPIO_INPUT | GPIO_PULL_UP);

    gpio_pin_configure_dt(&sidekey, GPIO_INPUT);
}

void kbd_terminate()
{
}

keyboard_t kbd_getKeys()
{
    uint32_t keys = 0;

    for (size_t d = 0; d < ARRAY_SIZE(driveLines); d++) {
        gpio_pin_set_dt(driveLines[d], 1);

        // Small delay to stabilize
        k_busy_wait(1);

        for (size_t s = 0; s < ARRAY_SIZE(senseLines); s++) {
            if (gpio_pin_get_dt(senseLines[s]) == 1)
                keys |= 1U << ((d * ARRAY_SIZE(senseLines)) + s);
        }

        gpio_pin_set_dt(driveLines[d], 0);
    }

    // Map raw keys to KEY_ enum values
    keyboard_t result = 0;

    for (size_t i = 0; i < ARRAY_SIZE(keyMap); i++) {
        if ((keys & (1U << i)) != 0)
            result |= keyMap[i];
    }

    if (gpio_pin_get_dt(&sidekey) == 1)
        result |= KEY_MONI;

    return result;
}

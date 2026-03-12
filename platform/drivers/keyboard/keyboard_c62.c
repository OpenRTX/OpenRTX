/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <interfaces/keyboard.h>
#include <zephyr/drivers/gpio.h>
#include <interfaces/delays.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(keyboard, LOG_LEVEL_DBG);
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

#define SIDEKEY_NODE DT_NODELABEL(sidekey)

static const struct gpio_dt_spec sidekey = GPIO_DT_SPEC_GET_OR(SIDEKEY_NODE, gpios, {0});

void kbd_init()
{

}

void kbd_terminate()
{

}

keyboard_t kbd_getKeys()
{
    uint32_t keys = 0;
    const struct gpio_dt_spec *cols[] = {&row1, &row2, &row3, &row4};
    const struct gpio_dt_spec *rows[] = {&col1, &col2, &col3, &col4, &col5};
    
    // Configure rows as outputs (initially low)
    for (int i = 0; i < 5; i++)
    {
        gpio_pin_configure_dt(rows[i], GPIO_OUTPUT);
        gpio_pin_set_dt(rows[i], 0); // Set to low
    }
    
    // Configure columns as inputs with pull-ups
    for (int i = 0; i < 4; i++)
    {
        gpio_pin_configure_dt(cols[i], GPIO_INPUT | GPIO_PULL_UP);
    }
    
    // Scan the matrix
    for (int r = 0; r < 5; r++)
    {
        // Set the current row high
        gpio_pin_set_dt(rows[r], 1);
        
        // Small delay to stabilize
        k_busy_wait(1);
        
        // Read columns
        for (int c = 0; c < 4; c++)
        {
            // If column reads low, key is pressed
            if (gpio_pin_get_dt(cols[c]) == 1)
            {
                // Set corresponding bit in result
                keys |= (1 << (r * 4 + c));
            }
        }
        
        // Set the current row low again
        gpio_pin_set_dt(rows[r], 0);
    }

    // Map raw keys to KEY_ enum values
    keyboard_t result = 0;

    static const uint32_t keyMap[] = {
        KEY_ENTER, KEY_UP, KEY_DOWN, KEY_ESC,
        KEY_3, KEY_4, KEY_5, KEY_6,
        KEY_7, KEY_8, KEY_9, KEY_STAR,
        KEY_0, KEY_HASH, 0, 0,  // 14 and 15 are unused
        KEY_MONI, KEY_F2, KEY_1, KEY_2
    };
    
    for (uint8_t i = 0; i < 20; i++) {
        if (keys & (1 << i) && i < sizeof(keyMap)/sizeof(keyMap[0])) {
            result |= keyMap[i];
        }
    }

    // Configure the side key as input with pull-up
    gpio_pin_configure_dt(&sidekey, GPIO_INPUT);
    // Read the pin state
    int pin_val = gpio_pin_get_dt(&sidekey);

    // Log the pin state
    //LOG_INF("GPIO B1 state: %d", pin_val);

    // Check if the side key is pressed
    if (pin_val == 1) {
        result |= KEY_MONI;
    }
    
    return result;
}

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "peripherals/gpio.h"
#include "interfaces/delays.h"
#include "interfaces/keyboard.h"
#include "interfaces/platform.h"
#include "drivers/ADC/adc_stm32.h"
#include "hwconfig.h"

/**
 * \internal
 * Helper function which compares two voltages and tells if they're equal within
 * a 200mV range.
 *
 * @param voltage: voltage to be compared against reference, in mV.
 * @param reference: reference voltage for comparison, in mV.
 * @return true if voltage is equal to reference ± 200mV, false otherwise.
 */
bool _compareVoltages(const int voltage, const int reference)
{
    int difference = voltage - reference;
    if(abs(difference) <= 200) return true;
    return false;
}

/*
 * Mapping of palmtop mic buttons:
 *
 *         +-------+-------+-------+-------+
 *         | Col 0 | Col 1 | Col 2 | Col 3 |
 * +-------+-------+-------+-------+-------+
 * | Row 0 |   1   |   2   |   3   |   A   |
 * +-------+-------+-------+-------+-------+
 * | Row 1 |   4   |   5   |   6   |   B   |
 * +-------+-------+-------+-------+-------+
 * | Row 2 |   7   |   8   |   9   |   C   |
 * +-------+-------+-------+-------+-------+
 * | Row 3 |   *   |   0   |   #   |   D   |
 * +-------+-------+-------+-------+-------+
 * | Row 4 |       |  A/B  |   UP  | DOWN  |
 * +-------+-------+-------+-------+-------+
 *
 */

static const uint32_t micKeymap[5][4] =
{
    {  KEY_1,  KEY_2,   KEY_3,    KEY_F1   },
    {  KEY_4,  KEY_5,   KEY_6,    KEY_F2   },
    {  KEY_7,  KEY_8,   KEY_9,   KEY_ENTER },
    {KEY_STAR, KEY_0,  KEY_HASH,  KEY_ESC  },
    {   0    , KEY_F5,  KEY_UP,  KEY_DOWN  }
};


void kbd_init()
{
    gpio_setMode(KB_COL1, INPUT_PULL_UP);
    gpio_setMode(KB_COL2, INPUT_PULL_UP);
    gpio_setMode(KB_COL3, INPUT_PULL_UP);
    gpio_setMode(KB_COL4, INPUT_PULL_UP);

    gpio_setMode(KB_ROW1, OUTPUT);
    gpio_setMode(KB_ROW2, OUTPUT);
    gpio_setMode(KB_ROW3, OUTPUT);

    gpio_setPin(KB_ROW1);
    gpio_setPin(KB_ROW2);
    gpio_setPin(KB_ROW3);
}

void kbd_terminate()
{
    /* Back to default state */
    gpio_setMode(KB_ROW1, INPUT);
    gpio_setMode(KB_ROW2, INPUT);
    gpio_setMode(KB_ROW3, INPUT);
}

keyboard_t kbd_getKeys()
{
    keyboard_t keys = 0;

    /* Use absolute position knob to emulate left and right buttons */
    static uint8_t old_pos = 0;
    uint8_t new_pos = platform_getChSelector();
    if (old_pos != new_pos)
    {
        if (new_pos < old_pos)
            keys |= KEY_LEFT;
        else
            keys |= KEY_RIGHT;
        old_pos = new_pos;
    }

   /*
    * Mapping of front buttons:
    *
    *       +------+-----+-------+-----+
    *       | PD0  | PD1 |  PE0  | PE1 |
    * +-----+------+-----+-------+-----+
    * | PD2 | ENT  |     |   P1  | P4  |
    * +-----+------+-----+-------+-----+
    * | PD3 | down |     |  red  | P3  |
    * +-----+------+-----+-------+-----+
    * | PD4 | ESC  | up  | green | P2  |
    * +-----+------+-----+-------+-----+
    *
    * The coloumn lines have pull-up resistors, thus the detection of a button
    * press follows an active-low logic.
    *
    */

    gpio_clearPin(KB_ROW1);

    delayUs(10);
    if(gpio_readPin(KB_COL1) == 0) keys |= KEY_ENTER;
    if(gpio_readPin(KB_COL3) == 0) keys |= KEY_F1;
    if(gpio_readPin(KB_COL4) == 0) keys |= KEY_F4;
    gpio_setPin(KB_ROW1);

    /* Row 2: button on col. 3 ("red") is not mapped */
    gpio_clearPin(KB_ROW2);
    delayUs(10);

    if(gpio_readPin(KB_COL1) == 0) keys |= KEY_DOWN;
    if(gpio_readPin(KB_COL4) == 0) keys |= KEY_F3;
    gpio_setPin(KB_ROW2);

    /* Row 3: button on col. 3 ("green") is not mapped */
    gpio_clearPin(KB_ROW3);
    delayUs(10);

    if(gpio_readPin(KB_COL1) == 0) keys |= KEY_ESC;
    if(gpio_readPin(KB_COL2) == 0) keys |= KEY_UP;
    if(gpio_readPin(KB_COL4) == 0) keys |= KEY_F2;
    gpio_setPin(KB_ROW3);

   /*
    * Mapping of palmtop mic row/coloumn voltages:
    *                  +-------+-------+-------+-------+
    *                  | Col 0 | Col 1 | Col 2 | Col 3 |
    *                  +-------+-------+-------+-------+
    *                  | 700mV | 1300mV| 1900mV| 2600mV|
    * +-------+--------+-------+-------+-------+-------+
    * | Row 0 | 2600mV |
    * +-------+--------+
    * | Row 1 | 2100mV |
    * +-------+--------+
    * | Row 2 | 1500mV |
    * +-------+--------+
    * | Row 3 | 1000mV |
    * +-------+--------+
    * | Row 4 | 500mV  |
    * +-------+--------+
    *
    */

    /* Retrieve row/coloumn voltage measurements. */
    uint16_t row = (uint16_t) (adc_getVoltage(&adc1, ADC_SW2_CH) / 1000);
    uint16_t col = (uint16_t) (adc_getVoltage(&adc1, ADC_SW1_CH) / 1000);

    /* Map row voltage to row index. */
    uint8_t rowIdx = 0xFF;
    if(_compareVoltages(row, 2600)) rowIdx = 0;
    if(_compareVoltages(row, 2100)) rowIdx = 1;
    if(_compareVoltages(row, 1500)) rowIdx = 2;
    if(_compareVoltages(row, 1000)) rowIdx = 3;
    if(_compareVoltages(row,  500)) rowIdx = 4;

    /* Map col voltage to col index. */
    uint8_t colIdx = 0xFF;
    if(_compareVoltages(col,  700)) colIdx = 0;
    if(_compareVoltages(col, 1300)) colIdx = 1;
    if(_compareVoltages(col, 1900)) colIdx = 2;
    if(_compareVoltages(col, 2600)) colIdx = 3;

    /*
     * Get corresponding key.
     * Having rowIdx or colIdx equal to 0xFF means that no button on the palmtop
     * key is pressed.
     */
    if((rowIdx != 0xFF) && (colIdx != 0xFF))
    {
        keys |= micKeymap[rowIdx][colIdx];
    }

    return keys;
}

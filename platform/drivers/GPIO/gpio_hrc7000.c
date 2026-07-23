/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivers/GPIO/gpio_hrc7000.h"

void gpio_setMode(const void *port, const uint8_t pin, const uint16_t mode)
{
    GPIO_TypeDef *g = (GPIO_TypeDef *)port;

    switch (mode & 0xFF) {
        case OUTPUT:
        case OPEN_DRAIN:
            g->DDR |= (1u << pin); /* output direction */
            break;

        /*
         * ALTERNATE*: the HR_C7000 selects pin functions through the SOCSYS
         * pad-mux, not a per-GPIO alternate register, so there is no per-pin
         * alternate direction here.  Leave the pad configured as an input and
         * let the pad-mux own the function.
         */
        case INPUT:
        case INPUT_PULL_UP:
        case INPUT_PULL_DOWN:
        case ANALOG:
        default:
            g->DDR &= ~(1u << pin); /* input direction */
            break;
    }
}

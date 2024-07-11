/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
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

#include <interfaces/platform.h>
#include <peripherals/gpio.h>
#include <hwconfig.h>
#include "backlight.h"

void backlight_init()
{
    /*
     * Configure backlight PWM: 58.5kHz, 8 bit resolution
     */
    SIM->SCGC6 |= SIM_SCGC6_FTM0(1);           /* Enable clock                     */

    FTM0->CONTROLS[3].CnSC = FTM_CnSC_MSB(1)
                           | FTM_CnSC_ELSB(1); /* Edge-aligned PWM, clear on match */
    FTM0->CONTROLS[3].CnV  = 0;

    FTM0->MOD  = 0xFF;                         /* Reload value                     */
    FTM0->SC   = FTM_SC_PS(3)                  /* Prescaler divide by 8            */
               | FTM_SC_CLKS(1);               /* Enable timer                     */

    gpio_setMode(LCD_BKLIGHT, OUTPUT | ALTERNATE_FUNC(4));
}

void backlight_terminate()
{
    gpio_clearPin(LCD_BKLIGHT);
    SIM->SCGC6 &= ~SIM_SCGC6_FTM0(1);
}

/*
 * This function is defined in display.h
 */
void display_setBacklightLevel(uint8_t level)
{
    if(level > 100) level = 100;

    // Convert value to 0 - 255
    FTM0->CONTROLS[3].CnV = (2 * level) + (level * 55)/100;
}

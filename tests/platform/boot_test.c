/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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
#include <interfaces/delays.h>
<<<<<<< Updated upstream
=======
#include <peripherals/gpio.h>
#include <platform/drivers/display/ST7735S.h>
#include <core/graphics.h>
#include <core/ui.h>
#include <ui/ui_default.h>
>>>>>>> Stashed changes

int main()
{
    platform_init();
<<<<<<< Updated upstream

    while(1)
    {
=======
    // gpio_bits_set(GPIOA, BOARD_GPIOA_LCD_RESX);
    gpio_setPin(LCD_RST);
    gfx_clearScreen();
    // delayMs(5000);
    // gfx_clearScreen();
    // Set state.settings.callsign to K8TUN
    state.settings.callsign[0] = 'K';
    state.settings.callsign[1] = '8';
    state.settings.callsign[2] = 'T';
    state.settings.callsign[3] = 'U';
    state.settings.callsign[4] = 'N';
    state.settings.callsign[5] = '\0';
    ui_drawSplashScreen();
    delayMs(1000);
    while(1)
    {
        GPIOB->cfgr_bit.iomc6 = 0x02; // PB6 Alternate function
        // gfx_clearScreen();
>>>>>>> Stashed changes
        platform_ledOn(GREEN);
        delayMs(1000);
        platform_ledOff(GREEN);
        // DISPLAY_FillColor(0x8888);
        delayMs(1000);
    }

    return 0;
}

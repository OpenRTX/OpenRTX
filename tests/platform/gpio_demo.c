/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Izzo IU2NUO,                    *
 *                                Niccolò Izzo IU2KIN                      *
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <os.h>
#include <interfaces/gpio.h>
#include <interfaces/graphics.h>
#include "hwconfig.h"
#include <interfaces/platform.h>

void printBits(uint16_t value, Pos_st pos)
{
    char buf[16] = {0};

    unsigned char i = 0;
    for(; i < 15; i++)
    {
        buf[i] = '0';
        if(value & (1 << i)) buf[i] += 1;
    }

    Color_st color_fg ;
    uiColorLoad( &color_fg , COLOR_FG );
    gfx_print(pos, FONT_SIZE_1, ALIGN_LEFT, color_fg, buf);
}

int main(void)
{
    GPIOA->MODER = 0;
    GPIOB->MODER = 0;
    GPIOC->MODER = 0;
    GPIOD->MODER = 0;
    GPIOE->MODER = 0;

    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setMode(LCD_BKLIGHT, OUTPUT);

    // Init the graphic stack
    gfx_init();
    platform_setBacklightLevel(255);

    OS_ERR os_err;

    // Task infinite loop
    while(1)
    {
        gpio_togglePin(GREEN_LED);
        gfx_clearScreen();

        Pos_st pos_line1 = {0, 0};
        Pos_st pos_line2 = {0, 17};
        Pos_st pos_line3 = {0, 35};
        Pos_st pos_line4 = {0, 53};
        Pos_st pos_line5 = {0, 71};

        printBits((GPIOA->IDR & 0x0000FFFF), pos_line1);
        printBits((GPIOB->IDR & 0x0000FFFF), pos_line2);
        printBits((GPIOC->IDR & 0x0000FFFF), pos_line3);
        printBits((GPIOD->IDR & 0x0000FFFF), pos_line4);
        printBits((GPIOE->IDR & 0x0000FFFF), pos_line5);

        gfx_render();
        while(gfx_renderingInProgress());
        OSTimeDlyHMSM(0u, 0u, 0u, 100u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }
}

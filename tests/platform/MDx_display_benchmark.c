/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <hwconfig.h>
#include <interfaces/graphics.h>
#include <interfaces/keyboard.h>
#include <interfaces/platform.h>
#include <os.h>
#include <stdint.h>
#include <stdio.h>

uint64_t benchmark(uint32_t n);

int main()
{
    platform_init();
    platform_setBacklightLevel(255);

    gfx_init();
    kbd_init();

    /*
     * Setup timer for time measurement: input clock is twice the APB2 clock, so
     * is 168MHz. Setting the prescaler to 33 we get a tick frequency of
     * 5,09090909091 MHz, that is a resolution of 0.196 us per tick.
     * Considering that the timer has a 16-bit counter, we have rollover in
     * 12.87 ms.
     */

    static const uint32_t clkDivider = 33;

    RCC->APB2ENR |= RCC_APB2ENR_TIM9EN;

    TIM9->PSC = clkDivider - 1;
    TIM9->ARR = 0xFFFF;
    TIM9->CR1 = TIM_CR1_CEN;

    uint32_t numIterations = 128;

    while (1)
    {
        getchar();

        uint64_t tot_ticks = benchmark(numIterations);

        float totalTime_s = ((float)(tot_ticks * clkDivider)) / 168000000.0f;
        printf("Average values over %ld iterations:\r\n", numIterations);
        printf("- %lld ticks\r\n- %f ms\r\n", tot_ticks, totalTime_s * 1000.0f);
    }
}

uint64_t benchmark(uint32_t n)
{
    uint64_t totalTime = 0;
    uint32_t dummy     = 0;

    for (uint32_t i = 0; i < n; i++)
    {
        gfx_clearScreen();
        point_t origin      = {0, i % 128};
        color_t color_red   = {255, 0, 0, 255};
        color_t color_white = {255, 255, 255, 255};
        gfx_drawRect(origin, 160, 20, color_red, 1);
        gfx_print(origin, buffer, FONT_SIZE_24PT, TEXT_ALIGN_LEFT, color_white,
                  "KEK");

        dummy += kbd_getKeys();

        /* Measure the time taken by gfx_render() */
        TIM9->CNT = 0;
        gfx_render();
        totalTime += TIM9->CNT;
    }

    return totalTime / n;
}

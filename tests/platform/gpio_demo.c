/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "os.h"
#include "interfaces/gpio.h"
#include "interfaces/graphics.h"
#include "hwconfig.h"
#include "interfaces/platform.h"

void printBits(uint16_t value, point_t pos)
{
    char buf[16] = {0};

    unsigned char i = 0;
    for(; i < 15; i++)
    {
        buf[i] = '0';
        if(value & (1 << i)) buf[i] += 1;
    }

    color_t color_white = {255, 255, 255};
    gfx_print(pos, FONT_SIZE_1, TEXT_ALIGN_LEFT, color_white, buf);
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

        point_t pos_line1 = {0, 0};
        point_t pos_line2 = {0, 17};
        point_t pos_line3 = {0, 35};
        point_t pos_line4 = {0, 53};
        point_t pos_line5 = {0, 71};

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

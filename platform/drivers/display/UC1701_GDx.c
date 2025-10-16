/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "drivers/backlight/backlight.h"
#include "peripherals/gpio.h"
#include "interfaces/display.h"
#include "interfaces/delays.h"
#include "hwconfig.h"

/**
 * \internal
 * Send one byte to display controller, via bit banging.
 *
 * @param value: byte to be sent.
 */
static void sendByteToController(uint8_t value)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        gpio_clearPin(LCD_CLK);

        if(value & 0x80)
        {
            gpio_setPin(LCD_DAT);
        }
        else
        {
            gpio_clearPin(LCD_DAT);
        }

        gpio_setPin(LCD_CLK);
        value <<= 1;
    }
}

/**
 * \internal
 * Send one row of pixels to the display.
 * Pixels in framebuffer are stored "by rows", while display needs data to be
 * sent "by columns": this function performs the needed conversion.
 *
 * @param row: pixel row to be be sent.
 */
static void display_renderRow(uint8_t row, uint8_t *frameBuffer)
{
    /* magic stuff */
    uint8_t *buf = (frameBuffer + 128 * row);
    for (uint8_t i = 0; i<16; i++)
    {
        uint8_t tmp[8] = {0};
        for (uint8_t j = 0; j < 8; j++)
        {
            uint8_t tmp_buf = buf[j*16 + i];
            int count = __builtin_popcount(tmp_buf);
            while (count > 0)
            {
                int pos = __builtin_ctz(tmp_buf);
                tmp[pos] |= 1UL << j;
                tmp_buf &= ~(1 << pos);
                count--;
            }
        }

        for (uint8_t s = 0; s < 8; s++)
        {
            sendByteToController(tmp[s]);
        }
    }
}


void display_init()
{
    backlight_init();           /* Initialise backlight driver */

    gpio_setMode(LCD_CS,  OUTPUT);
    gpio_setMode(LCD_RST, OUTPUT);
    gpio_setMode(LCD_RS,  OUTPUT);
    gpio_setMode(LCD_CLK, OUTPUT);
    gpio_setMode(LCD_DAT, OUTPUT);

    gpio_setPin(LCD_CS);
    gpio_clearPin(LCD_RS);
    gpio_clearPin(LCD_CLK);
    gpio_clearPin(LCD_DAT);

    gpio_clearPin(LCD_RST);     /* Reset controller                          */
    delayMs(1);
    gpio_setPin(LCD_RST);
    delayMs(5);

    gpio_clearPin(LCD_CS);      /* Bring down CS and keep it low from now on */

    gpio_clearPin(LCD_RS);      /* RS low -> command mode                    */
    sendByteToController(0x2F); /* Voltage Follower On                       */
    sendByteToController(0x81); /* Set Electronic Volume                     */
    sendByteToController(0x15); /* Contrast, initial setting                 */
    sendByteToController(0xA2); /* Set Bias = 1/9                            */
    sendByteToController(0xA1); /* A0 Set SEG Direction                      */
    sendByteToController(0xC0); /* Set COM Direction                         */
    sendByteToController(0xA4); /* White background, black pixels            */
    sendByteToController(0xAF); /* Set Display Enable                        */
}

void display_terminate()
{
    backlight_terminate();

    gpio_setMode(LCD_CS,  INPUT);
    gpio_setMode(LCD_RST, INPUT);
    gpio_setMode(LCD_RS,  INPUT);
    gpio_setMode(LCD_CLK, INPUT);
    gpio_setMode(LCD_DAT, INPUT);
}

void display_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    for(uint8_t row = startRow; row < endRow; row++)
    {
        gpio_clearPin(LCD_RS);            /* RS low -> command mode */
        sendByteToController(0xB0 | row); /* Set Y position         */
        sendByteToController(0x10);       /* Set X position         */
        sendByteToController(0x04);
        gpio_setPin(LCD_RS);              /* RS high -> data mode   */
        display_renderRow(row, (uint8_t *) fb);
    }

}

void display_render(void *fb)
{
    display_renderRows(0, CONFIG_SCREEN_HEIGHT / 8, fb);
}

void display_setContrast(uint8_t contrast)
{
    gpio_clearPin(LCD_RS);               /* RS low -> command mode              */
    sendByteToController(0x81);          /* Set Electronic Volume               */
    sendByteToController(contrast >> 2); /* Controller contrast range is 0 - 63 */
}

/*
 * Function implemented in backlight_GDx driver
 *
 * void display_setBacklightLevel(uint8_t level)
 */

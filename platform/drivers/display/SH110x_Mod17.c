/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "peripherals/gpio.h"
#include "interfaces/delays.h"
#include "hwconfig.h"
#include "drivers/SPI/spi_stm32.h"
#include "SH110x_Mod17.h"

extern const struct spiDevice spi2;

void SH110x_init()
{
    gpio_setPin(LCD_CS);
    gpio_clearPin(LCD_DC);

    gpio_clearPin(LCD_RST); /* Reset controller                */
    delayMs(50);
    gpio_setPin(LCD_RST);
    delayMs(50);

    static const uint8_t init[] =
    {
        0xAE,    /* SH110X_DISPLAYOFF               */
        0xD5,    /* SH110X_SETDISPLAYCLOCKDIV, 0x51 */
        0x51,
        0x81,    /* SH110X_SETCONTRAST, 0x4F        */
        0x4F,
        0xAD,    /* SH110X_DCDC, 0x8A               */
        0x8A,
        0xA1,    /* SH110X_SEGREMAP                 */
        0xC0,    /* SH110X_COMSCANINC               */
        0xDC,    /* SH110X_SETDISPSTARTLINE, 0x0    */
        0x00,
        0xD3,    /* SH110X_SETDISPLAYOFFSET, 0x60   */
        0x60,
        0xD9,    /* SH110X_SETPRECHARGE, 0x22       */
        0x22,
        0xDB,    /* SH110X_SETVCOMDETECT, 0x35      */
        0x35,
        0xA8,    /* SH110X_SETMULTIPLEX, 0x3F       */
        0x3F,
        0xA4,    /* SH110X_DISPLAYALLON_RESUME      */
        0xA6,    /* SH110X_NORMALDISPLAY            */
        0xAF     /* SH110x_DISPLAYON                */
    };

    gpio_clearPin(LCD_CS);
    gpio_clearPin(LCD_DC);  /* DC low -> command mode          */
    spi_send(&spi2, init, sizeof(init));
    gpio_setPin(LCD_CS);
}

void SH110x_terminate()
{
    uint8_t dispOff = 0xAE;

    gpio_clearPin(LCD_CS);
    gpio_clearPin(LCD_DC);  /* DC low -> command mode          */
    spi_send(&spi2, &dispOff, 1);
    gpio_setPin(LCD_CS);
}

void SH110x_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    gpio_clearPin(LCD_CS);

    uint8_t *frameBuffer = (uint8_t *) fb;
    for(uint8_t y = startRow; y < endRow; y++)
    {
        for(uint8_t x = 0; x < CONFIG_SCREEN_WIDTH/8; x++)
        {
            uint8_t cmd[3];
            cmd[0] = (y & 0x0F);     /* Set Y position         */
            cmd[1] = (0x10 | ((y >> 4) & 0x07));
            cmd[2] = (0xB0 | x);     /* Set X position         */

            gpio_clearPin(LCD_DC);              /* RS low -> command mode */
            spi_send(&spi2, cmd, sizeof(cmd));
            gpio_setPin(LCD_DC);                /* RS high -> data mode   */

            size_t pos = x + y * (CONFIG_SCREEN_WIDTH/8);
            spi_send(&spi2, &frameBuffer[pos], 1);
        }
    }

    gpio_setPin(LCD_CS);
}

void SH110x_render(void *fb)
{
    SH110x_renderRows(0, CONFIG_SCREEN_HEIGHT, fb);
}

void SH110x_setContrast(uint8_t contrast)
{
    uint8_t cmd[2];
    cmd[0] = 0x81;          /* Set Electronic Volume               */
    cmd[0] = contrast;      /* Controller contrast range is 0 - 63 */

    gpio_clearPin(LCD_CS);
    gpio_clearPin(LCD_DC);  /* RS low -> command mode              */
    spi_send(&spi2, cmd, sizeof(cmd));
    gpio_setPin(LCD_CS);
}

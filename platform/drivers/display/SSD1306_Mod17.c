/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                Mathis Schmieder DB9MAT                  *
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

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <peripherals/gpio.h>
#include <interfaces/display.h>
#include <interfaces/delays.h>
#include <hwconfig.h>
#include <SPI2.h>


/*
 * LCD framebuffer, statically allocated and placed in the "large" RAM block
 * starting at 0x20000000.
 * Pixel format is black and white, one bit per pixel.
 */
#define FB_SIZE (((CONFIG_SCREEN_HEIGHT * CONFIG_SCREEN_WIDTH) / 8 ) + 1)
static uint8_t __attribute__((section(".bss2"))) frameBuffer[FB_SIZE];

/**
 * \internal
 * Send one row of pixels to the display.
 * Pixels in framebuffer are stored "by rows", while display needs data to be
 * sent "by columns": this function performs the needed conversion.
 *
 * @param row: pixel row to be be sent.
 */
void display_renderRow(uint8_t row)
{
    for(uint16_t i = 0; i < 64; i++)
    {
        uint8_t out = 0;
        uint8_t tmp = frameBuffer[(i * 16) + (15 - row)];

        for(uint8_t j = 0; j < 8; j++)
        {
            out |= ((tmp >> (7-j)) & 0x01) << j;
        }

        spi2_sendRecv(out);
    }
}


void display_init()
{
    /* Clear framebuffer, setting all pixels to 0x00 makes the screen white */
    memset(frameBuffer, 0x00, FB_SIZE);

    /*
     * Initialise SPI2 for external flash and LCD
     */
    gpio_setMode(SPI2_CLK, ALTERNATE);
    gpio_setMode(SPI2_SDO, ALTERNATE);
    gpio_setMode(SPI2_SDI, ALTERNATE);
    gpio_setAlternateFunction(SPI2_CLK, 5); /* SPI2 is on AF5 */
    gpio_setAlternateFunction(SPI2_SDO, 5);
    gpio_setAlternateFunction(SPI2_SDI, 5);

    spi2_init();

    /*
     * Initialise GPIOs for LCD control
     */
    gpio_setMode(LCD_CS,  OUTPUT);
    gpio_setMode(LCD_RST, OUTPUT);
    gpio_setMode(LCD_RS,  OUTPUT);

    gpio_setPin(LCD_CS);
    gpio_clearPin(LCD_RS);

    gpio_clearPin(LCD_RST); // Reset controller
    delayMs(50);
    gpio_setPin(LCD_RST);
    delayMs(50);

    gpio_clearPin(LCD_CS);

    gpio_clearPin(LCD_RS);// RS low -> command mode
    spi2_sendRecv(0xAE);  // SH110X_DISPLAYOFF,
    spi2_sendRecv(0xd5);  // SH110X_SETDISPLAYCLOCKDIV, 0x51,
    spi2_sendRecv(0x51);
    spi2_sendRecv(0x81);  // SH110X_SETCONTRAST, 0x4F,
    spi2_sendRecv(0x4F);
    spi2_sendRecv(0xAD);  // SH110X_DCDC, 0x8A,
    spi2_sendRecv(0x8A);
    spi2_sendRecv(0xA0);  // SH110X_SEGREMAP,
    spi2_sendRecv(0xC0);  // SH110X_COMSCANINC,
    spi2_sendRecv(0xDC);  // SH110X_SETDISPSTARTLINE, 0x0,
    spi2_sendRecv(0x00);
    spi2_sendRecv(0xd3);  // SH110X_SETDISPLAYOFFSET, 0x60,
    spi2_sendRecv(0x60);
    spi2_sendRecv(0xd9);  // SH110X_SETPRECHARGE, 0x22,
    spi2_sendRecv(0x22);
    spi2_sendRecv(0xdb);  // SH110X_SETVCOMDETECT, 0x35,
    spi2_sendRecv(0x35);
    spi2_sendRecv(0xa8);  // SH110X_SETMULTIPLEX, 0x3F,
    spi2_sendRecv(0x3f);
    spi2_sendRecv(0xa4);  // SH110X_DISPLAYALLON_RESUME,
    spi2_sendRecv(0xa6);  // SH110X_NORMALDISPLAY,
    spi2_sendRecv(0xAF);  // SH110x_DISPLAYON
    gpio_setPin(LCD_CS);
}

void display_terminate()
{
    spi2_terminate();
}

void display_renderRows(uint8_t startRow, uint8_t endRow)
{
    gpio_clearPin(LCD_CS);

    for(uint8_t row = startRow; row <= endRow; row++)
    {
        gpio_clearPin(LCD_RS);            /* RS low -> command mode */
        (void) spi2_sendRecv(0xB0 | row); /* Set Y position         */
        (void) spi2_sendRecv(0x00);       /* Set X position         */
        (void) spi2_sendRecv(0x10);
        gpio_setPin(LCD_RS);              /* RS high -> data mode   */
        display_renderRow(row);
    }

    gpio_setPin(LCD_CS);
}

void display_render()
{
    display_renderRows(0, (CONFIG_SCREEN_WIDTH / 8) - 1);
}

void *display_getFrameBuffer()
{
    return (void *)(frameBuffer);
}

void display_setContrast(uint8_t contrast)
{
    gpio_clearPin(LCD_CS);

    gpio_clearPin(LCD_RS);             /* RS low -> command mode              */
    (void) spi2_sendRecv(0x81);        /* Set Electronic Volume               */
    (void) spi2_sendRecv(contrast);    /* Controller contrast range is 0 - 63 */

    gpio_setPin(LCD_CS);
}

void display_setBacklightLevel(uint8_t level)
{
    (void) level;
}

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
#include <interfaces/delays.h>
#include <hwconfig.h>
#include <SPI2.h>
#include "SH110x_Mod17.h"

void SH110x_init()
{
    gpio_setPin(LCD_CS);
    gpio_clearPin(LCD_DC);

    gpio_clearPin(LCD_RST); /* Reset controller                */
    delayMs(50);
    gpio_setPin(LCD_RST);
    delayMs(50);

    gpio_clearPin(LCD_CS);

    gpio_clearPin(LCD_DC);  /* DC low -> command mode          */
    spi2_sendRecv(0xAE);    /* SH110X_DISPLAYOFF               */
    spi2_sendRecv(0xD5);    /* SH110X_SETDISPLAYCLOCKDIV, 0x51 */
    spi2_sendRecv(0x51);
    spi2_sendRecv(0x81);    /* SH110X_SETCONTRAST, 0x4F        */
    spi2_sendRecv(0x4F);
    spi2_sendRecv(0xAD);    /* SH110X_DCDC, 0x8A               */
    spi2_sendRecv(0x8A);
    spi2_sendRecv(0xA1);    /* SH110X_SEGREMAP                 */
    spi2_sendRecv(0xC0);    /* SH110X_COMSCANINC               */
    spi2_sendRecv(0xDC);    /* SH110X_SETDISPSTARTLINE, 0x0    */
    spi2_sendRecv(0x00);
    spi2_sendRecv(0xD3);    /* SH110X_SETDISPLAYOFFSET, 0x60   */
    spi2_sendRecv(0x60);
    spi2_sendRecv(0xD9);    /* SH110X_SETPRECHARGE, 0x22       */
    spi2_sendRecv(0x22);
    spi2_sendRecv(0xDB);    /* SH110X_SETVCOMDETECT, 0x35      */
    spi2_sendRecv(0x35);
    spi2_sendRecv(0xA8);    /* SH110X_SETMULTIPLEX, 0x3F       */
    spi2_sendRecv(0x3F);
    spi2_sendRecv(0xA4);    /* SH110X_DISPLAYALLON_RESUME      */
    spi2_sendRecv(0xA6);    /* SH110X_NORMALDISPLAY            */
    spi2_sendRecv(0xAF);    /* SH110x_DISPLAYON                */
    gpio_setPin(LCD_CS);
}

void SH110x_terminate()
{
    spi2_sendRecv(0xAE);
}

void SH110x_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    gpio_clearPin(LCD_CS);

    uint8_t *frameBuffer = (uint8_t *) fb;
    for(uint8_t y = startRow; y < endRow; y++)
    {
        for(uint8_t x = 0; x < CONFIG_SCREEN_WIDTH/8; x++)
        {
            gpio_clearPin(LCD_DC);              /* RS low -> command mode */
            (void) spi2_sendRecv(y & 0x0F);     /* Set Y position         */
            (void) spi2_sendRecv(0x10 | ((y >> 4) & 0x07));
            (void) spi2_sendRecv(0xB0 | x);     /* Set X position         */
            gpio_setPin(LCD_DC);                /* RS high -> data mode   */

            size_t pos = x + y * (CONFIG_SCREEN_WIDTH/8);
            spi2_sendRecv(frameBuffer[pos]);
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
    gpio_clearPin(LCD_CS);

    gpio_clearPin(LCD_DC);             /* RS low -> command mode              */
    (void) spi2_sendRecv(0x81);        /* Set Electronic Volume               */
    (void) spi2_sendRecv(contrast);    /* Controller contrast range is 0 - 63 */

    gpio_setPin(LCD_CS);
}
/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                         Mathis Schmieder DB9MAT                         *
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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <interfaces/gpio.h>
#include <interfaces/display.h>
#include <interfaces/delays.h>
#include <hwconfig.h>
#include <SPI2.h>

/*
 * LCD framebuffer, allocated on the heap by display_init().
 * Pixel format is black and white, one bit per pixel
 */
static uint8_t *frameBuffer;

void display_init()
{

    /*
     * Framebuffer size, in bytes, with compensating for eventual truncation
     * error in division by rounding to the nearest greater integer.
     */
    unsigned int fbSize = (SCREEN_HEIGHT * SCREEN_WIDTH)/8;
    if((fbSize * 8) < (SCREEN_HEIGHT * SCREEN_WIDTH)) fbSize += 1;
    fbSize *= sizeof(uint8_t);

    /* Allocate framebuffer */
    frameBuffer = (uint8_t *) malloc(fbSize);
    if(frameBuffer == NULL)
    {
        puts("*** LCD ERROR: cannot allocate framebuffer! ***");
        return;
    }

    /* Clear framebuffer, setting all pixels to 0x00 makes the screen white */
    memset(frameBuffer, 0x00, fbSize);

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

    gpio_clearPin(LCD_RST); /* Reset controller                          */
    delayMs(50);
    gpio_setPin(LCD_RST);
    delayMs(50);

    gpio_clearPin(LCD_CS);

    gpio_clearPin(LCD_RS);  /* RS low -> command mode                                   */
    spi2_sendRecv(0xAE);    /* Disable Display                                          */
    spi2_sendRecv(0x20);    /* Set Memory Addressing Mode to horizontal addressing Mode */
    spi2_sendRecv(0x00);
    spi2_sendRecv(0xB0);    /* Set Page Start Address for Page Addressing Mode          */
    spi2_sendRecv(0xC8);    /* Set COM Output Scan Direction                            */
    spi2_sendRecv(0x00);    /* Set low column address                                   */
    spi2_sendRecv(0x10);    /* Set high column address                                  */
    spi2_sendRecv(0x40);    /* Set start line address                                   */
    spi2_sendRecv(0x81);    /* Set contrast                                             */
    spi2_sendRecv(0xCF);
    spi2_sendRecv(0xA1);    /* Set segment re-map 0 to 127                              */
    spi2_sendRecv(0xA6);    /* Set normal color                                         */
    spi2_sendRecv(0xA8);    /* Set multiplex ratio (1 to 64)                            */
    spi2_sendRecv(0x3F);
    spi2_sendRecv(0xA4);    /* Output follows RAM content                               */
    spi2_sendRecv(0xD3);    /* Set display offset                                       */
    spi2_sendRecv(0x00);    /* Not offset                                               */
    spi2_sendRecv(0xD5);    /* Set display clock divide ratio/oscillator frequency      */
    spi2_sendRecv(0xF0);    /* Set divide ratio                                         */
    spi2_sendRecv(0xD9);    /* Set pre-charge period                                    */
    spi2_sendRecv(0x22);
    spi2_sendRecv(0xDA);    /* Set com pins hardware configuration                      */
    spi2_sendRecv(0x12);
    spi2_sendRecv(0xDB);    /* Set vcomh                                                */
    spi2_sendRecv(0x40);
    spi2_sendRecv(0x8D);    /* Set DC-DC enable                                         */
    spi2_sendRecv(0x14);
    spi2_sendRecv(0xAF);    /* Enable Display                                           */
    gpio_setPin(LCD_CS);
}

void display_terminate()
{
    if(frameBuffer != NULL)
    {
        free(frameBuffer);
    }

    spi2_terminate();
}

void display_renderRow(uint8_t row)
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
            (void) spi2_sendRecv(tmp[s]);
        }
    }
}

void display_renderRows(uint8_t startRow, uint8_t endRow)
{
    gpio_clearPin(LCD_CS);

    for(uint8_t row = startRow; row < endRow; row++)
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
    display_renderRows(0, SCREEN_HEIGHT / 8);
}

bool display_renderingInProgress()
{
    return (gpio_readPin(LCD_CS) == 0);
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

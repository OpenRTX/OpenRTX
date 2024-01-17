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

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <peripherals/gpio.h>
#include <interfaces/display.h>
#include <interfaces/delays.h>
#include "hwconfig.h"
#include <SPI2.h>


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
            (void) spi2_sendRecv(tmp[s]);
        }
    }
}


void display_init()
{
    gpio_setMode(LCD_CS,  OUTPUT);
    gpio_setMode(LCD_RST, OUTPUT);
    gpio_setMode(LCD_RS,  OUTPUT);

    gpio_setPin(LCD_CS);
    gpio_clearPin(LCD_RS);

    gpio_clearPin(LCD_RST);     /* Reset controller                          */
    delayMs(1);
    gpio_setPin(LCD_RST);
    delayMs(5);


    spi2_lockDeviceBlocking();
    gpio_clearPin(LCD_CS);

    gpio_clearPin(LCD_RS);      /* RS low -> command mode                    */
    (void) spi2_sendRecv(0x2F); /* Voltage Follower On                       */
    (void) spi2_sendRecv(0x81); /* Set Electronic Volume                     */
    (void) spi2_sendRecv(0x15); /* Contrast, initial setting                 */
    (void) spi2_sendRecv(0xA2); /* Set Bias = 1/9                            */
    (void) spi2_sendRecv(0xA1); /* A0 Set SEG Direction                      */
    (void) spi2_sendRecv(0xC0); /* Set COM Direction                         */
    (void) spi2_sendRecv(0xA4); /* White background, black pixels            */
    (void) spi2_sendRecv(0xAF); /* Set Display Enable                        */
    gpio_clearPin(LCD_CS);
    spi2_releaseDevice();
}

void display_terminate()
{

}

void display_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    spi2_lockDeviceBlocking();
    gpio_clearPin(LCD_CS);

    for(uint8_t row = startRow; row < endRow; row++)
    {
        gpio_clearPin(LCD_RS);            /* RS low -> command mode */
        (void) spi2_sendRecv(0xB0 | row); /* Set Y position         */
        (void) spi2_sendRecv(0x10);       /* Set X position         */
        (void) spi2_sendRecv(0x04);
        gpio_setPin(LCD_RS);              /* RS high -> data mode   */
        display_renderRow(row, (uint8_t *) fb);
    }

    gpio_setPin(LCD_CS);
    spi2_releaseDevice();
}

void display_render(void *fb)
{
    display_renderRows(0, CONFIG_SCREEN_HEIGHT / 8, fb);
}

void display_setContrast(uint8_t contrast)
{
    spi2_lockDeviceBlocking();
    gpio_clearPin(LCD_CS);

    gpio_clearPin(LCD_RS);               /* RS low -> command mode              */
    (void) spi2_sendRecv(0x81);          /* Set Electronic Volume               */
    (void) spi2_sendRecv(contrast >> 2); /* Controller contrast range is 0 - 63 */

    gpio_setPin(LCD_CS);
    spi2_releaseDevice();
}

void display_setBacklightLevel(uint8_t level)
{
    (void) level;
}

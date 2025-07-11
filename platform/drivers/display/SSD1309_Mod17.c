/***************************************************************************
 *   Copyright (C) 2021 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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
#include <spi_stm32.h>
#include "SSD1309_Mod17.h"

extern const struct spiDevice spi2;

void SSD1309_init()
{
    gpio_setPin(LCD_CS);
    gpio_clearPin(LCD_DC);

    gpio_clearPin(LCD_RST); // Reset controller
    delayMs(50);
    gpio_setPin(LCD_RST);
    delayMs(50);

    static const uint8_t init[] =
    {
        0xAE,    // SSD1309_DISPLAYOFF,
        0xD5,    // Set display clock division
        0xF0,
        0xA8,    // Set multiplex ratio, 1/64
        0x3F,
        0x81,    // Set contrast control
        0x32,
        0xD9,    // Set pre-charge period
        0xF1,
        0xDB,    // Set VCOMH Deselect level
        0x30,
        0xAF
    };

    gpio_clearPin(LCD_CS);
    gpio_clearPin(LCD_DC);
    spi_send(&spi2, init, sizeof(init));
    gpio_setPin(LCD_CS);
}

void SSD1309_terminate()
{
    uint8_t dispOff = 0xAE;

    gpio_clearPin(LCD_CS);
    gpio_clearPin(LCD_DC);  /* DC low -> command mode          */
    spi_send(&spi2, &dispOff, 1);
    gpio_setPin(LCD_CS);
}

void SSD1309_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    gpio_clearPin(LCD_CS);

    // Convert rows to pages
    uint8_t startPage = startRow / 8;
    uint8_t endPage = endRow / 8;
    uint8_t cmd[3];

    gpio_clearPin(LCD_DC);
    cmd[0] = 0x20; // Set page addressing mode
    cmd[1] = 0x02;
    spi_send(&spi2, cmd, 2);

    uint8_t *framebuffer = (uint8_t *)fb;

    for(uint8_t page = startPage; page < endPage; page++)
    {
        gpio_clearPin(LCD_DC);
        cmd[0] = 0xB0 | page;
        cmd[1] = 0x00;
        cmd[1] = 0x10;
        spi_send(&spi2, cmd, 3);
        gpio_setPin(LCD_DC); // DC high -> data mode

        uint8_t topRow = page * 8;
        for(uint8_t col = 0; col < CONFIG_SCREEN_WIDTH; col++)
        {
            uint8_t data = 0;
            uint8_t bit_offset = col % 8; // Bit offset in the fb for the column we are refreshing
            // Gather the 8 rows of data
            for(uint8_t row = 0; row < 8; row++)
            {
                size_t pos = ((topRow + row) * CONFIG_SCREEN_WIDTH + col) / 8;
                uint8_t cell = framebuffer[pos];
                data |= ((cell >> bit_offset) & 0x01) << row;
            }

            spi_send(&spi2, &data, 1);
        }
    }

    gpio_setPin(LCD_CS);
}

void SSD1309_render(void *fb)
{
    static const uint8_t cmd[] =
    {
        0xB0,
        0x20, // Set horizontal addressing mode
        0x00,
        0x00,
        0x10
    };

    gpio_clearPin(LCD_CS);
    gpio_clearPin(LCD_DC);
    spi_send(&spi2, cmd, sizeof(cmd));
    gpio_setPin(LCD_DC); // DC high -> data mode

    uint8_t *framebuffer = (uint8_t *)fb;

    // Refresh the whole screen 8 rows by 8 rows
    for(uint8_t topRow = 0; topRow <= 56; topRow += 8)
    {
        for(uint8_t col = 0; col < CONFIG_SCREEN_WIDTH; col++)
        {
            uint8_t data = 0;
            uint8_t bit_offset = col % 8; // Bit offset in the fb for the column we are refreshing
            // Gather the 8 rows of data
            for(uint8_t subRow = 0; subRow < 8; subRow++)
            {
                size_t pos = ((topRow + subRow) * CONFIG_SCREEN_WIDTH + col) / 8;
                uint8_t cell = framebuffer[pos];
                data |= ((cell >> bit_offset) & 0x01) << subRow;
            }

            spi_send(&spi2, &data, 1);
        }
    }

    gpio_setPin(LCD_CS);
}

void SSD1309_setContrast(uint8_t contrast)
{
    uint8_t cmd[2];
    cmd[0] = 0x81;          /* Set Electronic Volume               */
    cmd[0] = contrast;      /* Controller contrast range is 0 - 63 */

    gpio_clearPin(LCD_CS);
    gpio_clearPin(LCD_DC);  /* RS low -> command mode              */
    spi_send(&spi2, cmd, sizeof(cmd));
    gpio_setPin(LCD_CS);
}

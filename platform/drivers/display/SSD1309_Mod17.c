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
#include "SSD1309_Mod17.h"

void SSD1309_init()
{
    gpio_setPin(LCD_CS);
    gpio_clearPin(LCD_DC);

    gpio_clearPin(LCD_RST); // Reset controller
    delayMs(50);
    gpio_setPin(LCD_RST);
    delayMs(50);

    gpio_clearPin(LCD_CS);

    gpio_clearPin(LCD_DC);// DC low -> command mode
    
    spi2_sendRecv(0xAE);  // SSD1309_DISPLAYOFF,
    spi2_sendRecv(0xD5);  // Set display clock division
    spi2_sendRecv(0xF0);
    spi2_sendRecv(0xA8);  // Set multiplex ratio, 1/64
    spi2_sendRecv(0x3F);
    spi2_sendRecv(0x81);  // Set contrast control
    spi2_sendRecv(0x32);
    spi2_sendRecv(0xD9);  // Set pre-charge period
    spi2_sendRecv(0xF1);
    spi2_sendRecv(0xDB);  // Set VCOMH Deselect level
    spi2_sendRecv(0x30);
    spi2_sendRecv(0xAF);
    gpio_setPin(LCD_CS);
}

void SSD1309_terminate()
{
    spi2_sendRecv(0xAE);
}

void SSD1309_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    gpio_clearPin(LCD_CS);

    // Convert rows to pages
    uint8_t startPage = startRow / 8;
    uint8_t endPage = endRow / 8;

    gpio_clearPin(LCD_DC);
    spi2_sendRecv(0x20); // Set page addressing mode
    spi2_sendRecv(0x02); 
    
    uint8_t *framebuffer = (uint8_t *)fb;

    for(uint8_t page = startPage; page < endPage; page++)
    {
        gpio_clearPin(LCD_DC);
        spi2_sendRecv(0xB0 | page);
        spi2_sendRecv(0x00);
        spi2_sendRecv(0x10);
        gpio_setPin(LCD_DC); // DC high -> data mode

        uint8_t topRow = page*8;

        for(uint8_t col = 0; col < CONFIG_SCREEN_WIDTH; col++)
        {
            uint8_t data = 0;
            uint8_t bit_offset = col%8; // Bit offset in the fb for the column we are refreshing
            // Gather the 8 rows of data
            for(uint8_t row = 0; row < 8; row++)
            {
                uint8_t cell =  framebuffer[((topRow+row)*CONFIG_SCREEN_WIDTH+col)/8];
                data |= ((cell>>bit_offset)&0x01) << row;
            }
            spi2_sendRecv(data);
        }
        
    }
    
    gpio_setPin(LCD_CS);
}

void SSD1309_render(void *fb)
{
    gpio_clearPin(LCD_CS);

    gpio_clearPin(LCD_DC);
    spi2_sendRecv(0xB0);
    spi2_sendRecv(0x20); // Set horizontal addressing mode
    spi2_sendRecv(0x00); 
    spi2_sendRecv(0x00);
    spi2_sendRecv(0x10);
    gpio_setPin(LCD_DC); // DC high -> data mode

    uint8_t *framebuffer = (uint8_t *)fb;

    // Refresh the whole screen 8 rows by 8 rows
    for(uint8_t topRow = 0; topRow <= 56; topRow += 8)
    {
        for(uint8_t col = 0; col < CONFIG_SCREEN_WIDTH; col++)
        {
            uint8_t data = 0;
            uint8_t bit_offset = col%8; // Bit offset in the fb for the column we are refreshing
            // Gather the 8 rows of data
            for(uint8_t subRow = 0; subRow < 8; subRow++)
            {
                uint8_t cell =  framebuffer[((topRow+subRow)*CONFIG_SCREEN_WIDTH+col)/8];
                data |= ((cell>>bit_offset)&0x01) << subRow;
            }
            spi2_sendRecv(data);
        }
        
    }
    
    gpio_setPin(LCD_CS); 
}

void SSD1309_setContrast(uint8_t contrast)
{
    gpio_clearPin(LCD_CS);

    gpio_clearPin(LCD_DC);             /* DC low -> command mode              */
    (void) spi2_sendRecv(0x81);        /* Set Electronic Volume               */
    (void) spi2_sendRecv(contrast);    /* Controller contrast range is 0 - 63 */

    gpio_setPin(LCD_CS);
}
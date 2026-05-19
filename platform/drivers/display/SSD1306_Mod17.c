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
#include "interfaces/display.h"
#include "interfaces/delays.h"
#include "hwconfig.h"
#include "drivers/SPI/spi_stm32.h"

extern const struct spiDevice spi2;

/**
 * \internal
 * Send one row of pixels to the display.
 * Pixels in framebuffer are stored "by rows", while display needs data to be
 * sent "by columns": this function performs the needed conversion.
 *
 * @param row: pixel row to be be sent.
 */
void SSD1306_renderRow(uint8_t row, uint8_t *frameBuffer)
{
    for(uint16_t i = 0; i < 64; i++)
    {
        uint8_t out = 0;
        uint8_t tmp = frameBuffer[(i * 16) + (15 - row)];

        for(uint8_t j = 0; j < 8; j++)
        {
            out |= ((tmp >> (7-j)) & 0x01) << j;
        }

        spi_send(&spi2, &out, 1);
    }
}


void SSD1306_init()
{
    gpio_setPin(LCD_CS);
    gpio_clearPin(LCD_RS);

    gpio_clearPin(LCD_RST); // Reset controller
    delayMs(50);
    gpio_setPin(LCD_RST);
    delayMs(50);

    static const uint8_t init[] =
    {
        0xAE,    // SH110X_DISPLAYOFF
        0xD5,    // SH110X_SETDISPLAYCLOCKDIV, 0x51
        0x51,
        0x81,    // SH110X_SETCONTRAST, 0x4F
        0x4F,
        0xAD,    // SH110X_DCDC, 0x8A
        0x8A,
        0xA0,    // SH110X_SEGREMAP
        0xC0,    // SH110X_COMSCANINC
        0xDC,    // SH110X_SETDISPSTARTLINE, 0x0
        0x00,
        0xD3,    // SH110X_SETDISPLAYOFFSET, 0x60
        0x60,
        0xD9,    // SH110X_SETPRECHARGE, 0x22
        0x22,
        0xDB,    // SH110X_SETVCOMDETECT, 0x35
        0x35,
        0xA8,    // SH110X_SETMULTIPLEX, 0x3F
        0x3F,
        0xA4,    // SH110X_DISPLAYALLON_RESUME
        0xA6,    // SH110X_NORMALDISPLAY
        0xAF     // SH110x_DISPLAYON
    };

    gpio_clearPin(LCD_CS);
    gpio_clearPin(LCD_DC);
    spi_send(&spi2, init, sizeof(init));
    gpio_setPin(LCD_CS);
}

void SSD1306_terminate()
{

}

void SSD1306_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    gpio_clearPin(LCD_CS);

    for(uint8_t row = startRow; row <= endRow; row++)
    {
        uint8_t cmd[3];
        cmd[0] = 0xB0 | row; /* Set Y position         */
        cmd[1] = 0x00;       /* Set X position         */
        cmd[2] = 0x10;

        gpio_clearPin(LCD_RS);            /* RS low -> command mode */
        spi_send(&spi2, cmd, sizeof(cmd));
        gpio_setPin(LCD_RS);              /* RS high -> data mode   */
        SSD1306_renderRow(row, (uint8_t *) fb);
    }

    gpio_setPin(LCD_CS);
}

void SSD1306_render(void *fb)
{
    SSD1306_renderRows(0, (CONFIG_SCREEN_WIDTH / 8) - 1, fb);
}

void SSD1306_setContrast(uint8_t contrast)
{
    uint8_t cmd[2];
    cmd[0] = 0x81;          /* Set Electronic Volume               */
    cmd[0] = contrast;      /* Controller contrast range is 0 - 63 */

    gpio_clearPin(LCD_CS);
    gpio_clearPin(LCD_DC);  /* RS low -> command mode              */
    spi_send(&spi2, cmd, sizeof(cmd));
    gpio_setPin(LCD_CS);
}

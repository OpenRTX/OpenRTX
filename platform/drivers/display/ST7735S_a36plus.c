/***************************************************************************
 *   Copyright (C) 2024 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                         Dual Tachyon                                    *
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

#include <interfaces/delays.h>
#include <interfaces/display.h>
#include <peripherals/gpio.h>
#include <hwconfig.h>
#include <stddef.h>
#include "gd32f3x0.h"
#include <graphics.h>

enum ST7735S_command
{
    ST7735S_CMD_NOP       = 0x00U,
    ST7735S_CMD_SWRESET   = 0x01U,
    ST7735S_CMD_RDDID     = 0x04U,
    ST7735S_CMD_RDDST     = 0x09U,
    ST7735S_CMD_RDDPM     = 0x0AU,
    ST7735S_CMD_RDDMADCTL = 0x0BU,
    ST7735S_CMD_RDDCOLMOD = 0x0CU,
    ST7735S_CMD_RDDIM     = 0x0DU,
    ST7735S_CMD_RDDSM     = 0x0EU,
    ST7735S_CMD_RDDSDR    = 0x0FU,
    ST7735S_CMD_SLPIN     = 0x10U,
    ST7735S_CMD_SLPOUT    = 0x11U,
    ST7735S_CMD_PTLON     = 0x12U,
    ST7735S_CMD_NORON     = 0x13U,
    ST7735S_CMD_INVOFF    = 0x20U,
    ST7735S_CMD_INVON     = 0x21U,
    ST7735S_CMD_GAMSET    = 0x26U,
    ST7735S_CMD_DISPOFF   = 0x28U,
    ST7735S_CMD_DISPON    = 0x29U,
    ST7735S_CMD_CASET     = 0x2AU,
    ST7735S_CMD_RASET     = 0x2BU,
    ST7735S_CMD_RAMWR     = 0x2CU,
    ST7735S_CMD_RGBSET    = 0x2DU,
    ST7735S_CMD_RAMRD     = 0x2EU,
    ST7735S_CMD_PTLAR     = 0x30U,
    ST7735S_CMD_SCRLAR    = 0x33U,
    ST7735S_CMD_TEOFF     = 0x34U,
    ST7735S_CMD_TEON      = 0x35U,
    ST7735S_CMD_MADCTL    = 0x36U,
    ST7735S_CMD_VSCSAD    = 0x37U,
    ST7735S_CMD_IDMOFF    = 0x38U,
    ST7735S_CMD_IDMON     = 0x39U,
    ST7735S_CMD_COLMOD    = 0x3AU,
    ST7735S_CMD_RDID1     = 0xDAU,
    ST7735S_CMD_RDID2     = 0xDBU,
    ST7735S_CMD_RDID3     = 0xDCU,
    ST7735S_CMD_FRMCTR1   = 0xB1U,
    ST7735S_CMD_FRMCTR2   = 0xB2U,
    ST7735S_CMD_FRMCTR3   = 0xB3U,
    ST7735S_CMD_INVCTR    = 0xB4U,
    ST7735S_CMD_PWCTR1    = 0xC0U,
    ST7735S_CMD_PWCTR2    = 0xC1U,
    ST7735S_CMD_PWCTR3    = 0xC2U,
    ST7735S_CMD_PWCTR4    = 0xC3U,
    ST7735S_CMD_PWCTR5    = 0xC4U,
    ST7735S_CMD_VMCTR1    = 0xC5U,
    ST7735S_CMD_VMOFCTR   = 0xC7U,
    ST7735S_CMD_WRID2     = 0xD1U,
    ST7735S_CMD_WRID3     = 0xD2U,
    ST7735S_CMD_NVFCTR1   = 0xD9U,
    ST7735S_CMD_NVFCTR2   = 0xDEU,
    ST7735S_CMD_NVFCTR3   = 0xDFU,
    ST7735S_CMD_GMCTRP1   = 0xE0U,
    ST7735S_CMD_GMCTRN1   = 0xE1U,
    ST7735S_CMD_GCV       = 0xFCU,
};

static inline void sendByte(uint8_t data)
{
    // Use SPI1 peripheral
    // while (spi_i2s_flag_get(SPI1, SPI_FLAG_TBE) == RESET)
    //     ;
    spi_i2s_data_transmit(SPI1, data);
    while (spi_i2s_flag_get(SPI1, SPI_FLAG_TRANS) != RESET)
        ;
}

static inline void sendShort(uint16_t val)
{
    gpio_clearPin(LCD_CS);

    sendByte((val >> 8) & 0xFF);
    sendByte( val       & 0xFF);

    gpio_setPin(LCD_CS);
}

static inline void sendCommand(uint8_t command)
{
    gpio_clearPin(LCD_DC);
    gpio_clearPin(LCD_CS);

    sendByte(command);

    gpio_setPin(LCD_CS);
    gpio_setPin(LCD_DC);
}

static inline void sendData(uint8_t data)
{
    gpio_clearPin(LCD_CS);

    sendByte(data);

    gpio_setPin(LCD_CS);
}

static inline void setPosition(uint16_t x, uint16_t y)
{
    sendCommand(ST7735S_CMD_CASET);
    sendShort(y);
    sendCommand(ST7735S_CMD_RASET);
    sendShort(x);
}


void display_init(void)
{
    // gpio_setMode(LCD_PWR, OUTPUT);
    // gpio_setMode(LCD_DC,  OUTPUT);
    // gpio_setMode(LCD_CS,  OUTPUT);
    // gpio_setMode(LCD_CLK, OUTPUT);
    // gpio_setMode(LCD_DAT, OUTPUT);
    // gpio_setMode(LCD_RST, OUTPUT);


    // Reset display controller
    gpio_setPin(LCD_RST);
    delayMs(1);
    gpio_clearPin(LCD_RST);
    delayMs(1);
    gpio_setPin(LCD_RST);
    delayMs(1);

    sendCommand(ST7735S_CMD_SLPOUT);
    delayMs(1);

    sendCommand(ST7735S_CMD_FRMCTR1);
    sendData(0x05);
    sendData(0x3C);
    sendData(0x3C);
    sendCommand(ST7735S_CMD_FRMCTR2);
    sendData(0x05);
    sendData(0x3C);
    sendData(0x3C);
    sendCommand(ST7735S_CMD_FRMCTR3);
    sendData(0x05);
    sendData(0x3C);
    sendData(0x3C);
    sendData(0x05);
    sendData(0x3C);
    sendData(0x3C);
    sendCommand(ST7735S_CMD_INVCTR);
    sendData(0x03);
    sendCommand(ST7735S_CMD_PWCTR1);
    sendData(0x28);
    sendData(0x08);
    sendData(0x04);
    sendCommand(ST7735S_CMD_PWCTR2);
    sendData(0xC0);
    sendCommand(ST7735S_CMD_PWCTR3);
    sendData(0x0D);
    sendData(0x00);
    sendCommand(ST7735S_CMD_PWCTR4);
    sendData(0x8D);
    sendData(0x2A);
    sendCommand(ST7735S_CMD_PWCTR5);
    sendData(0x8D);
    sendData(0xEE);
    sendCommand(ST7735S_CMD_VMCTR1);
    sendData(0x1A);
    sendCommand(ST7735S_CMD_MADCTL);
    sendData(0xC8);
    sendCommand(ST7735S_CMD_GMCTRP1);
    sendData(0x04);
    sendData(0x22);
    sendData(0x07);
    sendData(0x0A);
    sendData(0x2E);
    sendData(0x30);
    sendData(0x25);
    sendData(0x2A);
    sendData(0x28);
    sendData(0x26);
    sendData(0x2E);
    sendData(0x3A);
    sendData(0x00);
    sendData(0x01);
    sendData(0x03);
    sendData(0x13);
    sendCommand(ST7735S_CMD_GMCTRN1);
    sendData(0x04);
    sendData(0x16);
    sendData(0x06);
    sendData(0x0D);
    sendData(0x2D);
    sendData(0x26);
    sendData(0x23);
    sendData(0x27);
    sendData(0x27);
    sendData(0x25);
    sendData(0x2D);
    sendData(0x3B);
    sendData(0x00);
    sendData(0x01);
    sendData(0x04);
    sendData(0x13);
    sendCommand(ST7735S_CMD_COLMOD);
    sendData(0x05);
    sendCommand(ST7735S_CMD_DISPON);
    // Display power on
    // gpio_setPin(LCD_PWR);
    // display_setBacklightLevel(5);
}

void *display_getFrameBuffer()
{
    return NULL;
}

void display_terminate()
{

}

void display_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    (void) startRow;
    (void) endRow;
    (void) fb;
}

void display_render(void *fb)
{
    (void) fb;
}

void display_setWindow(uint16_t x, uint16_t y, uint16_t height, uint16_t width)
{
    // Set column address
    sendCommand(ST7735S_CMD_CASET);
    sendShort(x);
    sendShort(x + height - 1);
    // Set row address
    sendCommand(ST7735S_CMD_RASET);
    sendShort(y);
    sendShort(y + 28 + width - 1);

    sendCommand(ST7735S_CMD_RAMWR);
}

void display_sendRawPixel(uint16_t data)
{
    sendShort(data);
}

void display_defineScrollArea(uint16_t x, uint16_t x2)
{

	/* tfa: top fixed area: nr of line from top of the frame mem and display) */
	uint16_t tfa = 160 - x2 + 1;
	/* vsa: height of the vertical scrolling area in nr of line of the frame mem
	   (not the display) from the vertical scrolling address. the first line appears
	   immediately after the bottom most line of the top fixed area. */
	uint16_t vsa = x2 - x + 1;
	/* bfa: bottom fixed are in nr of lines from bottom of the frame memory and display */
	uint16_t bfa = x + 1;

	if (tfa + vsa + bfa < 162)
		return;

	/* reset mv */

	//sendCommand(ST7735S_CMD_MADCTL);                
	//sendShort(0xC8 & ~(1 << 5));

	sendCommand(ST7735S_CMD_SCRLAR);

	sendShort(tfa);
	sendShort(vsa);
	sendShort(bfa);
}

void display_scroll(uint8_t line)
{
	sendCommand(ST7735S_CMD_VSCSAD);
	sendShort(line);
}

void display_clearWindow(uint16_t x, uint16_t y, uint16_t height, uint16_t width)
{
    display_setWindow(x, y, height, width);
    // setPosition(x, y);
    sendCommand(ST7735S_CMD_RAMWR);
    for(size_t i = 0; i < (width*height); i++)
        sendShort(0x0000);
}

void display_fill(uint32_t color)
{
    setPosition(28, 0);
    sendCommand(ST7735S_CMD_RAMWR);
    for(size_t i = 0; i < (CONFIG_SCREEN_WIDTH * CONFIG_SCREEN_HEIGHT); i++)
        sendShort(color & 0x0000FFFF);
}

void display_setPixel(uint16_t x, uint16_t y, uint32_t color)
{
    setPosition(x+28, CONFIG_SCREEN_HEIGHT - y);
    sendCommand(ST7735S_CMD_RAMWR);
    sendShort(color & 0x0000FFFF);
}

void display_setBacklightLevel(uint8_t level)
{
     if(level > 100)
        level = 100;

    uint32_t pwmLevel = (level / 100.0) * 255;
    TIMER_CH0CV(TIMER16) = pwmLevel;
}

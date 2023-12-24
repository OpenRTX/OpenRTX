/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#ifndef DRIVER_ST7735S_H
#define DRIVER_ST7735S_H

//#define ST7735_NOP 0x0
//#define ST7735_SWRESET 0x01
//#define ST7735_RDDID 0x04
//#define ST7735_RDDST 0x09

//#define ST7735_SLPIN 0x10
//#define ST7735_SLPOUT 0x11
//#define ST7735_PTLON 0x12
//#define ST7735_NORON 0x13

//#define ST7735_INVOFF 0x20
//#define ST7735_INVON 0x21
//#define ST7735_DISPOFF 0x28
//#define ST7735_DISPON 0x29
#define ST7735_CASET 0x2A
#define ST7735_RASET 0x2B
#define ST7735_RAMWR 0x2C
//#define ST7735_RAMRD 0x2E

#define ST7735_SCRLAR 0x33 /* Scroll Area Set */
//#define ST7735_COLMOD 0x3A
#define ST7735_MADCTL 0x36

#define ST7735_VSCSAD 0x37 /* Vertical Scroll Start Address of RAM */

//#define ST7735_FRMCTR1 0xB1
//#define ST7735_FRMCTR2 0xB2
//#define ST7735_FRMCTR3 0xB3
//#define ST7735_INVCTR 0xB4
//#define ST7735_DISSET5 0xB6

//#define ST7735_PWCTR1 0xC0
//#define ST7735_PWCTR2 0xC1
//#define ST7735_PWCTR3 0xC2
//#define ST7735_PWCTR4 0xC3
//#define ST7735_PWCTR5 0xC4
//#define ST7735_VMCTR1 0xC5

//#define ST7735_RDID1 0xDA
//#define ST7735_RDID2 0xDB
//#define ST7735_RDID3 0xDC
//#define ST7735_RDID4 0xDD

//#define ST7735_PWCTR6 0xFC

//#define ST7735_GMCTRP1 0xE0
//#define ST7735_GMCTRN1 0xE1

#include <stdint.h>

enum ST7735S_Command_t {
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

typedef enum ST7735S_Command_t ST7735S_Command_t;

void ST7735S_SendCommand(ST7735S_Command_t Command);
void DISPLAY_FillColor(uint16_t Color);
void ST7735S_SendData(uint8_t Data);
void ST7735S_SetPosition(uint8_t X, uint8_t Y);
void ST7735S_SendU16(uint16_t Data);
void ST7735S_SetPixel(uint8_t X, uint8_t Y, uint16_t Color);
void display_init(void);
void ST7735S_SetAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void ST7735S_defineScrollArea(uint16_t x, uint16_t x2);
void ST7735S_scroll(uint8_t line);

#endif


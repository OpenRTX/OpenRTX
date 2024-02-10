/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Morgan Diepart ON4MOD                    *
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
#include <hwconfig.h>
#include <interfaces/platform.h>
#include "SH110x_Mod17.h"
#include "SSD1309_Mod17.h"
#include <SPI2.h>

const hwInfo_t *hwInfo = NULL;
Mod17_HwInfo_t *mod17HwInfo = NULL;

typedef struct {
    void (*init)(void);
    void (*terminate)(void);
    void (*renderRows)(uint8_t , uint8_t, void *);
    void (*render)(void *);
    void (*setContrast)(uint8_t);
    void (*setBacklightLevel)(uint8_t);
} display_fcts_t;

display_fcts_t disp_fcts = {
    .init                   = NULL,
    .terminate              = NULL,
    .renderRows             = NULL,
    .render                 = NULL,
    .setContrast            = NULL,
};

void display_init()
{
    hwInfo = platform_getHwInfo();
    mod17HwInfo = (Mod17_HwInfo_t *)hwInfo->other;

    /*
     * Initialise SPI2 for external flash and LCD
     */
    gpio_setMode(SPI2_SCK, ALTERNATE);
    gpio_setMode(SPI2_MOSI, ALTERNATE);
    gpio_setMode(SPI2_MISO, ALTERNATE);
    gpio_setAlternateFunction(SPI2_SCK, 5); /* SPI2 is on AF5 */
    gpio_setAlternateFunction(SPI2_MOSI, 5);
    gpio_setAlternateFunction(SPI2_MISO, 5);
    spi2_init();

    /*
     * Initialise GPIOs for LCD control
     */
    gpio_setMode(LCD_CS,  OUTPUT);
    gpio_setMode(LCD_RST, OUTPUT);
    gpio_setMode(LCD_DC,  OUTPUT);


    if(hwInfo->hw_version >= CONFIG_HWVER_1_0 && mod17HwInfo->HMI_present)
    {
        disp_fcts.init                  = SSD1309_init;
        disp_fcts.renderRows            = SSD1309_renderRows;
        disp_fcts.render                = SSD1309_render;
        disp_fcts.setContrast           = SSD1309_setContrast;
        disp_fcts.terminate             = SSD1309_terminate;

    }
    else 
    {
        disp_fcts.init                  = SH110x_init;
        disp_fcts.renderRows            = SH110x_renderRows;
        disp_fcts.render                = SH110x_render;
        disp_fcts.setContrast           = SH110x_setContrast;
        disp_fcts.terminate             = SH110x_terminate;
    }
    
    disp_fcts.init();
}

void display_terminate()
{
    disp_fcts.terminate();
    spi2_terminate();
}

void display_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    disp_fcts.renderRows(startRow, endRow, fb);
}

void display_render(void *fb)
{
    disp_fcts.render(fb);
}

void display_setContrast(uint8_t contrast)
{
    disp_fcts.setContrast(contrast);
}

void display_setBacklightLevel(uint8_t level)
{
    (void) level;
}

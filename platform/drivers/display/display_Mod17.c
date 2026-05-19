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
#include "hwconfig.h"
#include "interfaces/platform.h"
#include "drivers/SPI/spi_stm32.h"
#include "SH110x_Mod17.h"
#include "SSD1309_Mod17.h"

SPI_STM32_DEVICE_DEFINE(spi2, SPI2, NULL)

struct displayFuncs
{
    void (*init)(void);
    void (*terminate)(void);
    void (*renderRows)(uint8_t, uint8_t, void *);
    void (*render)(void *);
    void (*setContrast)(uint8_t);
    void (*setBacklightLevel)(uint8_t);
};

static struct displayFuncs display =
{
    .init        = NULL,
    .terminate   = NULL,
    .renderRows  = NULL,
    .render      = NULL,
    .setContrast = NULL,
};

void display_init()
{
    const hwInfo_t *hwinfo = platform_getHwInfo();
    if(((hwinfo->flags & MOD17_FLAGS_HMI_PRESENT) != 0) &&
       ((hwinfo->hw_version >> 8) == MOD17_HMI_V10))
    {
        display.init        = SSD1309_init;
        display.renderRows  = SSD1309_renderRows;
        display.render      = SSD1309_render;
        display.setContrast = SSD1309_setContrast;
        display.terminate   = SSD1309_terminate;
    }
    else
    {
        display.init        = SH110x_init;
        display.renderRows  = SH110x_renderRows;
        display.render      = SH110x_render;
        display.setContrast = SH110x_setContrast;
        display.terminate   = SH110x_terminate;
    }

    /*
     * Initialise SPI2 for external flash and LCD
     */
    gpio_setMode(SPI2_SCK,  ALTERNATE | ALTERNATE_FUNC(5));
    gpio_setMode(SPI2_MOSI, ALTERNATE | ALTERNATE_FUNC(5));
    gpio_setMode(SPI2_MISO, ALTERNATE | ALTERNATE_FUNC(5));
    spiStm32_init(&spi2, 1300000, 0);

    /*
     * Initialise GPIOs for LCD control
     */
    gpio_setMode(LCD_CS,  OUTPUT);
    gpio_setMode(LCD_RST, OUTPUT);
    gpio_setMode(LCD_DC,  OUTPUT);

    display.init();
}

void display_terminate()
{
    display.terminate();
    spiStm32_terminate(&spi2);
}

void display_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    display.renderRows(startRow, endRow, fb);
}

void display_render(void *fb)
{
    display.render(fb);
}

void display_setContrast(uint8_t contrast)
{
    display.setContrast(contrast);
}

void display_setBacklightLevel(uint8_t level)
{
    (void) level;
}

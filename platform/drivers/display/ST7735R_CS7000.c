/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "peripherals/gpio.h"
#include "interfaces/display.h"
#include "interfaces/delays.h"
#include "interfaces/platform.h"
#include "drivers/backlight/backlight.h"
#include "hwconfig.h"
#include <string.h>

enum ST775RCmd
{
    ST775R_CMD_NOP       = 0x00,
    ST775R_CMD_SWRESET   = 0x01,
    ST775R_CMD_RDID      = 0x04,
    ST775R_CMD_RDDST     = 0x09,
    ST775R_CMD_RDDPM     = 0x0A,
    ST775R_CMD_RDDMADCTL = 0x0B,
    ST775R_CMD_RDDCOLMOD = 0x0C,
    ST775R_CMD_RDDIM     = 0x0D,
    ST775R_CMD_RDDSM     = 0x0E,
    ST775R_CMD_SLPIN     = 0x10,
    ST775R_CMD_SLPOUT    = 0x11,
    ST775R_CMD_PTLON     = 0x12,
    ST775R_CMD_NORON     = 0x13,
    ST775R_CMD_INVOFF    = 0x20,
    ST775R_CMD_INVON     = 0x21,
    ST775R_CMD_GAMSET    = 0x26,
    ST775R_CMD_DISPOFF   = 0x28,
    ST775R_CMD_DISPON    = 0x29,
    ST775R_CMD_CASET     = 0x2A,
    ST775R_CMD_RASET     = 0x2B,
    ST775R_CMD_RAMWR     = 0x2C,
    ST775R_CMD_RGBSET    = 0x2D,
    ST775R_CMD_RAMRD     = 0x2E,
    ST775R_CMD_PTLAR     = 0x30,
    ST775R_CMD_TEOFF     = 0x34,
    ST775R_CMD_TEON      = 0x35,
    ST775R_CMD_MADCTL    = 0x36,
    ST775R_CMD_IDMOFF    = 0x38,
    ST775R_CMD_IDMON     = 0x39,
    ST775R_CMD_COLMOD    = 0x3A,
    ST775R_CMD_RDID1     = 0xDA,
    ST775R_CMD_RDID2     = 0xDB,
    ST775R_CMD_RDID3     = 0xDC
};

static inline void sendCmd(uint8_t cmd)
{
    // Set D/C low (command mode), clear WR and data lines
    GPIOD->BSRR = 0x30FF0000;
#ifdef PLATFORM_CS7000P
    asm volatile("           mov   r1, #65    \n"
#else
    asm volatile("           mov   r1, #21    \n"
#endif
                 "___loop_d: cmp   r1, #0     \n"
                 "           itt   ne         \n"
                 "           subne r1, r1, #1 \n"
                 "           bne   ___loop_d  \n":::"r1");
    GPIOD->BSRR = cmd;

    asm volatile("            mov   r1, #5     \n"
                 "___loop_1:  cmp   r1, #0     \n"
                 "            itt   ne         \n"
                 "            subne r1, r1, #1 \n"
                 "            bne   ___loop_1  \n":::"r1");
    GPIOD->BSRR = (1 << 13);
}

static inline void sendData(uint8_t val)
{
    // Set D/C high (data mode), clear WR and data lines
    GPIOD->BSRR = 0x20FF1000;
#ifdef PLATFORM_CS7000P
    asm volatile("           mov   r1, #65    \n"
#else
    asm volatile("           mov   r1, #21    \n"
#endif
                 "___loop_e: cmp   r1, #0     \n"
                 "           itt   ne         \n"
                 "           subne r1, r1, #1 \n"
                 "           bne   ___loop_e  \n":::"r1");
    GPIOD->BSRR = val;

    asm volatile("            mov   r1, #5     \n"
                 "___loop_2:  cmp   r1, #0     \n"
                 "            itt   ne         \n"
                 "            subne r1, r1, #1 \n"
                 "            bne   ___loop_2  \n":::"r1");
    GPIOD->BSRR = (1 << 13);
}

void display_init()
{
    backlight_init();

    /* Set up gpios */
    gpio_setMode(LCD_D0,  OUTPUT);
    gpio_setMode(LCD_D1,  OUTPUT);
    gpio_setMode(LCD_D2,  OUTPUT);
    gpio_setMode(LCD_D3,  OUTPUT);
    gpio_setMode(LCD_D4,  OUTPUT);
    gpio_setMode(LCD_D5,  OUTPUT);
    gpio_setMode(LCD_D6,  OUTPUT);
    gpio_setMode(LCD_D7,  OUTPUT);
    gpio_setMode(LCD_WR,  OUTPUT);
    gpio_setMode(LCD_RD,  OUTPUT);
    gpio_setMode(LCD_DC,  OUTPUT);
    gpio_setMode(LCD_RST, OUTPUT);
    gpio_setMode(LCD_CS,  OUTPUT);

    gpio_setOutputSpeed(LCD_D0,  HIGH);
    gpio_setOutputSpeed(LCD_D1,  HIGH);
    gpio_setOutputSpeed(LCD_D2,  HIGH);
    gpio_setOutputSpeed(LCD_D3,  HIGH);
    gpio_setOutputSpeed(LCD_D4,  HIGH);
    gpio_setOutputSpeed(LCD_D5,  HIGH);
    gpio_setOutputSpeed(LCD_D6,  HIGH);
    gpio_setOutputSpeed(LCD_D7,  HIGH);
    gpio_setOutputSpeed(LCD_WR,  HIGH);
    gpio_setOutputSpeed(LCD_RD,  HIGH);
    gpio_setOutputSpeed(LCD_DC,  HIGH);
    gpio_setOutputSpeed(LCD_RST, HIGH);
    gpio_setOutputSpeed(LCD_CS,  HIGH);

    gpio_clearPin(LCD_RST); /* Put LCD in reset mode       */
    gpio_setPin(LCD_CS);    /* CS idle state is high level */
    gpio_setPin(LCD_DC);    /* Idle state for DC line      */
    gpio_setPin(LCD_WR);    /* Idle state for WR line      */
    gpio_setPin(LCD_RD);    /* Idle state for RD line      */
    gpio_setPin(LCD_RST);   /* Exit from reset             */

    delayMs(10);

    gpio_clearPin(LCD_CS);

    sendCmd(ST775R_CMD_SWRESET);
    sendCmd(0xb1);    /* Undocumented command */
    sendData(5);
    sendData(8);
    sendData(5);
    sendCmd(0xb2);    /* Undocumented command */
    sendData(5);
    sendData(8);
    sendData(5);
    sendCmd(0xb3);    /* Undocumented command */
    sendData(5);
    sendData(8);
    sendData(5);
    sendData(5);
    sendData(8);
    sendData(5);
    sendCmd(0xb4);    /* Undocumented command */
    sendData(0);
    sendCmd(0xb6);    /* Undocumented command */
    sendData(0xb4);
    sendData(0xf0);
    sendCmd(0xc0);    /* Undocumented command */
    sendData(0xa2);
    sendData(2);
    /* sendData(0x85); TODO: see original fw  */
    sendData(0x84);
    sendCmd(0xc1);    /* Undocumented command */
    sendData(5);
    sendCmd(0xc2);    /* Undocumented command */
    sendData(10);
    sendData(0);
    sendCmd(0xc3);    /* Undocumented command */
    sendData(0x8a);
    sendData(0x2a);
    sendCmd(0xc4);    /* Undocumented command */
    sendData(0x8a);
    sendData(0xee);
    sendCmd(0xc5);    /* Undocumented command */
    sendData(0xe);
    sendCmd(ST775R_CMD_MADCTL);
    sendData(0xB0);
    sendCmd(0xe0);    /* Undocumented command */
    sendData(5);
    /* sendData(0x28); TODO: see original fw  */
    /* sendData(0x28); TODO: see original fw  */
    sendData(0x16);
    sendData(0xf);
    sendData(0x18);
    sendData(0x2f);
    sendData(0x28);
    sendData(0x20);
    sendData(0x22);
    sendData(0x1f);
    sendData(0x1b);
    sendData(0x23);
    sendData(0x37);
    sendData(0);
    sendData(7);
    sendData(2);
    sendData(0x10);
    sendCmd(0xe1);    /* Undocumented command */
    sendData(7);
    /* sendData(0x28); TODO: see original fw  */
    /* sendData(0x28); TODO: see original fw  */
    sendData(0x1b);
    sendData(0xf);
    sendData(0x17);
    sendData(0x33);
    sendData(0x2c);
    sendData(0x29);
    sendData(0x2e);
    sendData(0x30);
    sendData(0x30);
    sendData(0x39);
    sendData(0x3f);
    sendData(0);
    sendData(7);
    sendData(3);
    sendData(0x10);
    sendCmd(0xf0);    /* Undocumented command */
    sendData(1);
    sendCmd(0xf6);    /* Undocumented command */
    sendData(0);
    sendCmd(ST775R_CMD_COLMOD);
    sendData(0x05);   /* 16bpp - RGB 565      */
    sendCmd(ST775R_CMD_RASET);
    sendData(0);
    sendData(0);
    sendData(0);
    sendData(0x7f);   /* 128 rows             */
    sendCmd(ST775R_CMD_CASET);
    sendData(0);
    sendData(0);
    sendData(0);
    sendData(0x9f);   /* 160 columns          */

    /* Exit from sleep */
    sendCmd(ST775R_CMD_SLPOUT);
    delayMs(120);

    /* Enable display */
    sendCmd(ST775R_CMD_DISPON);

    gpio_setPin(LCD_CS);
}

void display_terminate()
{
}

void display_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    /*
     * Put screen data lines back to output mode, since they are in common with
     * keyboard buttons and the keyboard driver sets them as inputs.
     */
    gpio_setMode(LCD_D0, OUTPUT);
    gpio_setMode(LCD_D1, OUTPUT);
    gpio_setMode(LCD_D2, OUTPUT);
    gpio_setMode(LCD_D3, OUTPUT);
    gpio_setMode(LCD_D4, OUTPUT);
    gpio_setMode(LCD_D5, OUTPUT);
    gpio_setMode(LCD_D6, OUTPUT);
    gpio_setMode(LCD_D7, OUTPUT);

    /*
     * Select the display, configure start and end rows in display driver and
     * write to display memory.
     */
    gpio_clearPin(LCD_CS);

    sendCmd(ST775R_CMD_RASET);
    sendData(0x00);
    sendData(startRow);
    sendData(0x00);
    sendData(endRow);
    sendCmd(ST775R_CMD_RAMWR);

    for(uint8_t y = startRow; y < endRow; y++)
    {
        for(uint8_t x = 0; x < CONFIG_SCREEN_WIDTH; x++)
        {
            size_t pos = x + y * CONFIG_SCREEN_WIDTH;
            uint16_t pixel = ((uint16_t *) fb)[pos];
            sendData((pixel >> 8) & 0xff);
            sendData(pixel & 0xff);
        }
    }

    gpio_setPin(LCD_CS);
}

void display_render(void *fb)
{
    display_renderRows(0, CONFIG_SCREEN_HEIGHT, fb);
}

void display_setContrast(uint8_t contrast)
{
    /* This controller does not support contrast regulation */
    (void) contrast;
}

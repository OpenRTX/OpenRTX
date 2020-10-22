/***************************************************************************
 *   Copyright (C) 2020 by Silvano Seva IU2KWO and Niccolò Izzo IU2KIN     *
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
#include <os.h>
#include "gpio.h"
#include "display.h"
#include "delays.h"

/* Defines for GPIO control, really ugly but useful. */
#define D0  GPIOD,14
#define D1  GPIOD,15
#define D2  GPIOD,0
#define D3  GPIOD,1
#define D4  GPIOE,7
#define D5  GPIOE,8
#define D6  GPIOE,9
#define D7  GPIOE,10
#define WR  GPIOD,5
#define RD  GPIOD,4
#define CS  GPIOD,6
#define RS  GPIOD,12
#define RST GPIOD,13

/**
 * LCD command set, basic and extended
 */

#define CMD_NOP          0x00 // No Operation
#define CMD_SWRESET      0x01 // Software reset
#define CMD_RDDIDIF      0x04 // Read Display ID Info
#define CMD_RDDST        0x09 // Read Display Status
#define CMD_RDDPM        0x0a // Read Display Power
#define CMD_RDD_MADCTL   0x0b // Read Display
#define CMD_RDD_COLMOD   0x0c // Read Display Pixel
#define CMD_RDDDIM       0x0d // Read Display Image
#define CMD_RDDSM        0x0e // Read Display Signal
#define CMD_RDDSDR       0x0f // Read display self-diagnostic resut
#define CMD_SLPIN        0x10 // Sleep in & booster off
#define CMD_SLPOUT       0x11 // Sleep out & booster on
#define CMD_PTLON        0x12 // Partial mode on
#define CMD_NORON        0x13 // Partial off (Normal)
#define CMD_INVOFF       0x20 // Display inversion off
#define CMD_INVON        0x21 // Display inversion on
#define CMD_GAMSET       0x26 // Gamma curve select
#define CMD_DISPOFF      0x28 // Display off
#define CMD_DISPON       0x29 // Display on
#define CMD_CASET        0x2a // Column address set
#define CMD_RASET        0x2b // Row address set
#define CMD_RAMWR        0x2c // Memory write
#define CMD_RGBSET       0x2d // LUT parameter (16-to-18 color mapping)
#define CMD_RAMRD        0x2e // Memory read
#define CMD_PTLAR        0x30 // Partial start/end address set
#define CMD_VSCRDEF      0x31 // Vertical Scrolling Direction
#define CMD_TEOFF        0x34 // Tearing effect line off
#define CMD_TEON         0x35 // Tearing effect mode set & on
#define CMD_MADCTL       0x36 // Memory data access control
#define CMD_VSCRSADD     0x37 // Vertical scrolling start address
#define CMD_IDMOFF       0x38 // Idle mode off
#define CMD_IDMON        0x39 // Idle mode on
#define CMD_COLMOD       0x3a // Interface pixel format
#define CMD_RDID1        0xda // Read ID1
#define CMD_RDID2        0xdb // Read ID2
#define CMD_RDID3        0xdc // Read ID3

#define CMD_SETOSC       0xb0 // Set internal oscillator
#define CMD_SETPWCTR     0xb1 // Set power control
#define CMD_SETDISPLAY   0xb2 // Set display control
#define CMD_SETCYC       0xb4 // Set display cycle
#define CMD_SETBGP       0xb5 // Set BGP voltage
#define CMD_SETVCOM      0xb6 // Set VCOM voltage
#define CMD_SETEXTC      0xb9 // Enter extension command
#define CMD_SETOTP       0xbb // Set OTP
#define CMD_SETSTBA      0xc0 // Set Source option
#define CMD_SETID        0xc3 // Set ID
#define CMD_SETPANEL     0xcc // Set Panel characteristics
#define CMD_GETHID       0xd0 // Read Himax internal ID
#define CMD_SETGAMMA     0xe0 // Set Gamma
#define CMD_SET_SPI_RDEN 0xfe // Set SPI Read address (and enable)
#define CMD_GET_SPI_RDEN 0xff // Get FE A[7:0] parameter

/* Addresses for memory-mapped display data and command (through FSMC) */
#define LCD_FSMC_ADDR_COMMAND 0x60000000
#define LCD_FSMC_ADDR_DATA    0x60040000

/* Screen dimensions */
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 128

/*
 * LCD framebuffer, allocated on the heap by display_init().
 * Pixel format is RGB565, 16 bit per pixel
 */
static uint16_t *frameBuffer;

void __attribute__((used)) DMA2_Stream7_IRQHandler()
{
    OSIntEnter();
    DMA2->HIFCR |= DMA_HIFCR_CTCIF7 | DMA_HIFCR_CTEIF7;    /* Clear flags */
    gpio_setPin(CS);
    OSIntExit();
}

static inline __attribute__((__always_inline__)) void writeCmd(uint8_t cmd)
{
    *((volatile uint8_t*) LCD_FSMC_ADDR_COMMAND) = cmd;
}

static inline __attribute__((__always_inline__)) void writeData(uint8_t val)
{
    *((volatile uint8_t*) LCD_FSMC_ADDR_DATA) = val;
}

void display_init()
{
    /* Framebuffer size, in bytes */
    const size_t fbSize = SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t);

    /* Allocate framebuffer, two bytes per pixel */
    frameBuffer = (uint16_t *) malloc(fbSize);
    if(frameBuffer == NULL)
    {
        printf("*** LCD ERROR: cannot allocate framebuffer! ***");
        return;
    }

    /* Clear framebuffer, setting all pixels to 0xFFFF makes the screen white */
    memset(frameBuffer, 0xFF, fbSize);

    /*
     * Turn on DMA2 and configure its interrupt. DMA is used to transfer the
     * framebuffer content to the screen without using CPU.
     */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    NVIC_ClearPendingIRQ(DMA2_Stream7_IRQn);
    NVIC_SetPriority(DMA2_Stream7_IRQn, 14);
    NVIC_EnableIRQ(DMA2_Stream7_IRQn);

    /*
     * Turn on FSMC, used to efficiently manage the display data and control
     * lines.
     */
    RCC->AHB3ENR |= RCC_AHB3ENR_FSMCEN;

    /* Configure FSMC as LCD driver.
     * BCR1 config:
     * - CBURSTRW  = 0: asynchronous write operation
     * - ASYNCWAIT = 0: NWAIT not taken into account when running asynchronous protocol
     * - EXTMOD    = 0: do not take into account values of BWTR register
     * - WAITEN    = 0: nwait signal disabled
     * - WREN      = 1: write operations enabled
     * - WAITCFG   = 0: nwait active one data cycle before wait state
     * - WRAPMOD   = 0: direct wrapped burst disabled
     * - WAITPOL   = 0: nwait active low
     * - BURSTEN   = 0: burst mode disabled
     * - FACCEN    = 1: NOR flash memory disabled
     * - MWID      = 1: 16 bit external memory device
     * - MTYP      = 2: NOR
     * - MUXEN     = 0: addr/data not multiplexed
     * - MBNEN     = 1: enable bank
     */
    FSMC_Bank1->BTCR[0] = 0x10D9;

    /* BTR1 config:
     * - ACCMOD  = 0: access mode A
     * - DATLAT  = 0: don't care in asynchronous mode
     * - CLKDIV  = 1: don't care in asynchronous mode, 0000 is reserved
     * - BUSTURN = 0: time between two consecutive write accesses
     * - DATAST  = 5: we must have LCD twrl < DATAST*HCLK_period
     * - ADDHLD  = 1: used only in mode D, 0000 is reserved
     * - ADDSET  = 7: address setup time 7*HCLK_period
     */
    FSMC_Bank1->BTCR[1] = (0 << 28) /* ACCMOD */
                        | (0 << 24) /* DATLAT */
                        | (1 << 20) /* CLKDIV */
                        | (0 << 16) /* BUSTURN */
                        | (5 << 8)  /* DATAST */
                        | (1 << 4)  /* ADDHLD */
                        | 7;        /* ADDSET */

    /* Configure alternate function for data and control lines. */
    gpio_setMode(D0, ALTERNATE);
    gpio_setMode(D1, ALTERNATE);
    gpio_setMode(D2, ALTERNATE);
    gpio_setMode(D3, ALTERNATE);
    gpio_setMode(D4, ALTERNATE);
    gpio_setMode(D5, ALTERNATE);
    gpio_setMode(D6, ALTERNATE);
    gpio_setMode(D7, ALTERNATE);
    gpio_setMode(RS, ALTERNATE);
    gpio_setMode(WR, ALTERNATE);
    gpio_setMode(RD, ALTERNATE);

    gpio_setAlternateFunction(D0, 12);
    gpio_setAlternateFunction(D1, 12);
    gpio_setAlternateFunction(D2, 12);
    gpio_setAlternateFunction(D3, 12);
    gpio_setAlternateFunction(D4, 12);
    gpio_setAlternateFunction(D5, 12);
    gpio_setAlternateFunction(D6, 12);
    gpio_setAlternateFunction(D7, 12);
    gpio_setAlternateFunction(RS, 12);
    gpio_setAlternateFunction(WR, 12);
    gpio_setAlternateFunction(RD, 12);

    /* Reset and chip select lines as outputs */
    gpio_setMode(CS,  OUTPUT);
    gpio_setMode(RST, OUTPUT);

    gpio_setPin(CS);    /* CS idle state is high level */
    gpio_clearPin(RST); /* Put LCD in reset mode */

    delayMs(20);
    gpio_setPin(RST);   /* Exit from reset */

    gpio_clearPin(CS);

    /**
     * The following command sequence has been taken as-is from Tytera original
     * firmware. Without it, screen needs framebuffer data to be sent very slowly,
     * otherwise nothing will be rendered.
     * Since we do not have the datasheet for the controller employed in this
     * screen, we can only copy-and-paste...
     */
    writeCmd(0xfe);
    writeCmd(0xef);
    writeCmd(0xb4);
    writeData(0x00);
    writeCmd(0xff);
    writeData(0x16);
    writeCmd(0xfd);
    writeData(0x4f);
    writeCmd(0xa4);
    writeData(0x70);
    writeCmd(0xe7);
    writeData(0x94);
    writeData(0x88);
    writeCmd(0xea);
    writeData(0x3a);
    writeCmd(0xed);
    writeData(0x11);
    writeCmd(0xe4);
    writeData(0xc5);
    writeCmd(0xe2);
    writeData(0x80);
    writeCmd(0xa3);
    writeData(0x12);
    writeCmd(0xe3);
    writeData(0x07);
    writeCmd(0xe5);
    writeData(0x10);
    writeCmd(0xf0);
    writeData(0x00);
    writeCmd(0xf1);
    writeData(0x55);
    writeCmd(0xf2);
    writeData(0x05);
    writeCmd(0xf3);
    writeData(0x53);
    writeCmd(0xf4);
    writeData(0x00);
    writeCmd(0xf5);
    writeData(0x00);
    writeCmd(0xf7);
    writeData(0x27);
    writeCmd(0xf8);
    writeData(0x22);
    writeCmd(0xf9);
    writeData(0x77);
    writeCmd(0xfa);
    writeData(0x35);
    writeCmd(0xfb);
    writeData(0x00);
    writeCmd(0xfc);
    writeData(0x00);
    writeCmd(0xfe);
    writeCmd(0xef);
    writeCmd(0xe9);
    writeData(0x00);
    delayMs(20);

    /** The registers and commands below are the same in HX8353-E controller **/

    /*
     * Configuring screen's memory access control: TYT MD380 has the screen
     * rotated by 90° degrees, so we have to exgange row and coloumn indexing.
     * Moreover, we need to invert the vertical updating order to avoid painting
     * an image from bottom to top (that is, horizontally mirrored).
     * For reference see, in HX8353-E datasheet, MADCTL description at page 149
     * and paragraph 6.2.1, starting at page 48.
     *
     * Current confguration:
     * - MY  (bit 7): 0 -> do not invert y direction
     * - MX  (bit 6): 1 -> invert x direction
     * - MV  (bit 5): 1 -> exchange x and y
     * - ML  (bit 4): 0 -> refresh screen top-to-bottom
     * - BGR (bit 3): 0 -> RGB pixel format
     * - SS  (bit 2): 0 -> refresh screen left-to-right
     * - bit 1 and 0: don't care
     */
    writeCmd(CMD_MADCTL);
    writeData(0x60);

    writeCmd(CMD_CASET);
    writeData(0x00);
    writeData(0x00);
    writeData(0x00);
    writeData(0xA0);      /* 160 coloumns */
    writeCmd(CMD_RASET);
    writeData(0x00);
    writeData(0x00);
    writeData(0x00);
    writeData(0x80);      /* 128 rows */
    writeCmd(CMD_COLMOD);
    writeData(0x05);      /* 16 bit per pixel */
    delayMs(10);

    writeCmd(CMD_SLPOUT); /* Finally, turn on display */
    delayMs(120);
    writeCmd(CMD_DISPON);
    writeCmd(CMD_RAMWR);

    gpio_setPin(CS);
}

void display_terminate()
{
    /* Shut off FSMC and deallocate framebuffer */
    gpio_setMode(GPIOC, 6, OUTPUT);
    gpio_clearPin(GPIOC, 6);
    RCC->AHB3ENR &= ~RCC_AHB3ENR_FSMCEN;
    if(frameBuffer != NULL)
    {
        free(frameBuffer);
    }
}

uint16_t display_screenWidth()
{
    return SCREEN_WIDTH;
}

uint16_t display_screenHeight()
{
    return SCREEN_HEIGHT;
}

void display_renderRows(uint8_t startRow, uint8_t endRow)
{
    /*
     * Put screen data lines back to alternate function mode, since they are in
     * common with keyboard buttons and the keyboard driver sets them as inputs.
     * Little HACK: we bypass GPIO API and write directly into GPIO control
     * registers.
     */

    GPIOD->MODER &= ~0xF000000F;    /* Clear old values */
    GPIOE->MODER &= ~0x3FC000;
    GPIOD->MODER |= 0xA000000A;     /* Back to AF mode */
    GPIOE->MODER |= 0x2A8000;

    gpio_clearPin(CS);

    /*
     * First of all, convert pixels from little to big endian, for
     * compatibility with the display driver. We do this after having brought
     * the CS pin low, in this way user code calling the renderingInProgress
     * function gets true as return value and does not stomp our work.
     */
    for(uint8_t y = startRow; y < endRow; y++)
    {
        for(uint8_t x = 0; x < SCREEN_WIDTH; x++)
        {
            size_t pos = x + y * SCREEN_WIDTH;
            uint16_t pixel = frameBuffer[pos];
            frameBuffer[pos] = __builtin_bswap16(pixel);
        }
    }

    /* Configure start and end rows in display driver */
    writeCmd(CMD_RASET);
    writeData(0x00);
    writeData(startRow);
    writeData(0x00);
    writeData(endRow);

    /* Now, write to memory */
    writeCmd(CMD_RAMWR);

    /*
     * Configure DMA2 stream 7 to send framebuffer data to the screen.
     * Both source and destination memory sizes are configured to 8 bit, thus
     * we have to set the transfer size to twice the framebuffer size, since
     * this one is made of 16 bit variables.
     */
    DMA2_Stream7->NDTR = (endRow - startRow) * SCREEN_WIDTH * sizeof(uint16_t);
    DMA2_Stream7->PAR  = ((uint32_t ) frameBuffer + (startRow * SCREEN_WIDTH 
                                                     * sizeof(uint16_t)));
    DMA2_Stream7->M0AR = LCD_FSMC_ADDR_DATA;
    DMA2_Stream7->CR = DMA_SxCR_CHSEL         /* Channel 7                   */
                     | DMA_SxCR_PL_0          /* Medium priority             */
                     | DMA_SxCR_PINC          /* Increment source pointer    */
                     | DMA_SxCR_DIR_1         /* Memory to memory            */
                     | DMA_SxCR_TCIE          /* Transfer complete interrupt */
                     | DMA_SxCR_TEIE          /* Transfer error interrupt    */
                     | DMA_SxCR_EN;           /* Start transfer              */
}

void display_render()
{
    display_renderRows(0, SCREEN_HEIGHT);
}

bool display_renderingInProgress()
{
    /*
     * Render is in progress if PD6 is low. Its value can be tested reading
     * GPIOD->ODR.
     */
    uint16_t pinValue = (GPIOD->ODR & (1 << 6));
    return (pinValue == 0) ? 1 : 0;
}

void *display_getFrameBuffer()
{
    return (void *)(frameBuffer);
}

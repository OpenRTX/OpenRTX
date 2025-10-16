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
#include <miosix.h>
#include <kernel/scheduler/scheduler.h>

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


using namespace miosix;
static Thread *lcdWaiting = 0;

void __attribute__((used)) DmaImpl()
{
    DMA2->HIFCR |= DMA_HIFCR_CTCIF7 | DMA_HIFCR_CTEIF7;    /* Clear flags */
    gpio_setPin(LCD_CS);

    if(lcdWaiting == 0) return;
    lcdWaiting->IRQwakeup();
    if(lcdWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    lcdWaiting = 0;
}

void __attribute__((naked)) DMA2_Stream7_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z7DmaImplv");
    restoreContext();
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
    /* Initialise backlight driver */
    backlight_init();

    /*
     * Turn on DMA2 and configure its interrupt. DMA is used to transfer the
     * framebuffer content to the screen without using CPU.
     */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    __DSB();

    NVIC_ClearPendingIRQ(DMA2_Stream7_IRQn);
    NVIC_SetPriority(DMA2_Stream7_IRQn, 14);
    NVIC_EnableIRQ(DMA2_Stream7_IRQn);

    /*
     * Turn on FSMC, used to efficiently manage the display data and control
     * lines.
     */
    RCC->AHB3ENR |= RCC_AHB3ENR_FSMCEN;
    __DSB();

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
    gpio_setMode(LCD_D0, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_D1, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_D2, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_D3, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_D4, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_D5, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_D6, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_D7, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_RS, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_WR, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_RD, ALTERNATE | ALTERNATE_FUNC(12));

    /* Reset and chip select lines as outputs */
    gpio_setMode(LCD_CS,  OUTPUT);
    gpio_setMode(LCD_RST, OUTPUT);

    gpio_setPin(LCD_CS);    /* CS idle state is high level */
    gpio_clearPin(LCD_RST); /* Put LCD in reset mode */

    delayMs(20);
    gpio_setPin(LCD_RST);   /* Exit from reset */

    gpio_clearPin(LCD_CS);

    /**
     * The command sequence below has been taken as-is from the LCD initialisation
     * routine at address 0x0804d1c8 of Tytera's firmware for MD-UV380 radios,
     * version S18.016.
     * It has also been cross-checked with the firmware image for MD-380/390 to
     * ensure that is compatible also with the displays of that radios.
     * Without this sequence, screen needs framebuffer data to be sent very slowly,
     * otherwise nothing will be rendered.
     * Since we do not have the datasheet for the controller employed in this
     * screen, we can only do copy-and-paste...
     */
    uint8_t lcd_type = platform_getHwInfo()->hw_version;

    if((lcd_type == 2) || (lcd_type == 3))
    {
        writeCmd(0xfe);
        writeCmd(0xef);
        writeCmd(0xb4);
        writeData(0x00);
        writeCmd(0xff);
        writeData(0x16);
        writeCmd(0xfd);

        if(lcd_type == 3)
            writeData(0x40);
        else
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
    }
    else
    {
        writeCmd(0x11);
        delayMs(120);
        writeCmd(0xb1);
        writeData(0x05);
        writeData(0x3c);
        writeData(0x3c);
        writeCmd(0xb2);
        writeData(0x05);
        writeData(0x3c);
        writeData(0x3c);
        writeCmd(0xb3);
        writeData(0x05);
        writeData(0x3c);
        writeData(0x3c);
        writeData(0x05);
        writeData(0x3c);
        writeData(0x3c);
        writeCmd(0xb4);
        writeData(0x03);
        writeCmd(0xc0);
        writeData(0x28);
        writeData(0x08);
        writeData(0x04);
        writeCmd(0xc1);
        writeData(0xc0);
        writeCmd(0xc2);
        writeData(0xd);
        writeData(0x00);
        writeCmd(0xc3);
        writeData(0x8d);
        writeData(0x2a);
        writeCmd(0xc4);
        writeData(0x8d);
        writeData(0xee);
        writeCmd(0xc5);
        writeData(0x1a);
        writeCmd(0x36);
        writeData(0x08);
        writeCmd(0xe0);
        writeData(0x04);
        writeData(0x0c);
        writeData(0x07);
        writeData(0x0a);
        writeData(0x2e);
        writeData(0x30);
        writeData(0x25);
        writeData(0x2a);
        writeData(0x28);
        writeData(0x26);
        writeData(0x2e);
        writeData(0x3a);
        writeData(0x00);
        writeData(0x01);
        writeData(0x03);
        writeData(0x13);
        writeCmd(0xe1);
        writeData(0x04);
        writeData(0x16);
        writeData(0x06);
        writeData(0x0d);
        writeData(0x2d);
        writeData(0x26);
        writeData(0x23);
        writeData(0x27);
        writeData(0x27);
        writeData(0x25);
        writeData(0x2d);
        writeData(0x3b);
        writeData(0x00);
        writeData(0x01);
        writeData(0x04);
        writeData(0x13);
    }

    /*
     * Configuring screen's memory access control: TYT MD3x0 radios have the screen
     * rotated by 90° degrees, so we have to exchange row and coloumn indexing.
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
    if(lcd_type == 1)
    {
        writeData(0x60);    /* Reference case: MD-390(G)  */
    }
    else if(lcd_type == 2)
    {
        writeData(0xE0);    /* Reference case: MD-380V(G) */
    }
    else
    {
        writeData(0xA0);    /* Reference case: MD-380     */
    }

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

    gpio_setPin(LCD_CS);
}

void display_terminate()
{
    /* Shut down backlight */
    backlight_terminate();

    /* Shut off FSMC and deallocate framebuffer */
    RCC->AHB3ENR &= ~RCC_AHB3ENR_FSMCEN;
    __DSB();
}

void display_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    /*
     * Put screen data lines back to alternate function mode, since they are in
     * common with keyboard buttons and the keyboard driver sets them as inputs.
     */
    gpio_setMode(LCD_D0, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_D1, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_D2, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_D3, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_D4, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_D5, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_D6, ALTERNATE | ALTERNATE_FUNC(12));
    gpio_setMode(LCD_D7, ALTERNATE | ALTERNATE_FUNC(12));

    gpio_clearPin(LCD_CS);

    /*
     * First of all, convert pixels from little to big endian, for
     * compatibility with the display driver. We do this after having brought
     * the CS pin low, in this way user code calling the renderingInProgress
     * function gets true as return value and does not stomp our work.
     */
    uint16_t *frameBuffer = (uint16_t *) fb;
    for(uint8_t y = startRow; y < endRow; y++)
    {
        for(uint8_t x = 0; x < CONFIG_SCREEN_WIDTH; x++)
        {
            size_t pos = x + y * CONFIG_SCREEN_WIDTH;
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
    DMA2_Stream7->NDTR = (endRow - startRow) * CONFIG_SCREEN_WIDTH * sizeof(uint16_t);
    DMA2_Stream7->PAR  = ((uint32_t ) frameBuffer + (startRow * CONFIG_SCREEN_WIDTH
                                                     * sizeof(uint16_t)));
    DMA2_Stream7->M0AR = LCD_FSMC_ADDR_DATA;
    DMA2_Stream7->CR = DMA_SxCR_CHSEL         /* Channel 7                   */
                     | DMA_SxCR_PINC          /* Increment source pointer    */
                     | DMA_SxCR_DIR_1         /* Memory to memory            */
                     | DMA_SxCR_TCIE          /* Transfer complete interrupt */
                     | DMA_SxCR_TEIE          /* Transfer error interrupt    */
                     | DMA_SxCR_EN;           /* Start transfer              */

    /*
     * Put the calling thread in waiting status until render completes.
     */
    {
        FastInterruptDisableLock dLock;
        lcdWaiting = Thread::IRQgetCurrentThread();
        do
        {
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        } while(lcdWaiting);
    }
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

/*
 * Function implemented in backlight_MDx driver
 *
 * void display_setBacklightLevel(uint8_t level)
 */

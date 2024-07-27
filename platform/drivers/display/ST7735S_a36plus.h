// Written by Jamiexu

#ifndef __ST7735S_JAMIEXU_H_
#define __ST7735S_JAMIEXU_H_
#include "string.h"

// Written by Jamiexu

// LCD
#define LCD_GPIO_PORT GPIOB
#define LCD_GPIO_CS_PIN GPIO_PIN_2
#define LCD_GPIO_SDA_PIN GPIO_PIN_15
#define LCD_GPIO_RST_PIN GPIO_PIN_12
#define LCD_GPIO_WR_PIN GPIO_PIN_10
#define LCD_GPIO_SCK_PIN GPIO_PIN_13
#define LCD_GPIO_LIGHT_PIN GPIO_PIN_7
#define LCD_GPIO_RCU RCU_GPIOB


#define LCD_CS_LOW gpio_clearPin(LCD_GPIO_PORT, LCD_GPIO_CS_PIN)
#define LCD_CS_HIGH gpio_setPin(LCD_GPIO_PORT, LCD_GPIO_CS_PIN)

#define LCD_RST_LOW gpio_clearPin(LCD_GPIO_PORT, LCD_GPIO_RST_PIN)
#define LCD_RST_HIGH gpio_setPin(LCD_GPIO_PORT, LCD_GPIO_RST_PIN)

#define LCD_LIGHT_LOW gpio_clearPin(LCD_GPIO_PORT, LCD_GPIO_LIGHT_PIN)
#define LCD_LIGHT_HIGH gpio_setPin(LCD_GPIO_PORT, LCD_GPIO_LIGHT_PIN)

#define LCD_DC_LOW gpio_clearPin(LCD_GPIO_PORT, LCD_GPIO_WR_PIN)
#define LCD_DC_HIGH gpio_setPin(LCD_GPIO_PORT, LCD_GPIO_WR_PIN)

#define LCD_SDA_LOW gpio_clearPin(LCD_GPIO_PORT, LCD_GPIO_SDA_PIN)
#define LCD_SDA_HIGH gpio_setPin(LCD_GPIO_PORT, LCD_GPIO_SDA_PIN)

#define LCD_SCK_LOW gpio_clearPin(LCD_GPIO_PORT, LCD_GPIO_SCK_PIN)
#define LCD_SCK_HIGH gpio_setPin(LCD_GPIO_PORT, LCD_GPIO_SCK_PIN)

typedef enum
{
    ST7735S_CMD_NOP = 0x00,       // NOP
    ST7735S_CMD_RESET = 0x01,     // Software reset
    ST7735S_CMD_RDDID = 0x04,     // Read Display ID
    ST7735S_CMD_RDDST = 0x09,     // Read Dis Status
    ST7735S_CMD_RDDPM = 0x0A,     // Rd Dis power
    ST7735S_CMD_RDDMADCTL = 0x0B, // Rd Dis MADCTL
    ST7735S_CMD_RDDCOLMOD = 0x0C, // Rd Dis Pix Format
    ST7735S_CMD_RDDIM = 0x0D,     // Rd Dis Img Mode
    ST7735S_CMD_RDDSM = 0x0E,     //
    ST7735S_CMD_RDDSDR = 0x0F,    //
    ST7735S_CMD_SLPIN = 0x10,     //
    ST7735S_CMD_SLPOUT = 0x11,    //
    ST7735S_CMD_PTLON = 0x12,     //
    ST7735S_CMD_NORON = 0x13,     //
    ST7735S_CMD_INVOFF = 0x20,    //
    ST7735S_CMD_INVON = 0x21,     //
    ST7735S_CMD_GAMSET = 0x26,    //
    ST7735S_CMD_DISPOFF = 0x28,   //
    ST7735S_CMD_DISPON = 0x29,    //
    ST7735S_CMD_CASET = 0x2A,     //
    ST7735S_CMD_RASET = 0x2B,     //
    ST7735S_CMD_RAMWR = 0x2C,     //
    ST7735S_CMD_RGBSET = 0x2D,    //
    ST7735S_CMD_RAMRD = 0x2E,     //
    ST7735S_CMD_PTLAR = 0x30,     //
    ST7735S_CMD_SCRLAR = 0x33,    //
    ST7735S_CMD_TEOFF = 0x34,     //
    ST7735S_CMD_TEON = 0x35,      //
    ST7735S_CMD_MADCTL = 0x36,    //
    ST7735S_CMD_VSCSAD = 0x37,    //
    ST7735S_CMD_IDMOFF = 0x38,    //
    ST7735S_CMD_IDMON = 0x39,     //
    ST7735S_CMD_COLMOD = 0x3A,    //
    ST7735S_CMD_FRMCTR1 = 0xB1,   //
    ST7735S_CMD_FRMCTR2 = 0xB2,   //
    ST7735S_CMD_FRMCTR3 = 0xB3,   //
    ST7735S_CMD_INVCTR = 0xB4,    //
    ST7735S_CMD_PWCTR1 = 0xC0,    //
    ST7735S_CMD_PWCTR2 = 0xC1,    //
    ST7735S_CMD_PWCTR3 = 0xC2,    //
    ST7735S_CMD_PWCTR4 = 0xC3,    //
    ST7735S_CMD_PWCTR5 = 0xC4,    //
    ST7735S_CMD_VMCTR1 = 0xC5,    //
    ST7735S_CMD_VMOFCTR = 0xC7,   //
    ST7735S_CMD_WRID2 = 0xD1,     //
    ST7735S_CMD_WRID3 = 0xD2,     //
    ST7735S_CMD_NVFCTR1 = 0xD9,   //
    ST7735S_CMD_RDID1 = 0xDA,     //
    ST7735S_CMD_RDID2 = 0xDB,     //
    ST7735S_CMD_RDID3 = 0xDC,     //
    ST7735S_CMD_NVFCTR2 = 0xDE,   //
    ST7735S_CMD_NVFCTR3 = 0xDF,   //
    ST7735S_CMD_GMCTRP1 = 0xE0,   //
    ST7735S_CMD_GMCTRN1 = 0xE1,   //
    ST7735S_CMD_GCV = 0xFC        //
} st7735s_cmd_t;

// define piexel rgb depth format
typedef struct
{
    uint8_t r : 4;
    uint8_t g : 4;
    uint8_t b : 4;
} _color_rgb444;

typedef struct
{
    uint8_t r : 5;
    uint8_t g : 6;
    uint8_t b : 5;
} _color_rgb565;

typedef struct
{
    uint8_t r : 6;
    uint8_t reserved1 : 2;
    uint8_t g : 6;
    uint8_t reserved2 : 2;
    uint8_t b : 6;
    uint8_t reserved3 : 2;
} _color_rgb666;

typedef enum
{
    COLOR_FORMAT_RGB444 = 0x03,
    COLOR_FORMAT_RGB565 = 0x05,
    COLOR_FORMAT_RGB666 = 0x06,
    COLOR_FORMAT_NO_USED = 0x07
} _color_format;

typedef struct
{
    uint16_t xs;
    uint16_t xe;
    uint16_t ys;
    uint16_t ye;
} st7735s_window_t;

static void spi_send_bytes(uint8_t len, uint8_t *data); // spi send data
static void spi_send_byte(uint8_t data);                // spi send one byte data
static void spi_send_bit(uint8_t data);                 // spi send one bit data
static void st7735s_send_command(st7735s_cmd_t cmd);    // st7735s send command
static void st7735s_send_data(uint8_t data);            // st7735s data
static void st7735s_delay(uint32_t count);            // st7735s data

void st7735s_init(void);    
void st7735s_test(void);                                     // st7735s init
void st7735s_set_color(uint8_t red, uint8_t green, uint8_t blue); // st7735s set color
void st7735s_set_color_hex(uint32_t color); // st7735s set color by hex
void st7735s_set_pixel_format(_color_format x);                    // st7735s set pixel format
void st7735s_draw_pixel(uint8_t x, uint8_t y);
void st7735s_fill_react(uint16_t x, uint16_t y, uint16_t width, uint16_t height); // st7735s fill react
void st7735s_flush(void);
void display_setWindow(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2);
void display_clearWindow(uint16_t x1, uint16_t x2, uint16_t width, uint16_t height);

static void st7735s_reset_window(void);                                             // st7735s reset window
static void st7735s_update_window(uint16_t x, uint16_t y);                          // st7735s update window
static void st7735s_set_window(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2); // st7735s set window
static void st7735s_send_color(void);

#endif
#ifndef __GPIO_JAMIEXU_H__
#define __GPIO_JAMIEXU_H__
#include "platform/mcu/CMSIS/Device/GD/GD32F3x0/Include/gd32f3x0.h"

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


// FLASH
#define FLASH_GPIO_PORT GPIOA
#define FLASH_GPIO_CS_PIN GPIO_PIN_4
#define FLASH_GPIO_SCK_PIN GPIO_PIN_5
#define FLASH_GPIO_DIN_PIN GPIO_PIN_7
#define FLASH_GPIO_DOUT_PIN GPIO_PIN_6
#define FLASH_GPIO_RCU RCU_GPIOA


// BATTERY
#define BATTERY_GPIO_PORT GPIOA
#define BATTERY_GPIO_PIN GPIO_PIN_1
#define BATTERY_GPIO_RCU RCU_GPIOA


// BK4819
#define BK4819_GPIO_PORT GPIOA
#define BK4819_GPIO_SCK_PIN GPIO_PIN_2
#define BK4819_GPIO_SDA_PIN GPIO_PIN_3
#define BK4819_GPIO_SCN_PORT GPIOC
#define BK4819_GPIO_SCN_PIN GPIO_PIN_13
#define BK4819_GPIO_RCU RCU_GPIOA
#define BK4819_GPIO_SCN_RCU RCU_GPIOC


// BK1080
#define BK1080_GPIO_PORT GPIOF
#define BK1080_GPIO_SCK_PIN GPIO_PIN_6
#define BK1080_GPIO_SDA_PORT GPIOA
#define BK1080_GPIO_SDA_PIN GPIO_PIN_3
#define BK1080_GPIO_RCU RCU_GPIOF
#define BK1080_GPIO_SDA_RCU RCU_GPIOA


// KEY
#define KEY_GPIO_PTT_PORT GPIOA
#define KEY_GPIO_PTT_PIN GPIO_PIN_10
#define KEY_GPIO_PTT_RCU RCU_GPIOA

#define KEY_GPIO_PORT GPIOB
#define KEY_GPIO_RCU RCU_GPIOB
#define KEY_GPIO_ROW0_PIN GPIO_PIN_3
#define KEY_GPIO_ROW1_PIN GPIO_PIN_4
#define KEY_GPIO_ROW2_PIN GPIO_PIN_5
#define KEY_GPIO_ROW3_PIN GPIO_PIN_6

#define KEY_GPIO_COL0_PIN GPIO_PIN_11
#define KEY_GPIO_COL1_PIN GPIO_PIN_9
#define KEY_GPIO_COL2_PIN GPIO_PIN_14
#define KEY_GPIO_COL3_PIN GPIO_PIN_8


// USART
#define USART_GPIO_PORT GPIOA
#define USART_GPIO_TX_PIN GPIO_PIN_9
#define USART_GPIO_RX_PIN GPIO_PIN_10
#define USART_GPIO_RCU RCU_GPIOA


// MIC_EN
#define MIC_EN_GPIO_PORT GPIOA
#define MIC_EN_GPIO_PIN GPIO_PIN_12
#define MIC_EN_GPIO_RCU RCU_GPIOA

void gpio_config(void);

// void gpio_setMode(void *port, uint8_t pin, uint8_t mode);
// void gpio_setPin(void *port, uint8_t pin);
// void gpio_clearPin(void *port, uint8_t pin);
// void gpio_togglePin(void *port, uint8_t pin);
// uint8_t gpio_getPin(void *port, uint8_t pin);
// void gpio_af_set(void *port, uint8_t af, uint16_t pin);
// void gpio_mode_set(void *port, uint8_t mode, uint8_t pupd, uint16_t pin);
// void gpio_output_options_set(void *port, uint8_t otype, uint8_t ospeed, uint16_t pin);

#endif
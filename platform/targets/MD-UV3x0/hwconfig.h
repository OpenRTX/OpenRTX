/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Silvano Seva IU2KWO                             *
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

#ifndef HWCONFIG_H
#define HWCONFIG_H

#include <stm32f4xx.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Device has a working real time clock */
#define HAS_RTC

/* Device supports an optional GPS chip */
#define HAS_GPS

/* Screen dimensions */
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 128

/* Screen pixel format */
#define PIX_FMT_RGB565

/* Battery type */
#define BAT_LIPO_2S

/* Power keep switch */
#define PWR_SW GPIOA,7

/* Display */
#define LCD_D0  GPIOD,14
#define LCD_D1  GPIOD,15
#define LCD_D2  GPIOD,0
#define LCD_D3  GPIOD,1
#define LCD_D4  GPIOE,7
#define LCD_D5  GPIOE,8
#define LCD_D6  GPIOE,9
#define LCD_D7  GPIOE,10
#define LCD_WR  GPIOD,5
#define LCD_RD  GPIOD,4
#define LCD_CS  GPIOD,6
#define LCD_RS  GPIOD,12
#define LCD_RST GPIOD,13
#define LCD_BKLIGHT GPIOD,8

/* Signalling LEDs */
#define GREEN_LED  GPIOE,0
#define RED_LED    GPIOE,1

/* Analog inputs */
#define AIN_VOLUME GPIOA,0
#define AIN_VBAT   GPIOA,1
#define AIN_MIC    GPIOA,3

/* Channel selection rotary encoder */
#define CH_SELECTOR_0 GPIOE,14
#define CH_SELECTOR_1 GPIOB,11

/* Push-to-talk switch */
#define PTT_SW  GPIOE,11
#define PTT_EXT GPIOE,12

/*
 * Keyboard. Here we define only rows, since coloumn lines are the same as
 * LCD_Dx. See also: https://www.qsl.net/dl4yhf/RT3/md380_hw.html#keyboard
 *
 * "Monitor" and "Function" buttons, on the other hand, are connected to
 * keyboard row 3 and on LCD_D6 and LCD_D7. They are SWAPPED with respect to
 * connections made on MD-380.
 */
#define KB_ROW1 GPIOA,6 /* K1 */
#define KB_ROW2 GPIOD,2 /* K2 */
#define KB_ROW3 GPIOD,3 /* K3 */
#define MONI_SW LCD_D6
#define FUNC_SW LCD_D7

/* Tone generator  */
#define CTCSS_OUT GPIOC,7   /* System "beep" */
#define BEEP_OUT  GPIOC,8   /* CTCSS tone    */

/* External flash */
#define FLASH_CS  GPIOD,7
#define FLASH_CLK GPIOB,3
#define FLASH_SDO GPIOB,4
#define FLASH_SDI GPIOB,5

/* Audio control */
#define AUDIO_AMP_EN GPIOB,9
#define SPK_MUTE     GPIOB,8
#define MIC_PWR      GPIOA,13
#define RX_AUDIO_MUX GPIOD,9

/* GPS, for the devices who have it */
#define GPS_EN   GPIOA,9
#define GPS_DATA GPIOA,10

/* RTX stage control */
#define VHF_LNA_EN GPIOA,5
#define UHF_LNA_EN GPIOA,2
#define PA_EN_1    GPIOC,5
#define PA_EN_2    GPIOC,4
#define PA_SEL_SW  GPIOC,6
#define APC_REF    GPIOA,4

/* I2C for AT1846S */
#define I2C_SDA GPIOC,9
#define I2C_SCL GPIOA,8

/* HR_C6000 control interface */
#define DMR_SLEEP GPIOE,6
#define DMR_CS    GPIOE,2
#define DMR_CLK   GPIOE,3
#define DMR_MOSI  GPIOE,4
#define DMR_MISO  GPIOE,5


/*
 * To enable pwm for display backlight dimming uncomment this directive.
 *
 * WARNING: backlight pwm is disabled by default because it generates a
 * continuous tone in the speaker and headphones.
 *
 * This issue cannot be solved in any way because it derives from how the
 * MD-UV380 mcu pins are used: to have a noiseless backlight pwm, the control
 * pin has to be connected to a mcu pin having between its alternate functions
 * an output compare channel of one of the timers. With this configuration, the
 * pwm signal can completely generated in hardware and its frequency can be well
 * above 22kHz, which is the upper limit for human ears.
 *
 * In the MD-UV380 radio, display backlight is connected to PD8, which is not
 * connected to any of the available output compare channels. Thus, the pwm
 * signal generation is managed inside the TIM11 ISR by toggling the backlight
 * pin and its frequency has to be low (~250Hz) to not put too much overehad on
 * the processor due to timer ISR triggering at an high rate.
 *
 * #define ENABLE_BKLIGHT_DIMMING
 */

#ifdef __cplusplus
}
#endif

#endif /* HWCONFIG_H */

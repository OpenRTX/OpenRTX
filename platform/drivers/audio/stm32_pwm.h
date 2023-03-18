/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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

#ifndef STM32_PWM_H
#define STM32_PWM_H

#include <stdbool.h>
#include <stdint.h>
#include <stm32f4xx.h>
#include <interfaces/audio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Driver STM32 PWM used as audio output stream device.
 * Input data format is signed 16-bit, internally converted to unsigned 8-bit
 * values for compatibility with the hardware.
 *
 * The driver uses the following peripherals: DMA1_Stream2, TIM7
 */

/**
 * Data structure holding the configuration for a given DAC channel.
 */
struct PwmChannelCfg
{
    volatile uint32_t *pwmReg;  ///< Address of PWM duty cycle register.
    void (*startCbk)(void);     ///< Callback function for additional setup operations.
    void (*stopCbk)(void);      ///< Callback function for additional end operations.
};


extern const struct audioDriver stm32_pwm_audio_driver;


/**
 * Initialize the driver and the peripherals.
 */
void stm32pwm_init();

/**
 * Shutdown the driver and the peripherals.
 */
void stm32pwm_terminate();

#ifdef __cplusplus
}
#endif

#endif /* STM32_PWM_H */

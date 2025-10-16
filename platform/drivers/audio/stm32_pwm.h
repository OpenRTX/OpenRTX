/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef STM32_PWM_H
#define STM32_PWM_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "interfaces/audio.h"

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

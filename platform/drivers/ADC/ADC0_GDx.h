/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef ADC0_H
#define ADC0_H

#include <stdint.h>

/**
 * Initialise and start ADC0.
 */
void adc0_init();

/**
 * Turn off ADC0.
 */
void adc0_terminate();

/**
 * Get current measurement of a given channel returning the raw ADC value.
 * @param ch: channel number.
 * @return current value of the specified channel, in ADC counts.
 */
uint16_t adc0_getRawSample(uint8_t ch);

/**
 * Get current measurement of a given channel.
 * @param ch: channel number.
 * @return current value of the specified channel in mV.
 */
uint16_t adc0_getMeasurement(uint8_t ch);

#endif /* ADC0_H */

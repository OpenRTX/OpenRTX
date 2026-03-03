/***************************************************************************
 *   Copyright (C) 2026 by OpenRTX                                         *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>  *
 ***************************************************************************/

#ifndef ADC_ULP_SAMPLER_H
#define ADC_ULP_SAMPLER_H

#include <stdint.h>
#include <stddef.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ULP ADC Sampler - Main CPU Interface
 * 
 * This API provides continuous ADC2 channel 4 sampling using the ESP32-S3
 * ULP RISC-V coprocessor for M17 protocol microphone input.
 * 
 * The ULP coprocessor samples ADC2 CH4 (GPIO15) at 8 kHz and stores samples
 * in a ring buffer in RTC slow memory. The main CPU can read samples
 * asynchronously without blocking or wasting CPU cycles on polling.
 * 
 * Typical usage:
 *   1. ulp_adc_sampler_init() - Initialize subsystem
 *   2. ulp_adc_sampler_start(8000) - Start sampling at 8 kHz
 *   3. ulp_adc_read_samples(buffer, count) - Read available samples
 *   4. ulp_adc_sampler_stop() - Stop sampling when done
 */

/**
 * Initialize ULP ADC sampler subsystem
 * 
 * This function:
 * - Initializes ADC2 for ULP use
 * - Loads the ULP RISC-V program into RTC memory
 * - Configures but does NOT start sampling (call ulp_adc_sampler_start)
 * 
 * Must be called once during platform initialization before using other APIs.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ulp_adc_sampler_init(void);

/**
 * Start continuous ADC sampling at specified rate
 * 
 * Configures the ULP timer for the desired sample rate and enables sampling.
 * The ULP will wake up periodically and store samples in the ring buffer.
 * 
 * @param sample_rate_hz Sampling frequency in Hz (typical: 8000 for M17)
 *                       Supported range: 100 Hz to 50 kHz
 * 
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if rate is out of range
 */
esp_err_t ulp_adc_sampler_start(uint32_t sample_rate_hz);

/**
 * Stop continuous ADC sampling
 * 
 * Disables sampling by clearing the enabled flag. The ULP will continue
 * to wake up but will not take samples. Call ulp_adc_sampler_start()
 * to resume sampling.
 */
void ulp_adc_sampler_stop(void);

/**
 * Read available samples from ring buffer
 * 
 * Non-blocking read of samples from the ULP ring buffer. Reads up to
 * max_count samples or all available samples, whichever is less.
 * 
 * @param dest Destination buffer for samples (must be at least max_count × 2 bytes)
 * @param max_count Maximum number of samples to read
 * 
 * @return Number of samples actually read (0 to max_count)
 */
size_t ulp_adc_read_samples(int16_t *dest, size_t max_count);

/**
 * Get number of samples available in ring buffer
 * 
 * Returns the number of samples currently available for reading.
 * This can be used to check buffer fill level before reading.
 * 
 * @return Number of samples available (0 to 511)
 */
size_t ulp_adc_get_available_count(void);

/**
 * Get overflow counter value
 * 
 * Returns the number of times the ring buffer has overflowed (filled up
 * completely). Overflows indicate the main CPU is not reading samples
 * fast enough to keep up with the sample rate.
 * 
 * @return Overflow count since last reset
 */
uint32_t ulp_adc_get_overflow_count(void);

/**
 * Reset overflow counter
 * 
 * Resets the overflow counter to zero. Useful for testing and diagnostics.
 */
void ulp_adc_reset_overflow_count(void);

/**
 * Get ULP wakeup counter (debug)
 */
uint32_t ulp_adc_get_wakeup_counter(void);

/**
 * Get ULP sample counter (debug)
 */
uint32_t ulp_adc_get_sample_counter(void);

/**
 * Get ULP write pointer (debug)
 */
uint32_t ulp_adc_get_write_ptr(void);

/**
 * Get ULP read pointer (debug)
 */
uint32_t ulp_adc_get_read_ptr(void);

/**
 * Get ULP missed samples counter (debug)
 */
uint32_t ulp_adc_get_missed_samples(void);

#ifdef __cplusplus
}
#endif

#endif // ADC_ULP_SAMPLER_H

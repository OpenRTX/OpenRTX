#ifndef ADC_TIMER_ISR_H
#define ADC_TIMER_ISR_H

#include <stdint.h>
#include <stddef.h>

/**
 * Initialize ADC2 timer-based sampling
 * @param sample_rate_hz Sampling frequency in Hz (e.g., 8000 for 8 kHz)
 * @return 0 on success, negative on error
 */
int adc_timer_init(uint32_t sample_rate_hz);

/**
 * Start ADC2 timer sampling
 * @return 0 on success, negative on error
 */
int adc_timer_start(void);

/**
 * Stop ADC2 timer sampling and print statistics
 * @return 0 on success
 */
int adc_timer_stop(void);

/**
 * Deinitialize ADC2 timer
 */
void adc_timer_deinit(void);

/**
 * Read samples from ring buffer
 * @param dest Destination buffer for samples
 * @param max_count Maximum number of samples to read
 * @return Number of samples actually read
 */
size_t adc_timer_read_samples(int16_t *dest, size_t max_count);

/**
 * Get number of available samples in ring buffer
 * @return Number of samples available
 */
size_t adc_timer_get_available_count(void);

/**
 * Get total sample counter
 * @return Total samples captured
 */
uint32_t adc_timer_get_sample_counter(void);

/**
 * Get overflow counter
 * @return Number of buffer overflows
 */
uint32_t adc_timer_get_overflow_count(void);

/**
 * Reset overflow counter
 */
void adc_timer_reset_overflow_count(void);

/**
 * Get ISR call counter (debug)
 * @return Number of times ISR was called
 */
uint32_t adc_timer_get_isr_call_count(void);

/**
 * Get conversion timeout counter (debug)
 * @return Number of conversion timeouts
 */
uint32_t adc_timer_get_timeout_count(void);

#endif // ADC_TIMER_ISR_H

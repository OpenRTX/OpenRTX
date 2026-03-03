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

#ifndef ULP_ADC_SHARED_H
#define ULP_ADC_SHARED_H

#include <stdint.h>

/**
 * ULP ADC Sampler - Shared Memory Definitions
 * 
 * This header defines the shared data structures placed in RTC slow memory
 * that are accessible by both the ULP RISC-V coprocessor and the main CPU.
 * 
 * The ring buffer provides 512 samples (64ms at 8kHz) of buffering for
 * continuous ADC2 channel 4 sampling. This matches M17 protocol requirements
 * for 8kHz microphone input.
 */

// Ring buffer size - must be power of 2 for efficient modulo via bitwise AND
#define ULP_ADC_BUFFER_SIZE 512

// ADC configuration
#define ULP_ADC_UNIT        2       // ADC2
#define ULP_ADC_CHANNEL     4       // GPIO15 on ESP32-S3
#define ULP_ADC_ATTEN       3       // 11dB attenuation (0-3.3V range)
#define ULP_ADC_WIDTH       12      // 12-bit resolution (0-4095)

/**
 * Shared ring buffer structure in RTC slow memory
 * 
 * Memory layout:
 * - buffer: 512 samples × 2 bytes = 1024 bytes
 * - pointers and counters: 20 bytes
 * Total: ~1KB (well within ESP32-S3's 16KB RTC slow memory)
 * 
 * Synchronization:
 * - write_ptr: updated by ULP only
 * - read_ptr: updated by main CPU only
 * - No locking needed due to single-writer/single-reader pattern
 * - Use volatile to prevent compiler optimizations
 */

// Ring buffer for ADC samples (0-4095 for 12-bit ADC)
extern volatile uint16_t ulp_adc_buffer[ULP_ADC_BUFFER_SIZE];

// Write pointer (updated by ULP coprocessor)
extern volatile uint32_t ulp_write_ptr;

// Read pointer (updated by main CPU)
extern volatile uint32_t ulp_read_ptr;

// Total samples collected (for diagnostics and overflow detection)
extern volatile uint32_t ulp_sample_counter;

// Overflow counter (increments when buffer is full)
extern volatile uint32_t ulp_overflow_count;

// Enable flag (main CPU sets to 1 to start sampling, 0 to stop)
extern volatile uint32_t ulp_enabled;

#endif // ULP_ADC_SHARED_H

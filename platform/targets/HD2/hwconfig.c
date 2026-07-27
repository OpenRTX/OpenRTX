/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Definitions of the const driver-config structs + hardware descriptor
 * declared in hwconfig.h.
 */

#include "hwconfig.h"
#include "interfaces/platform.h" /* hwInfo_t */
#include "drivers/ADC/adcHrc7000.h"
#include "drivers/i2c_hrc7000.h"
#include <pthread.h>

/* On-chip ADC: battery on ADC_VBAT_CH via a 1:3 divider; 3.3 V reference, 10-bit
 * converter.  The mutex serialises access -- it is taken/released by the generic
 * adc_getRawSample() wrapper (peripherals/adc.h), so the driver's sample() need
 * not lock. */
static pthread_mutex_t adc1Mutex;
ADC_HRC7000_DEVICE_DEFINE(adc1, &adc1Mutex, ADC_COUNTS_TO_UV(3300000, 10))

/* Radio-bus I2C1 DesignWare master (base 0x14070000) -- the AT1846S transceiver.
 * Pads PTA7/PTA8 are muxed to I2C1 by platform_init's IO_DIPLEX0 baseline; the
 * mutex is taken via i2c_acquire()/i2c_release() by the AT1846S driver. */
static pthread_mutex_t i2c1Mutex = PTHREAD_MUTEX_INITIALIZER;
I2C_HRC7000_DEVICE_DEFINE(i2c1, I2C1, &i2c1Mutex)

/* Internal I2C2 DesignWare master (base 0x14080000) -- the RTC's dedicated bus.
 * The mutex is taken via i2c_acquire()/i2c_release() by the RTC driver. */
static pthread_mutex_t i2c2Mutex = PTHREAD_MUTEX_INITIALIZER;
I2C_HRC7000_DEVICE_DEFINE(i2c2, I2C2, &i2c2Mutex)

const hwInfo_t hwInfo = {
    .name = "HD2",
    .hw_version = 0,
    .flags = 0,
    .uhf_band = 1,
    .vhf_band = 1,
    .uhf_maxFreq = 480,
    .uhf_minFreq = 400,
    .vhf_maxFreq = 174,
    .vhf_minFreq = 136,
};

/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Polling driver for the HR_C7000 on-chip ADC (Ailunce HD2 / CSKY V2).
 */

#ifndef ADC_HD2_H
#define ADC_HD2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Perform one single-shot 10-bit conversion on the given ADC channel and
 * return the raw sample (0..1023).  Busy-polls the ADC_CTRL_STATE register
 * with a bounded guard loop -- no RTOS / semaphore.
 *
 * @param channel: ADC channel index 0..7 (battery pack is channel 2).
 * @return raw 10-bit sample, or 0 on timeout / invalid channel.
 */
uint16_t adc_hd2_sample(uint8_t channel);

/**
 * One-time ADC controller reset + config (the vendor adc_controller_init
 * reset-release pulse our driver was missing -- without it the conversion FSM
 * stays held and DATA reads 0).  adc_hd2_sample() calls this lazily on first
 * use.  LIVE-VERIFIED 2026-06-01: ch2 then reads a stable ~0x357.
 */
void adc_hd2_init(void);

/**
 * Read the battery-pack ADC channel (channel 2), apply the vendor's
 * "(raw << 2)" pre-scale and 16-sample sliding average, and return the
 * averaged raw value used by the vendor's voltage/percentage math.
 *
 * The first call seeds the whole window from the first reading so the
 * average is sane immediately.
 *
 * @return averaged scaled raw value (~0xbf4 == 7.4V on the vendor scale).
 */
uint16_t adc_hd2_battery_raw_avg(void);

#ifdef __cplusplus
}
#endif

#endif /* ADC_HD2_H */

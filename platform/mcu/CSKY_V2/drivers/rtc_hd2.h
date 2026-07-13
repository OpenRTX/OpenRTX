/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Real-time-clock driver for the Ailunce HD2 (Dahua HR_C7000, C-SKY V2
 * ck803s).  The HR_C7000 has an on-chip RTC that is hung off the SoC's
 * "internal I2C2" master (manual section 4.12, "DH4570 中 RTC 挂载于内部
 * I2C2 上").  The RTC core is fed by an external 32.768 kHz crystal and is
 * kept alive across power-down by a coin cell (LDO_RTC_AVDD33).
 *
 * Register layout / access pattern reverse-engineered from vendor V2.1.3
 * firmware (assets/source_v213/0300d000_v2_1_3_app.c):
 *
 *   rtc_reg_write(reg, val)  ->  I2C2 write, dev 0xE0 (7-bit 0x70<<1), 1 byte
 *   rtc_reg_read(reg)        ->  I2C2 read,  dev 0xE0, 1 byte
 *   i2c2_reinit @ 0x030599f8 -> DesignWare I2C controller base 0x14080000
 *                               (movih r4, 5128 == 0x1408_0000)
 *
 * Values are plain BINARY (not BCD).  The seconds/minute registers are
 * 6-bit, the hour register 5-bit and the day register a 16-bit count of
 * days since 1970-01-01 (verified: 1970-01-01 -> 0, 2024-01-01 -> 19723).
 */

#ifndef RTC_HD2_H
#define RTC_HD2_H

#include <stdint.h>
#include <stdbool.h>
#include "core/datetime.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise access to the HR_C7000 internal RTC.
 *
 * Brings up the DesignWare I2C2 master that the RTC is wired to and makes
 * sure the RTC counter is running (sets RTC_CCR run bit).  The RTC core
 * itself is battery-backed, so this does NOT perform the cold first-power
 * initialisation sequence (manual 4.12.3.1 steps 1-5) -- it only ensures
 * the timekeeper is ticking.  Safe to call repeatedly.
 */
void rtc_hd2_init(void);

/**
 * Read the current date and time from the RTC.
 *
 * @return populated datetime_t (year is 0-99, i.e. calendar year - 2000).
 */
datetime_t rtc_hd2_getTime(void);

/**
 * Program the RTC with a new date and time.
 *
 * @param t: datetime_t to load (year 0-99 == 2000..2099).
 */
void rtc_hd2_setTime(datetime_t t);

#ifdef __cplusplus
}
#endif

#endif /* RTC_HD2_H */

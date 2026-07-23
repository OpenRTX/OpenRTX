/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * RTC driver for the Ailunce HD2 (HR_C7000 / CK803S), implementing
 * OpenRTX peripherals/rtc.h.
 *
 * The HR_C7000 RTC is an I2C timekeeper on the SoC's internal I2C2 bus
 * (slave 0x70; manual 4.9), accessed through the shared HR_C7000 I2C driver
 * (i2c_hrc7000, instance `i2c2` in hwconfig.c).  Register values are plain
 * BINARY (not BCD): seconds/minutes are 6-bit, hours 5-bit, and the day field
 * is a 16-bit count of days since 1970-01-01.  The core is battery-backed and
 * keeps running across power cycles, so init only ensures the run bit is set
 * rather than replaying the cold first-power sequence.
 */

#include "peripherals/rtc.h"
#include "core/datetime.h"
#include "hwconfig.h" /* i2c2 device instance + RTC_I2C_SLAVE (registers.h) */
#include "i2c_hrc7000.h" /* i2c_init */

/* RTC register offsets (manual 4.9.4). */
enum {
    RTC_VAL_S = 0x00,
    RTC_VAL_M = 0x01,
    RTC_VAL_H = 0x02,
    RTC_VAL_D_L = 0x03,
    RTC_VAL_D_H = 0x04,
    RTC_LOAD_S = 0x0c,
    RTC_LOAD_M = 0x0d,
    RTC_LOAD_H = 0x0e,
    RTC_LOAD_D_L = 0x0f,
    RTC_LOAD_D_H = 0x10,
    RTC_CCR = 0x11,
    RTC_LOAD_VSTAT = 0x12,
};

/* Clock-control register (RTC_CCR) bits. */
enum {
    RTC_CCR_RUN = (1u << 1),
    RTC_CCR_LOAD = (1u << 2),
};

/* ---- register access over the shared I2C driver -------------------- */

static void rtc_reg_write(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };

    i2c_acquire(&i2c2);
    i2c_write(&i2c2, RTC_I2C_SLAVE, buf, sizeof(buf), true);
    i2c_release(&i2c2);
}

static uint8_t rtc_reg_read(uint8_t reg)
{
    uint8_t val = 0;

    /* Combined repeated-START read: write the register pointer (no STOP), then
     * read one byte (STOP).  The mutex is held across both so the pointer write
     * and the read are not interleaved by another bus user. */
    i2c_acquire(&i2c2);
    i2c_write(&i2c2, RTC_I2C_SLAVE, &reg, 1, false);
    i2c_read(&i2c2, RTC_I2C_SLAVE, &val, 1, true);
    i2c_release(&i2c2);

    return val;
}

/* ---- OpenRTX peripherals/rtc.h interface --------------------------- */

void rtc_init()
{
    uint8_t ccr;

    i2c_init(&i2c2, I2C_SPEED_400kHz);
    ccr = rtc_reg_read(RTC_CCR);
    if ((ccr & RTC_CCR_RUN) == 0u)
        rtc_reg_write(RTC_CCR, ccr | RTC_CCR_RUN);
}

void rtc_terminate()
{
    /* The RTC is battery-backed and keeps running; nothing to shut down. */
}

datetime_t rtc_getTime()
{
    datetime_t t = { 0 };
    uint8_t s, m, h, dl, dh;
    int32_t days, year;
    uint32_t mon, mday;

    s = rtc_reg_read(RTC_VAL_S);
    m = rtc_reg_read(RTC_VAL_M);
    h = rtc_reg_read(RTC_VAL_H);
    dl = rtc_reg_read(RTC_VAL_D_L);
    dh = rtc_reg_read(RTC_VAL_D_H);

    t.second = (int8_t)(s & 0x3fu);
    t.minute = (int8_t)(m & 0x3fu);
    t.hour = (int8_t)(h & 0x1fu);

    days = (int32_t)(((uint32_t)dh << 8) | (uint32_t)dl);
    daysToCivil(days, &year, &mon, &mday);
    t.date = (int8_t)mday;
    t.month = (int8_t)mon;
    t.year = (uint8_t)((year >= 2000) ? (year - 2000) : 0);

    /* 1970-01-01 was a Thursday; map to 1..7 (Mon..Sun). */
    t.day = (int8_t)((((days % 7) + 7 + 3) % 7) + 1);
    return t;
}

void rtc_setTime(datetime_t t)
{
    int32_t days;
    uint32_t day16;
    uint8_t ccr;

    days = civilToDays(2000 + (int32_t)t.year, (uint32_t)t.month,
                       (uint32_t)t.date);
    if (days < 0)
        days = 0;
    day16 = (uint32_t)days & 0xffffu;

    rtc_reg_write(RTC_LOAD_VSTAT, 0u);
    rtc_reg_write(RTC_LOAD_H, (uint8_t)((uint32_t)t.hour & 0x1fu));
    rtc_reg_write(RTC_LOAD_M, (uint8_t)((uint32_t)t.minute & 0x3fu));
    rtc_reg_write(RTC_LOAD_S, (uint8_t)((uint32_t)t.second & 0x3fu));
    rtc_reg_write(RTC_LOAD_D_L, (uint8_t)(day16 & 0xffu));
    rtc_reg_write(RTC_LOAD_D_H, (uint8_t)((day16 >> 8) & 0xffu));
    rtc_reg_write(RTC_LOAD_VSTAT, 1u);

    ccr = rtc_reg_read(RTC_CCR);
    rtc_reg_write(RTC_CCR, ccr | RTC_CCR_LOAD | RTC_CCR_RUN);
}

void rtc_setHour(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    datetime_t t = rtc_getTime();
    t.hour = (int8_t)hours;
    t.minute = (int8_t)minutes;
    t.second = (int8_t)seconds;
    rtc_setTime(t);
}

void rtc_setDate(uint8_t date, uint8_t month, uint8_t year)
{
    datetime_t t = rtc_getTime();
    t.date = (int8_t)date;
    t.month = (int8_t)month;
    t.year = year;
    rtc_setTime(t);
}

void rtc_dstSet()
{
} /* no hardware DST bit; handled in higher layers */
void rtc_dstClear()
{
}

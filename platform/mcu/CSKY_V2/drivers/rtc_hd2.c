/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * RTC driver for the Ailunce HD2 (Dahua HR_C7000 / C-SKY V2 ck803s).
 *
 * The HR_C7000 RTC is an I2C-attached timekeeper hung off the SoC's
 * "internal I2C2" Synopsys DesignWare APB I2C master.  Everything here is
 * derived from the HR_C7000 manual section 4.12 ("RTC") and the vendor
 * V2.1.3 firmware decompile.
 *
 *   Manual 4.12.3       : "DH4570 中 RTC 挂载于内部 I2C2 上, 通过 I2C 总线进行操作."
 *   Manual table 0x190  : "0x1408_0000 .. 0x1408_FFFF  I2C2  64KB  内部 RTC 专用"
 *   Manual 4.12.3.1 #3  : RTC device address 7'b1110000 == 0x70.
 *   Manual 4.12.4       : register table (offsets 0x00..0x17, see below).
 *
 *   vendor rtc_reg_write @ 0x03059e44  -> i2c2 write dev 0xE0 (0x70<<1)
 *   vendor rtc_reg_read  @ 0x03059e60  -> i2c2 read  dev 0xE0
 *   vendor i2c2_reinit   @ 0x030599f8  -> DesignWare base 0x14080000
 *                                         (movih r4, 5128 == 0x1408_0000)
 *   vendor rtc_set_datetime @ 0x0305a008, rtc_get_datetime @ 0x03059e7c
 *
 * Register values are plain BINARY (not BCD).  Seconds/minutes are 6-bit,
 * hours 5-bit, and the day field is a 16-bit count of days since
 * 1970-01-01 (verified against the vendor day-count math: 1970-01-01 -> 0,
 * 2024-01-01 -> 19723, which is the correct Unix day number).
 */

#include "drivers/rtc_hd2.h"
#include "hd2_regs.h"
#include "interfaces/delays.h"

/* ---- DesignWare APB I2C2 controller (RTC bus) ----------------------- *
 *
 * Base 0x14080000.  The controller registers (I2C2_BASE, IC_CON/TAR/
 * DATA_CMD/FS_SCL_HCNT/FS_SCL_LCNT/INTR_MASK/RX_TL/TX_TL/ENABLE/STATUS/
 * START) come from hd2_regs.h -- the standard Synopsys DW_apb_i2c map;
 * the ones the vendor touches were confirmed in the i2c2_reinit /
 * i2c2_tx_loop / i2c2_rx_loop disassembly.
 */

/* IC_DATA_CMD command bits */
#define CMD_READ           0x100u            /* read request          */
#define CMD_STOP           0x200u            /* issue STOP after byte */

/* IC_STATUS bits */
#define STA_TFNF           (1u << 1)         /* tx FIFO not full      */
#define STA_TFE            (1u << 2)         /* tx FIFO empty         */
#define STA_RFNE           (1u << 3)         /* rx FIFO not empty     */

/* RTC slave: 7-bit 0x70 (manual 4.12.3.1 step 3, "设备地址 7'b1110000"). */
#define RTC_ADDR_7BIT      0x70u

/* RTC register offsets (manual table 4.20, section 4.12.4). */
#define RTC_VAL_S          0x00u   /* current seconds (RO)            */
#define RTC_VAL_M          0x01u   /* current minutes (RO)            */
#define RTC_VAL_H          0x02u   /* current hours   (RO)            */
#define RTC_VAL_D_L        0x03u   /* current day count, low  (RO)    */
#define RTC_VAL_D_H        0x04u   /* current day count, high (RO)    */
#define RTC_LOAD_S         0x0Cu   /* load seconds                    */
#define RTC_LOAD_M         0x0Du   /* load minutes                    */
#define RTC_LOAD_H         0x0Eu   /* load hours                      */
#define RTC_LOAD_D_L       0x0Fu   /* load day count, low             */
#define RTC_LOAD_D_H       0x10u   /* load day count, high            */
#define RTC_CCR            0x11u   /* bit0 IRQ-en, bit1 run, bit2 load-now */
#define RTC_LOAD_VSTAT     0x12u   /* bit0: load values valid         */
#define RTC_SOFT_RST_N     0x17u   /* 0 = assert soft reset           */

#define RTC_CCR_IRQ_EN     (1u << 0)
#define RTC_CCR_RUN        (1u << 1)
#define RTC_CCR_LOAD       (1u << 2)

/*
 * APB clock feeding the I2C2 controller, in Hz.  Used only to derive the
 * 400 kHz SCL high/low counts -- the RTC bus is slow and tolerant, so the
 * exact value is non-critical.  Post-PLL the HD2 APB runs ~42 MHz (memory
 * project-hd2-clock-init).  Mirrors the vendor's clk/400000 split.
 */
#ifndef I2C2_APB_CLK_HZ
#define I2C2_APB_CLK_HZ    42000000u
#endif

/* Generous spin-loop bound so a wedged bus can't hang the UI forever.   */
#define I2C2_SPIN_LIMIT    0x40000

static bool rtc_ready = false;

/* ---- low-level DesignWare I2C2 helpers ------------------------------ */

/*
 * Configure the I2C2 master for 400 kHz, 7-bit addressing, RTC as target.
 * Mirrors vendor i2c2_reinit @ 0x030599f8.
 */
static void i2c2_setup(void)
{
    uint32_t total = I2C2_APB_CLK_HZ / 400000u;   /* SCL period in clks  */
    uint32_t hcnt  = total >> 1;

    IC_ENABLE      = 0u;                  /* must be off to reconfigure  */
    IC_CON         = 0x65u;              /* master, 7-bit, FS, restart  */
    IC_INTR_MASK   = 0x40u;              /* (matches vendor)            */
    IC_FS_SCL_HCNT = (hcnt > 8u) ? (hcnt - 8u) : 1u;
    IC_FS_SCL_LCNT = (total > hcnt + 1u) ? (total - 1u - hcnt) : 1u;
    IC_TX_TL       = 6u;
    IC_RX_TL       = 6u;
    IC_TAR         = RTC_ADDR_7BIT;      /* talk to the RTC             */
}

/* Write one byte to RTC register reg.  Mirrors vendor rtc_reg_write. */
static void rtc_reg_write(uint8_t reg, uint8_t val)
{
    int spin;

    IC_TAR    = RTC_ADDR_7BIT;
    IC_ENABLE = 1u;

    /* push register pointer, then the data byte */
    for (spin = I2C2_SPIN_LIMIT; spin && !(IC_STATUS & STA_TFNF); spin--) { }
    IC_DATA_CMD = reg;
    for (spin = I2C2_SPIN_LIMIT; spin && !(IC_STATUS & STA_TFNF); spin--) { }
    IC_DATA_CMD = (uint32_t)val | CMD_STOP;

    /* HR_C7000 quirk (manual 5.1.6.27): unlike stock DesignWare, this I2C
     * master does NOT auto-start once the FIFO has data -- it waits for an
     * explicit IC_START (0xa0) trigger.  Without it the bytes sit in the TX
     * FIFO forever (TXFLR stuck, no abort). LIVE-VERIFIED 2026-06-01. */
    IC_START = 1u;

    /* wait for tx FIFO to drain (STA_TFE) before releasing the bus */
    for (spin = I2C2_SPIN_LIMIT; spin && !(IC_STATUS & STA_TFE); spin--) { }

    IC_START  = 0u;
    IC_ENABLE = 0u;
}

/* Read one byte from RTC register reg.  Mirrors vendor rtc_reg_read. */
static uint8_t rtc_reg_read(uint8_t reg)
{
    int spin;
    uint8_t val;

    IC_TAR    = RTC_ADDR_7BIT;
    IC_ENABLE = 1u;

    /* set the register pointer with a write ... */
    for (spin = I2C2_SPIN_LIMIT; spin && !(IC_STATUS & STA_TFNF); spin--) { }
    IC_DATA_CMD = reg;

    /* ... then a single read command with STOP */
    for (spin = I2C2_SPIN_LIMIT; spin && !(IC_STATUS & STA_TFNF); spin--) { }
    IC_DATA_CMD = CMD_READ | CMD_STOP;

    /* HR_C7000 IC_START trigger -- see rtc_reg_write (manual 5.1.6.27). */
    IC_START = 1u;

    /* pull the resulting byte out of the rx FIFO */
    for (spin = I2C2_SPIN_LIMIT; spin && !(IC_STATUS & STA_RFNE); spin--) { }
    val = (uint8_t)IC_DATA_CMD;

    IC_START  = 0u;
    IC_ENABLE = 0u;
    return val;
}

/* ---- date <-> day-count conversion ---------------------------------- *
 *
 * The RTC day register counts days since 1970-01-01.  We convert with
 * Howard Hinnant's branch-free civil-date algorithms (epoch 1970), which
 * exactly reproduce the vendor's day numbers without dragging in libc
 * <time.h>.  Years handled here are 2000..2099 (datetime_t.year is 0-99).
 */

/* y/m/d (proleptic Gregorian) -> days since 1970-01-01. */
static int32_t civil_to_days(int32_t y, uint32_t m, uint32_t d)
{
    y -= (m <= 2);
    int32_t era = (y >= 0 ? y : y - 399) / 400;
    uint32_t yoe = (uint32_t)(y - era * 400);                 /* [0, 399]  */
    uint32_t doy = (153u * (m + (m > 2 ? -3u : 9u)) + 2u) / 5u + d - 1u; /* [0, 365] */
    uint32_t doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;  /* [0, 146096] */
    return era * 146097 + (int32_t)doe - 719468;
}

/* days since 1970-01-01 -> y/m/d. */
static void days_to_civil(int32_t z, int32_t *y, uint32_t *m, uint32_t *d)
{
    z += 719468;
    int32_t era = (z >= 0 ? z : z - 146096) / 146097;
    uint32_t doe = (uint32_t)(z - era * 146097);              /* [0, 146096] */
    uint32_t yoe = (doe - doe / 1460u + doe / 36524u - doe / 146096u) / 365u;
    int32_t yr = (int32_t)yoe + era * 400;
    uint32_t doy = doe - (365u * yoe + yoe / 4u - yoe / 100u);
    uint32_t mp = (5u * doy + 2u) / 153u;                     /* [0, 11]   */
    *d = doy - (153u * mp + 2u) / 5u + 1u;                    /* [1, 31]   */
    *m = mp + (mp < 10u ? 3u : -9u);                          /* [1, 12]   */
    *y = yr + (*m <= 2);
}

/* ---- public API ------------------------------------------------------ */

void rtc_hd2_init(void)
{
    uint8_t ccr;

    i2c2_setup();

    /*
     * The RTC core is battery-backed and normally already running from a
     * previous boot, so we do NOT replay the cold first-power init
     * (manual 4.12.3.1 steps 1-5).  Just make sure the run bit is set so
     * the counter is ticking.
     */
    ccr = rtc_reg_read(RTC_CCR);
    if ((ccr & RTC_CCR_RUN) == 0u)
        rtc_reg_write(RTC_CCR, ccr | RTC_CCR_RUN);

    rtc_ready = true;
}

datetime_t rtc_hd2_getTime(void)
{
    datetime_t t = {0};
    uint8_t s, m, h, dl, dh;
    int32_t days, year;
    uint32_t mon, mday;

    if (!rtc_ready)
        rtc_hd2_init();

    s  = rtc_reg_read(RTC_VAL_S);
    m  = rtc_reg_read(RTC_VAL_M);
    h  = rtc_reg_read(RTC_VAL_H);
    dl = rtc_reg_read(RTC_VAL_D_L);
    dh = rtc_reg_read(RTC_VAL_D_H);

    t.second = (int8_t)(s & 0x3Fu);     /* 6-bit field */
    t.minute = (int8_t)(m & 0x3Fu);     /* 6-bit field */
    t.hour   = (int8_t)(h & 0x1Fu);     /* 5-bit field */

    days = (int32_t)(((uint32_t)dh << 8) | (uint32_t)dl);
    days_to_civil(days, &year, &mon, &mday);

    t.date  = (int8_t)mday;
    t.month = (int8_t)mon;
    t.year  = (uint8_t)((year >= 2000) ? (year - 2000) : 0);

    /* day-of-week: 1970-01-01 was a Thursday (== 4); map to 1..7 (Mon..Sun) */
    {
        int32_t dow = ((days % 7) + 7 + 3) % 7;   /* 0 = Monday */
        t.day = (int8_t)(dow + 1);
    }

    return t;
}

void rtc_hd2_setTime(datetime_t t)
{
    int32_t days;
    uint32_t day16;

    if (!rtc_ready)
        rtc_hd2_init();

    days  = civil_to_days(2000 + (int32_t)t.year,
                          (uint32_t)t.month, (uint32_t)t.date);
    if (days < 0)
        days = 0;
    day16 = (uint32_t)days & 0xFFFFu;

    /*
     * Load sequence, mirroring vendor rtc_set_datetime @ 0x0305a008:
     *   - clear LOAD_VSTAT
     *   - write the five LOAD registers (binary values)
     *   - set LOAD_VSTAT
     *   - pulse CCR load bit (rtc_commit) to latch the new time
     */
    rtc_reg_write(RTC_LOAD_VSTAT, 0u);
    rtc_reg_write(RTC_LOAD_H,   (uint8_t)((uint32_t)t.hour   & 0x1Fu));
    rtc_reg_write(RTC_LOAD_M,   (uint8_t)((uint32_t)t.minute & 0x3Fu));
    rtc_reg_write(RTC_LOAD_S,   (uint8_t)((uint32_t)t.second & 0x3Fu));
    rtc_reg_write(RTC_LOAD_D_L, (uint8_t)(day16 & 0xFFu));
    rtc_reg_write(RTC_LOAD_D_H, (uint8_t)((day16 >> 8) & 0xFFu));
    rtc_reg_write(RTC_LOAD_VSTAT, 1u);

    /* commit: CCR bit2 latches LOAD_* into the running counter (self-clears) */
    {
        uint8_t ccr = rtc_reg_read(RTC_CCR);
        rtc_reg_write(RTC_CCR, ccr | RTC_CCR_LOAD | RTC_CCR_RUN);
    }
}

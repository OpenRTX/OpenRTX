/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HD2 OpenRTX bring-up.  Most platform.h functions are stubs or use
 * mid-pack defaults until the ADC/battery driver lands.  The chip-
 * level writes in platform_init() are derived from a live dbgshell
 * snapshot of V2.1.3 (memory project-hd2-v213-mmio-snapshot) and are
 * tentative -- 2026-05-29 they got us to a white-screen state but the
 * LCD command path isn't fully working yet.  Other agents: do not
 * refactor or extend this code as if it's a settled implementation.
 *
 * Platform-level glue for the Ailunce HD2.  Implements the API in
 * openrtx/include/interfaces/platform.h with the minimum needed for
 * a first-bring-up OpenRTX build.
 *
 * Most measurements (battery, mic, RSSI) are stubbed to constant
 * values for now -- they need real ADC bring-up which can come once
 * we know the rest of the core compiles + boots.  See
 * src/firmware/<subsys>/ for the extracted vendor code we'll
 * port driver-by-driver.
 */

#include "interfaces/platform.h"
#include "interfaces/delays.h"
#include "hwconfig.h"
#include "hd2_regs.h"
#include "drivers/adc_hd2.h"
#include "drivers/GPS/gps_HD2.h"
#include "drivers/rtc_hd2.h"
#include "core/battery.h"
#include <string.h>
#include <stdio.h>
/* Hard-lock trace forensics (boot counter in the banner).  The kernel trace
 * facility header (hd2_trace.h) is on the build's include path (openrtx_hd2
 * CMake adds arch/chip/hr_c7000); gated behind WITH_HD2_TRACE. */
#include "hd2_trace.h"

/* ---- UART polled write helper for early boot debug ------------------
 *
 * Duplicates uart_putc/uart_puts from src/diagboot-c/.  Kept inline
 * here so the very first build doesn't require a real GPIO/UART driver
 * in platform/drivers/uart/.  Once that exists, drop these.
 *
 * NOTE: the debug console is UART0 (0x14030000); the helper just used to
 * be misnamed "UART2_*" against that address.  Now routed via hd2_regs.h.
 */
static void hd2_uart_putc(char c)
{
    while ((UART0_LSR & LSR_TX_RDY) == 0) { }
    UART0_THR = (unsigned char)c;
}

static void hd2_uart_puts(const char *s)
{
    while (*s) hd2_uart_putc(*s++);
}


/* ---- hwInfo: what the radio is ------------------------------------- */
static const hwInfo_t hwInfo =
{
    .name        = "HD2",
    .hw_version  = 1,
    .flags       = 0,
    .uhf_band    = 1,
    .vhf_band    = 1,
    .uhf_minFreq = 400,
    .uhf_maxFreq = 480,
    .vhf_minFreq = 136,
    .vhf_maxFreq = 174,
};


/* ---- Required platform.h interface --------------------------------- */

/*
 * clk_init_pll -- port of vendor V2.1.3 FUN_00030b6c at IRAM 0x00030b6c.
 *
 * Without this the chip's PLL is in its IAP-default state and several
 * peripherals (ADC at 0x140d0000, SPI Master 0/1 at 0x140A/B0000) don't
 * respond.  Live-verified 2026-05-30: writing only `clk_ctrl |= 3` is
 * enough to shift the chip clock (UART baud immediately desyncs), so we
 * MUST do the full PLL bring-up + UART recalibration in one go.
 *
 * Decompiled vendor sequence:
 *   socsys+0x30 = 2
 *   socsys+0x10 = (old & 0xff000000) | 0x712
 *   socsys+0x28 = 6
 *   socsys+0x24 = (old & 0xfffffff0) | 4
 *   socsys+0x04 |= 3                    ; engage PLL
 *   spin until bit 31 set               ; PLL locked
 *   socsys+0x04 &= ~bit2 ; delay
 *   socsys+0x04 &= ~bit3 ; delay
 *   spin until bit 30 set               ; second lock
 *   socsys+0x2c = 0xfff0ff3c
 *
 * Vendor also caches four computed frequencies in IRAM
 * (0x000483f0/0x00048320/0x0004834c/0x000484bc) -- those feed the
 * vendor's later baud-rate / PWM divider math, not the PLL itself,
 * so we skip them.
 */
/* SOCSYS clock/PLL registers are defined in hd2_regs.h
 * (SOCSYS_CLK_CTRL, SOCSYS_REG10/24/28/2C/30). */

static void clk_delay_short(void)
{
    /* Vendor calls FUN_00030ab8(100) which is a busy-wait of 100 unknown
     * units.  Burn ~25k cycles which is generous at the ck803s default. */
    for (volatile uint32_t i = 0; i < 25000u; ++i) { }
}

/*
 * clk_wait_bit_stable -- the manual (ch.04, steps 8 & 10) requires the
 * pll_ld / clk_rdy flags to be confirmed asserted for MORE THAN 10
 * CONSECUTIVE reads before they may be trusted (or wait 100 us).  Our old
 * code broke on the first set read, which let us proceed onto an APLL that
 * had not actually locked -- switching the baseband clock onto an unstable
 * PLL leaves the Modem RX FSM unclocked (g_modem_irq_acc stuck at 0).
 * Returns 1 if the bit was confirmed stable, 0 on timeout.
 */
static int clk_wait_bit_stable(uint32_t mask)
{
    uint32_t consecutive = 0;
    for (uint32_t guard = 0; guard < 4000u; ++guard) {
        if ((SOCSYS_CLK_CTRL & mask) != 0u) {
            if (++consecutive > 10u) return 1;
        } else {
            consecutive = 0;
        }
    }
    return 0;
}

static void clk_init_pll(void)
{
    /* Program the CLK_MGR registers the IAP hand-off leaves implicit.  These
     * are the chip reset defaults -- the vendor's playing unit carries exactly
     * these values -- but we set them explicitly so the APLL (Modem/baseband
     * +audio source clock) and its dividers are in a known-good state before
     * we engage the PLLs and switch the baseband clock domain onto APLL. */
    SOCSYS_APLL      = SOCSYS_APLL_CFG;
    SOCSYS_CLKDIV_18 = SOCSYS_CLKDIV_18_CFG;
    SOCSYS_CLKDIV_1C = SOCSYS_CLKDIV_1C_CFG;
    SOCSYS_CLKDIV_20 = SOCSYS_CLKDIV_20_CFG;

    SOCSYS_REG30 = 2u;
    SOCSYS_REG10 = (SOCSYS_REG10 & 0xff000000u) | 0x712u;
    SOCSYS_REG28 = 6u;
    SOCSYS_REG24 = (SOCSYS_REG24 & 0xfffffff0u) | 4u;

    SOCSYS_CLK_CTRL |= CLK_CTRL_RE_CFG;       /* engage both PLLs (re-cfg) */

    /* Confirm PLL lock (bit 31) for >10 consecutive reads BEFORE switching the
     * CPU/baseband clocks onto the PLLs. */
    clk_wait_bit_stable(CLK_CTRL_PLL_LD);

    SOCSYS_CLK_CTRL &= ~CLK_CTRL_ACLK_WORK_SEL;   /* baseband -> APLL */
    clk_delay_short();
    SOCSYS_CLK_CTRL &= ~CLK_CTRL_BCLK_WORK_SEL;   /* SoC -> BPLL */

    /* Confirm clock-switch ready (bit 30) for >10 consecutive reads. */
    clk_wait_bit_stable(CLK_CTRL_CLK_RDY);

    SOCSYS_REG2C = 0xfff0ff3cu;                /* enable peripheral clock gates */
}

/*
 * clk_snapshot_prepll / clk_restore_prepll -- the IAP sets its UART
 * divisor assuming the pre-PLL (~84 MHz) clock it handed off with.  Our
 * clk_init_pll halves that to 42 MHz, so a direct force-DFU jump
 * (loader 'F') lands the IAP's UART at 28800 instead of 57600.  We
 * snapshot the SOCSYS clock regs as handed off (before clk_init_pll),
 * and restore them right before the jump to put the clock back.
 */
static uint32_t g_socsys_prepll[6];
static int      g_socsys_saved = 0;

static void clk_snapshot_prepll(void)
{
    g_socsys_prepll[0] = SOCSYS_CLK_CTRL;
    g_socsys_prepll[1] = SOCSYS_REG10;
    g_socsys_prepll[2] = SOCSYS_REG24;
    g_socsys_prepll[3] = SOCSYS_REG28;
    g_socsys_prepll[4] = SOCSYS_REG2C;
    g_socsys_prepll[5] = SOCSYS_REG30;
    g_socsys_saved = 1;
}

void clk_restore_prepll(void)
{
    if (!g_socsys_saved)
        return;
    /* CLK_CTRL bits 0/1 (PLL engage) were clear at handoff, so writing the
     * saved value disengages the PLL; restore the divider regs too. */
    SOCSYS_CLK_CTRL = g_socsys_prepll[0];
    SOCSYS_REG10    = g_socsys_prepll[1];
    SOCSYS_REG24    = g_socsys_prepll[2];
    SOCSYS_REG28    = g_socsys_prepll[3];
    SOCSYS_REG2C    = g_socsys_prepll[4];
    SOCSYS_REG30    = g_socsys_prepll[5];
    clk_delay_short();
}

/*
 * uart_recalibrate_57600 -- empirically derived from live tests
 * 2026-05-31, three boot variants on the running radio:
 *
 *   Stage         DLL   UART clock   Baud
 *   BOOTROM       13    ~24 MHz      115200    (per HR_C7000 manual)
 *   IAP handoff   91    ~84 MHz      57600     (probed: read DLL via 'D')
 *   Post-PLL      91    ~42 MHz      28800     (TUI baud sweep -- bingo at 28800)
 *
 * So clk_init_pll HALVES the UART input clock from 84 to 42 MHz.  That
 * matches the literal 0x0280de80 = 42,000,000 cached by vendor at IRAM
 * 0x0004834c -- vendor uses that constant downstream for baud math.
 *
 * For 57600 baud at 42 MHz UART input:
 *   DLL = 42_000_000 / (16 * 57600) = 45.57  ->  46
 */
/* UART0 DLL/DLH/LCR are defined in hd2_regs.h. */

static void uart_recalibrate_57600(void)
{
    /* DLAB=1 to expose DLL/DLH at +0x00/+0x04 */
    uint32_t lcr = UART0_LCR;
    UART0_LCR = lcr | LCR_DLAB;
    UART0_DLL = 46u;                    /* 57600 at 42 MHz post-PLL */
    UART0_DLH = 0u;
    UART0_LCR = lcr & ~LCR_DLAB;
}

/*
 * platform_init -- chip-level peripheral routing.
 *
 * Writes ten MMIO registers to match the live V2.1.3 working state
 * captured 2026-05-29 via dbgshell (see memory
 * [[project-hd2-v213-mmio-snapshot]]).  In particular:
 *
 *   - GPIOC DDR (0x14110004) = 0x7ffc enables bits 2-14 of the LCD bus
 *     as outputs.  Without this our LCD bit-bang writes ride a high-Z
 *     bus and the panel sees nothing.
 *   - PWM ch0 control words at +0x10..+0x1c = 0x300 each are the
 *     duty-honored gates per memory [[hd2-brightness-pwm]].  Without
 *     them the PWM duty register is ignored and the backlight stays
 *     dark.
 *   - socsys_io_diplex0/1/2 are written to the small "post-V2.1.3-
 *     boot-clean" values (high bits cleared) rather than the IAP-leaves
 *     state our previous bring-up inherited.
 *   - GPIOA/B DDR+DR snapshot V2.1.3's peripheral-rail state.
 *
 * If this build doesn't light the LCD, take a fresh dbgshell snapshot
 * after platform_init runs and diff against the V2.1.3 snapshot in the
 * memory entry.
 */
/* Free-running 42 MHz timebase for getTick()/delays -- see delays.c. */
extern void timer_init(void);

void platform_init(void)
{
    /* Engage the chip PLL, then immediately re-set UART DLL.  clk_init_pll
     * halves the UART input clock from 84 to 42 MHz, so the IAP's DLL=91
     * would land us at 28800 baud (live-verified).  uart_recalibrate
     * writes DLL=46 to keep 57600 across the transition. */
#ifndef _MIOSIX
    clk_snapshot_prepll();   /* capture IAP-handoff clock for force-DFU restore */
    clk_init_pll();
    uart_recalibrate_57600();
#else
    /* Under Miosix the KERNEL already ran this exact clk_init_pll sequence at
     * boot (vendor bsp IRQmemoryAndClockInit) AND set its tickless os_timer to
     * the resulting 42 MHz, plus UART0 to 57600 (DLL=46). Re-running clk_init_pll
     * here power-cycles the PLL (clocks drop to the crystal mid-flight), which
     * breaks the kernel's wakeup timer and HANGS Thread::sleep. So skip it. */
#endif

    /* Start the free-running DW timer (ch0) BEFORE anything that uses
     * delayUs/delayMs (LCD/SPI/I2C bit-bang all do).  42 MHz timebase,
     * measured live; replaces the old calibrated busy loop. */
    timer_init();

    /* socsys_io_diplex0/1/2 -- chip-level pin-mux (32-bit registers!).
     * BUG FIXED 2026-05-30: these were written as 0x000000xx, which ZEROED
     * the upper 24 bits = the GPIO-select fields for PTC9-22. That left the
     * GPIOC pads (LCD bus + keypad cols) muxed to peripheral function, so DR
     * writes never reached the pins and EXT_PORT read 0 -- the long-hunted
     * "GPIOC dead" blocker for BOTH the LCD and the keypad. The bad low-byte
     * values came from a byte-truncated MMIO snapshot (these regs only return
     * the low byte on byte-access). diplex2=0x3ffffffb puts all PTC pins in
     * GPIO mode -- LIVE-VERIFIED: GPIOC EXT_PORT then reads real data and
     * self-loopback works.  NOTE: 0x3ffffffb also forces the ADC pins
     * (PTC15-22) to GPIO; refine if/when the on-chip ADC is needed.
     * diplex0/1 (PTA pins) are also low-byte-only here but GPIOA works as-is
     * (its active pins live in the low byte); revisit if PTA>bit7 is needed.
     *
     * IO_DIPLEX0 bits 7/8 (i2c1_scl_sel/i2c1_sda_sel) mux PTA7(SCL)/PTA8(SDA)
     * between GPIO (=1) and the I2C_SCL_1/I2C_SDA_1 peripheral (=0).  We clear
     * both so the pins carry the HW I2C1 master (i2c_csky.c) for the AT1846S /
     * RDA5802E radio bus (confirmed against HR_C7000_v2.7_en manual 4.4.4.4).
     *
     * This MUST stay consistent with i2c0_init(), which fires from the AT1846S
     * constructor at STATIC-INIT time (before platform_init()).  History (the
     * "wedge on bring-up" root cause, 2026-06-12): an earlier unconditional
     * 0x1e0 left these pins as GPIO and disconnected the HW controller, so
     * every bring-up transfer timed out in the driver's bounded spin loops (the
     * rtx thread burning multi-second CPU = the "system wedge"), command bytes
     * rotting in the TX FIFO with no abort raised.  LIVE-VERIFIED clean with
     * bits 7/8 cleared + the 33 Hz RSSI poll over HW I2C1.
     * NOTE: bit 8 is WRITE-ONLY (reads as 0) -- never RMW this register
     * expecting bit 8 to survive, and readback can't verify the SDA mux. */
    SOCSYS_IO_DIPLEX0 = 0x00000060u;   /* diplex0: PTA7+PTA8 -> I2C1 controller (HW i2c) */

    /* IO_DIPLEX1 (manual 4.4.4.5) muxes the W25Q flash bit-bang pins:
     *   [1:0] spi0_csn0_sel = 3 -> GPIO_PTA18 (flash CS#)
     *   [4:3] spi0_sclk_sel = 3 -> GPIO_PTA20 (flash SCK)
     *   [6:5] spi0_mosi_sel = 3 -> GPIO_PTA21 (flash MOSI)
     *   [8:7] spi0_miso_sel = 3 -> GPIO_PTA22 (flash MISO)
     * BUG FIXED 2026-06-10: this was written as 0x000000ff, which left
     * [8:7] = 0b01 = "Slave SPI0_MISO" -- PTA22 was owned by the (dead)
     * SPI0 controller, so EXT_PORTA bit 22 read frozen and every
     * firmware-side flash READ returned flat 0x00, even though the
     * host-driven bit-bang on the same pins read the JEDEC id fine
     * (the host pokes GPIOA while the chip mux is still at its reset
     * default, where [8:7]=3=GPIO).  Same diplex byte-truncation
     * failure mode as the PTA8/i2c1_sda_sel bug above; this is the
     * prime suspect behind the firmware-side half of #73.  0x1ff =
     * the old low byte 0xff + bit 8; bits 9..31 stay 0 as before. */
#if defined(HD2_SPI_BITBANG) && (HD2_SPI_BITBANG)
    SOCSYS_IO_DIPLEX1 = 0x000001ffu;   /* diplex1: PTA18/20/21/22 -> GPIO (W25Q bit-bang) */
#else
    /* HW SPI0 for the W25Q (2026-06-13, live-verified JEDEC ef4020):
     * SCLK/MOSI/MISO -> master SPI0 (fields 0); CSN0 (PTA18) and CSN1
     * (PTA19) stay GPIO -- CS is driven by the flash driver/W25Qx.c as
     * before, which sidesteps the DW auto-CS behavior.  Like the other
     * diplex registers: write-only upper bits, constants only. */
    SOCSYS_IO_DIPLEX1 = 0x00000007u;
#endif
#if defined(HD2_LCD_BITBANG) && (HD2_LCD_BITBANG)
    SOCSYS_IO_DIPLEX2 = HD2_DIPLEX2_PTC_GPIO;  /* all PTC -> GPIO (latch bit-bang) */
#else
    /* HW i8080 LCD (2026-06-13, live-verified red-stripe test): the LCD
     * CS/RS/WR/RD/DB0-7 pads belong to the controller @0x12000000; keypad
     * scans swap them back to GPIO transiently (keyboard_HD2.c). */
    SOCSYS_IO_DIPLEX2 = HD2_DIPLEX2_LCD_I80;
#endif

    /* GPIO direction + data registers -- match V2.1.3 snapshot.  Use OR
     * for data so any LEDs the IAP/early-boot already set stay set. */
    GPIOA_DDR  = 0x003401e0u;          /* GPIOA DDR */
    GPIOA_DR  |= 0x001401e0u;          /* GPIOA DR  */

    GPIOB_DDR  = 0x3d7ae51fu;          /* GPIOB DDR */
    GPIOB_DR  |= 0x00506414u;          /* GPIOB DR  */

    /* GPS module power (GPIOB.15): the vendor IAP leaves it HIGH, and the
     * OR above preserved that -- so the module streamed NMEA into the
     * shared PTA11 pad (= PTT, active-low) even with GPS off in settings,
     * false-keying TX (2026-06-11 red-LED-blip bug).  Park it OFF here;
     * gps_HD2_enable() raises it when the user actually enables GPS. */
    GPIOB_DR  &= ~(1u << 15);

    GPIOC_DDR  = 0x00007ffcu;          /* GPIOC DDR -- LCD bus outputs */

    /* PWM ch0 (backlight) duty-gating control words.  V2.1.3 has all
     * four at 0x300; ours had them at 0 which silently squashed duty. */
    PWM_CH0_GATE0 = 0x00000300u;
    PWM_CH0_GATE1 = 0x00000300u;
    PWM_CH0_GATE2 = 0x00000300u;
    PWM_CH0_GATE3 = 0x00000300u;

    /* Internal RTC (DesignWare I2C2 @ 0x14080000).  Battery-backed, so
     * this just brings up the controller and ensures the counter runs. */
    rtc_hd2_init();

    /* Boot banner with identity + battery-backed RTC time.  When the hard-lock
     * trace is enabled it adds the boot counter (monotonic across watchdog
     * reboots -- hd2_trace.h; "warm" = previous life's block was valid, i.e. a
     * reboot, not a cold power-on), letting the host-side log order and
     * wall-clock the WDT crash-loop cycles. */
    {
        datetime_t t = rtc_hd2_getTime();
        char banner[96];
#ifdef WITH_HD2_TRACE
        snprintf(banner, sizeof banner,
                 "OpenRTX HD2 platform_init boot=%lu(%s) rtc=%02u:%02u:%02u\r\n",
                 (unsigned long)HD2_TRACE->boot_count,   /* live count (sane on cold boots) */
                 (hd2_trace_prev.magic == HD2_TRACE_MAGIC) ? "warm" : "cold",
                 (unsigned)t.hour, (unsigned)t.minute, (unsigned)t.second);
#else
        snprintf(banner, sizeof banner,
                 "OpenRTX HD2 platform_init rtc=%02u:%02u:%02u\r\n",
                 (unsigned)t.hour, (unsigned)t.minute, (unsigned)t.second);
#endif
        hd2_uart_puts(banner);
    }
}

/* Power control GPIOs (GPIOB @ 0x14100000), RE'd from V2.1.3 + live-
 * verified 2026-05-31 (see memory hd2-power-shutdown):
 *   GPIOB.13 (pin 0x2d) = power self-latch.  Driven HIGH to keep the
 *                         radio powered; drive LOW to cut the rail.
 *                         platform_init() already drives it high.
 *   GPIOB.12 (pin 0x2c) = volume/power knob sense.  LOW = on, HIGH = off
 *                         (live-verified: on=0x..6ee0, off=0x..7ee0). */
/* GPIOB_DR / GPIOB_EXT_PORT / PWR_HOLD_BIT / PWR_KNOB_BIT in hd2_regs.h. */

void platform_terminate(void)
{
    hd2_uart_puts("OpenRTX HD2 platform_terminate -- releasing power latch\r\n");
    /* Drop the self-latch: the main rail collapses and the MCU dies here. */
    GPIOB_DR &= ~PWR_HOLD_BIT;
    while (1) { }                /* never returns -- power is gone */
}

/* ---- battery voltage ------------------------------------------------
 *
 * The vendor V2.1.3 firmware samples the HR_C7000 ADC channel 2 via
 * FUN_0305ae94(2), runs a 16-sample sliding average, and stores the
 * scaled deci-volt value at chip_uid_buf+0x60.  The display path
 * (battery_voltage_display @ 0x03051a4c) formats it as 'X.Y V':
 *
 *     deci_volts = ((raw_avg * 99) & 0x7ffff) >> 12;
 *
 * Low-battery threshold is raw_avg < 0xa2f (= 2607).
 * Linear percentage:  pct = clamp((raw_avg - 0xa2e) / 7, 0, 100).
 *
 * The ADC read + 16-sample average now live in the polling driver
 * platform/mcu/CSKY_V2/drivers/adc_hd2.c (no RTOS / semaphore).  Here we
 * apply the vendor display scale:
 *
 *     deci_volts = (raw_avg * 99) >> 12;     // e.g. 74 == 7.4 V
 *     millivolts = deci_volts * 100;
 *
 * (vendor battery_voltage_display @ 0x03051a4c,
 *  assets/source_v213/0300d000_v2_1_3_app.c:85922.)
 */
uint16_t platform_getVbat(void)
{
    uint32_t raw_avg = adc_hd2_battery_raw_avg();

    /* The ADC now converts for real (#90 SOLVED 2026-06-01): adc_hd2_init()'s
     * controller reset-release pulse was the missing step.  ch2 reads a stable
     * ~0x350 -> raw_avg ~0xd40 -> ~8.1 V on a charged pack (HW-verified live).
     * The raw_avg==0 guard below is now only a defensive fallback (e.g. if the
     * controller ever fails to init) -- it reports a plausible mid-pack ~7.4 V
     * instead of a misleading 0%. */
    if (raw_avg == 0u)
        raw_avg = 0xbf4u;                    /* ~7.4 V fallback (should not hit) */

    uint32_t deci_volts = ((raw_avg * 99u) & 0x7ffffu) >> 12;
    return (uint16_t)(deci_volts * 100u);    /* millivolts */
}

/*
 * battery_getCharge -- state of charge in percent from pack millivolts.
 *
 * Port of the vendor's linear curve (FUN_0305853c,
 * assets/source_v213/0300d000_v2_1_3_app.c:93962-93985):
 *
 *     if (raw_avg < 0xa2f) low-battery;          // == 6.3 V on the scale
 *     pct = clamp((raw_avg - 0xa2e) / 7, 0, 100);
 *
 * The vendor curve is defined over the averaged ADC raw value, so we invert
 * platform_getVbat's scale to reconstruct raw_avg from millivolts:
 *
 *     deci_volts = (raw_avg * 99) >> 12   =>   raw_avg ~= mV * 4096 / 9900
 *
 * Sanity: 7400 mV -> raw_avg 3062 (~0xbf6) -> pct ~65%; 6300 mV -> raw_avg
 * 2606 (0xa2e) -> pct 0% (matches the vendor low-battery cutoff at 0xa2f).
 */
uint8_t battery_getCharge(uint16_t vbat)
{
    uint32_t raw_avg = ((uint32_t)vbat * 4096u) / 9900u;

    if (raw_avg <= 0xa2eu)
        return 0;

    uint32_t pct = (raw_avg - 0xa2eu) / 7u;
    if (pct > 100u)
        pct = 100u;
    return (uint8_t)pct;
}

/* ---- microphone input (SCOPED, not yet implemented) -----------------
 *
 * The HD2 mic is NOT on the SoC general ADC.  The SoC ADC (vendor
 * FUN_0305ae94 @ 0x0305ae94, 8 channels) samples only DC rails -- ch2 is
 * the battery (see platform_getVbat above); none are the mic.
 *
 * Mic audio is digitised inside the HR_C7000 modem's codec/PCM block, the
 * same hardware boot_init_modem_audio_path() (@0x0305e62c) brings up for RX
 * audio.  The analog mic path is gated by mic_path_set_by_freq()
 * (@0x03040fb4), which drives GPIO pin id 0x23 (gpio_out_set(0x23)) to
 * route the electret mic into the codec; spkr_amp_mute() (@0x03040ff8)
 * clears the same pin.  Mic GAIN is set via audio_codec_set_gain_a()
 * (vendor default audio_codec_set_gains(0x14, 0x13)); the live mic level
 * the UI shows comes from a codec register read, not an ADC.
 *
 * Implementing this requires the full HR_C7000 PCM/codec port (modem I2C
 * register block at +0xc8..0xe5, PCM handshake on _hrc7000_pcm_handshake)
 * plus codec2 for the digital path -- out of scope for tone output.
 * Return 0 until that driver exists.
 */
uint8_t platform_getMicLevel(void)
{
    return 0;
}

/* Volume knob = analog pot on ADC ch0 (the audio gain itself is analog in HW;
 * ch0 is the position tap for the UI bar).  Live-mapped 2026-06-01: ch0 ~0xd4
 * low .. ~0x15d at vol 100.  MIN/MAX are calibration bounds (refine on HW). */
#define VOL_ADC_CH      0u
#define VOL_ADC_MIN     0x0c0u   /* just above the power-off detent (TODO: calibrate) */
#define VOL_ADC_MAX     0x170u   /* full volume (TODO: calibrate)                     */

uint8_t platform_getVolumeLevel(void)
{
    uint16_t raw = adc_hd2_sample(VOL_ADC_CH);   /* 10-bit pot reading */
    if (raw <= VOL_ADC_MIN) return 0;
    if (raw >= VOL_ADC_MAX) return 255;
    return (uint8_t)(((uint32_t)(raw - VOL_ADC_MIN) * 255u)
                     / (VOL_ADC_MAX - VOL_ADC_MIN));
}

/* Channel knob = 2-bit quadrature encoder on GPIOB.5 (A) / GPIOB.6 (B), live-
 * mapped 2026-06-01.  Incremental (up/down), so we decode A/B transitions into
 * an accumulating position counter -- MUST be polled frequently (the UI calls
 * this every iteration).  CW gray sequence is 00->01->11->10. */
#define CH_ENC_A_BIT    5u
#define CH_ENC_B_BIT    6u
/* Quarter-steps per emitted channel step.  HW measurement: polled (non-IRQ)
 * decoding catches only ~1-1.5 of the 4 quadrature edges per detent (the
 * poll loop is too slow to see them all), so 1 gives ~1 channel/click.
 * Reliable 1:1 would need a GPIO interrupt on PB5/PB6 (not wired -- TODO). */
#define CH_ENC_DETENT   1

int8_t platform_getChSelector(void)
{
    static uint8_t prev     = 0xffu;   /* last gray code (A<<1|B); 0xff = uninit */
    static int16_t quarters = 0;       /* accumulated quarter-steps              */

    uint32_t g   = GPIOB_EXT_PORT;
    uint8_t  cur = (uint8_t)((((g >> CH_ENC_A_BIT) & 1u) << 1) |
                              ((g >> CH_ENC_B_BIT) & 1u));   /* 00,01,11,10 = 0,1,3,2 */

    if (prev == 0xffu) {
        prev = cur;
    }
    else if (cur != prev) {
        uint8_t t = (uint8_t)((prev << 2) | cur);
        switch (t) {
            case 0x1: case 0x7: case 0xE: case 0x8: quarters++; break;  /* CW  */
            case 0x2: case 0xB: case 0xD: case 0x4: quarters--; break;  /* CCW */
            default: break;                                             /* missed step */
        }
        prev = cur;
    }
    return (int8_t)(quarters / CH_ENC_DETENT);
}

bool platform_getPttStatus(void)
{
    /* PTT = GPIOB.11, active-low (PTT_BIT).  Live-verified 2026-06-11 by a
     * run-length GPIO sweep: a held press is a solid 20 s low on GPIOB.11.
     * (The same-day "GPIOA.11" conclusion was GPS-NMEA noise on the shared
     * UART2-RXD pad, and the old "PWM ch1 ctrl bit-1" theory was wrong --
     * the original press_detect.py note in keyboard_HD2.c had it right.) */
    return (GPIOB_EXT_PORT & PTT_BIT) == 0;
}

bool platform_pwrButtonStatus(void)
{
    /* Volume/power knob sense on GPIOB.12: LOW = on, HIGH = off
     * (live-verified 2026-05-31).  Return true while on so the OpenRTX
     * main loop keeps running; false when the knob is clicked off, which
     * triggers the shutdown path -> platform_terminate() cuts power. */
    return (GPIOB_EXT_PORT & PWR_KNOB_BIT) == 0;
}

/* HD2 LED wiring -- live-verified 2026-06-01 (re-confirmed by per-bit probe):
 *   GPIOB bit 1 (gpio pin id 0x21) = RED   (active-high)
 *   GPIOB bit 0 (gpio pin id 0x20) = GREEN (active-high)
 *   (The earlier GREEN=bit2 was WRONG -- bit2 drives nothing; the green LED
 *    is PTB0.  No DIPLEX1 mux change needed -- PTB0 is GPIO by default.)
 *   YELLOW = green + red simultaneously; WHITE -> GREEN+RED.
 */
/* GPIOB_DR/GPIOB_DDR + LED_RED_BIT/LED_GREEN_BIT/SPKR_AMP_BIT/AUDIO_ROUTE_BIT
 * are defined in hd2_regs.h. */

void platform_ledOn(led_t led)
{
    switch (led) {
        case GREEN:  GPIOB_DR |= (1u << LED_GREEN_BIT); break;
        case RED:    GPIOB_DR |= (1u << LED_RED_BIT);   break;
        case YELLOW: GPIOB_DR |= (1u << LED_GREEN_BIT) | (1u << LED_RED_BIT); break;
        case WHITE:  GPIOB_DR |= (1u << LED_GREEN_BIT) | (1u << LED_RED_BIT); break;
        default:                                                              break;
    }
}

void platform_ledOff(led_t led)
{
    switch (led) {
        case GREEN:  GPIOB_DR &= ~(1u << LED_GREEN_BIT);                       break;
        case RED:    GPIOB_DR &= ~(1u << LED_RED_BIT);                         break;
        case YELLOW: GPIOB_DR &= ~((1u << LED_GREEN_BIT) | (1u << LED_RED_BIT)); break;
        case WHITE:  GPIOB_DR &= ~((1u << LED_GREEN_BIT) | (1u << LED_RED_BIT)); break;
        default:                                                                 break;
    }
}

/* ---- speaker tone / beep output -------------------------------------
 *
 * Simple tones go through PWM channel 1 -> speaker, completely independent
 * of the HR_C7000 modem/codec PCM path (that path -- boot_init_modem_audio_path,
 * audio_codec_set_gains, modem_pcm_path_enable -- is only for digital RX
 * audio + voice-prompt PCM, and is deferred).  Ported 1:1 from V2.1.3
 * pwm_channel_start(1, freq, 5000) / pwm_channel_stop(1) at IRAM 0x03059d20 /
 * 0x03059db4 (assets/source_v213/0300d000_v2_1_3_app.c):
 *
 *   pwm_channel_start(ch=1, freq, duty=5000):
 *     socsys_io_diplex0 &= 0xfffbffff;   // clear bit 0x40000 -> UNMUTE
 *     CTRL &= ~1; &= ~4; &= ~8; &= ~0x30; |= 0x20; |= 0x100; |= 0x200;
 *     PERIOD(+0x08) = 42MHz / freq;  DUTY(+0x0c) = period * 5000 / 10000;
 *     CTRL |= 4; CTRL |= 1;           // enable + run
 *
 *   pwm_channel_stop(1):
 *     CTRL &= ~1; CTRL |= 2;          // stop
 *     socsys_io_diplex0 |= 0x40000;   // set bit -> MUTE
 *
 * The channel base for ch N is ((N + 0xa06000) * 0x20); for N=1 that is
 * 0x140c0020 -- the audio PWM channel main.c's boot_beep already uses.
 * 50% duty (5000) matches every vendor tone call (UI beeps, siren, shutdown
 * tune).  freq is in Hz, as the OpenRTX voicePrompts.c beep series expects.
 */
/* PWM_CH1_BASE (audio), PWM_TIMER_HZ, DIPLEX0_AUDIO_MUTE and
 * SOCSYS_IO_DIPLEX0 are defined in hd2_regs.h. */

/* Codec audio-output warm-up (defined in radio_HD2.cpp); brings the codec
 * DAC->lineout->speaker path up once so the PWM-ch1 beep is audible on this unit. */
void hd2_audio_out_warm(void);

/* RF-freeze flag (radio_HD2.cpp, loader op 'z').  While set, beeps must
 * not rewrite the audio GPIOs (PTB4/PTB10) or the DIPLEX0 PWM-audio mute that
 * a host-side experiment may be holding: beepStart becomes a no-op and
 * beepStop only stops the PWM channel (no GPIO/diplex rewrite). */
extern volatile uint32_t g_rf_freeze;

void platform_beepStart(uint16_t freq)
{
    if (freq == 0u)
        return;                         /* vendor guards param_2 != 0 */

    if (g_rf_freeze != 0u)
        return;                         /* rf_freeze: no audio-GPIO rewrites */

    volatile uint32_t *p = PWM_CH1_BASE;

    /* On this unit the PWM-ch1 tone is MIXED THROUGH the HR_C7000 codec (DAC ->
     * Mercury PWM lineout -> speaker), so the codec audio-output path must be up
     * before a beep is audible (proven 2026-06-01: silent until the codec was
     * brought up).  Warm it once -- no AT1846S tune, no servicing loop. */
    hd2_audio_out_warm();

    SOCSYS_IO_DIPLEX0 &= ~DIPLEX0_AUDIO_MUTE;   /* unmute PWM diplex0 path */

    /* Unmute the speaker amp (GPIOB.4 LOW) + route audio (GPIOB.10 LOW) -- boot
     * leaves both muted.  Re-asserted per beep (beepStop re-mutes). */
    GPIOB_DDR |=  (SPKR_AMP_BIT | SPKR_GAIN_BIT | AUDIO_ROUTE_BIT);
    GPIOB_DR  &= ~(SPKR_AMP_BIT | AUDIO_ROUTE_BIT);   /* PB4 + PB10 LOW     */
    GPIOB_DR  |=  SPKR_GAIN_BIT;                      /* PB17 HIGH = full gain */

    p[0] &= ~1u;
    p[0] &= ~4u;
    p[0] &= ~8u;
    p[0] &= ~0x30u;
    p[0] |=  0x20u;
    p[0] |=  0x100u;
    p[0] |=  0x200u;
    p[1] = 5u;
    p[2] = PWM_TIMER_HZ / (uint32_t)freq;         /* period */
    p[3] = (p[2] * 5000u) / 10000u;               /* 50% duty */
    p[0] |= 4u;
    p[0] |= 1u;                         /* run */
}

void platform_beepStop(void)
{
    volatile uint32_t *p = PWM_CH1_BASE;
    p[0] &= ~1u;                        /* stop */
    p[0] |=  2u;

    if (g_rf_freeze != 0u)
        return;                         /* rf_freeze: PWM stopped, but leave
                                         * PTB4 + DIPLEX0 to the host */

    gpio_atomic_set(&GPIOB_DR, SPKR_AMP_BIT);    /* GPIOB.4 HIGH = amp MUTE  */
    gpio_atomic_clear(&GPIOB_DR, SPKR_GAIN_BIT); /* PB17 LOW = gain back down */
    SOCSYS_IO_DIPLEX0 |= DIPLEX0_AUDIO_MUTE;   /* re-mute PWM diplex0 path */
}

datetime_t platform_getCurrentTime(void)
{
    return rtc_hd2_getTime();
}

void platform_setTime(datetime_t t)
{
    rtc_hd2_setTime(t);
}

const hwInfo_t *platform_getHwInfo(void)
{
    return &hwInfo;
}

const struct gpsDevice *platform_initGps(void)
{
    /* HD2-GPS: NMEA receiver on UART2 @ 0x14050000, 9600 8N1.
     * See platform/drivers/GPS/gps_HD2.c for the UART/pin-mux sourcing. */
    return gps_HD2_init();
}

void platform_vibrateOn(void)
{
}

void platform_vibrateOff(void)
{
}

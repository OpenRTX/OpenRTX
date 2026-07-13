/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * GPS driver for the Ailunce HD2 (Dahua HR_C7000, C-SKY V2 CK803S).
 *
 * The HD2's GPS module is wired to UART2 of the HR_C7000 SoC.  This is
 * derived directly from the V2.1.3 vendor firmware:
 *
 *   - gps_uart_init @ 0x0305acc0 programs the UART block at base
 *     0x14050000 (movih r3, 5125 -> 0x14050000), which per the
 *     HR_C7000 manual memory map (vendor/hr_c7000_manual.md) is UART2
 *     (0x1405_0000..0x1405_FFFF).  It registers its RX ISR at PIC
 *     vector 0x2B (43), and the manual's IRQ table maps IRQ 43 to
 *     UART2 -- two independent confirmations of the same UART.
 *
 *   - The init is called as gps_uart_init(0x2580) (= 9600) from the
 *     app startup (vendor app.c line ~113659), so the GPS link is
 *     9600 baud, 8 data bits, no parity, 1 stop bit.
 *
 *   - gps_init_and_keepalive @ 0x030423a4 muxes the UART2 RXD/TXD pins:
 *     it reads 0x11000034 (movi r6,136; lsli r6,r6,21 -> 0x11000000;
 *     ld.w (r6,0x34)), clears bit 11 (uart2_rxd_sel) and bit 12
 *     (uart2_txd_sel) to select the UART function on PTA11/PTA12, then
 *     writes it back.  Per the manual IOMUX table, diplex0 @ 0x11000034
 *     bit[11]=uart2 rxd sel (0=UART2 RXD), bit[12]=uart2 txd sel
 *     (0=UART2 TXD).  PTA11/PTA12 are shared with MODEM SPI MOSI/MISO,
 *     so muxing them to UART2 is correct only on GPS-equipped models
 *     (the HD2-GPS is one).
 *
 * The UART is a DesignWare 16550-compatible block with 32-bit,
 * word-spaced registers (see HR_C7000 manual 4.3).  This driver polls
 * the receiver (LSR Data-Ready) and feeds bytes into OpenRTX's
 * nmeaRbuf, which the core gps_task drains via getSentence().  A polled
 * design avoids wiring the CSKY V2 PIC; the core gps_task drives it on
 * the threaded build.
 */

#include "hwconfig.h"
#include "hd2_regs.h"
#include "core/gps.h"
#include "drivers/GPS/nmea_rbuf.h"
#include <stdint.h>
#include <stddef.h>

/* --- UART2 (GPS) DesignWare 16550 register block @ 0x14050000 --------
 *
 * UART2_BASE + the RBR/LSR + LCR_DLAB/LCR_8N1 + SOCSYS_IO_DIPLEX0 +
 * DIPLEX0_UART2_RXD/TXD come from hd2_regs.h.  The DLL/DLH/IER/FCR
 * registers this polled driver needs aren't in the shared header (only
 * UART0 has full accessors there), so they're built locally off the
 * shared UART2_BASE / offset constants.  Registers are 32-bit and
 * word-spaced; DLL/DLH alias RBR/IER when the LCR DLAB bit is set.
 */
#define UART_REG(off)   (*(volatile uint32_t *)(UART2_BASE + (off)))

/* --- optional GPS debug echo (HD2_GPS_DEBUG build flag) ----------------
 *
 * When built with -DHD2_GPS_DEBUG, every raw byte drained from the GPS
 * UART2 is echoed to the CONSOLE UART0 (0x14030000) -- the same port the
 * loader/diag bridge captures, so the raw NMEA stream shows up in
 * `rtx.py log` / the TUI /log.  This is the fast way to tell "no fix"
 * apart from "no bytes at all": if nothing echoes, UART2 RX / GPS power
 * (PTB15) / the DIPLEX0 RXD mux is the problem; if NMEA echoes but the
 * core reports no fix, it's antenna / sky-view / cold-start.  Off by
 * default (no overhead in a normal build). */
#ifdef HD2_GPS_DEBUG
#define GPSDBG_UART0(off)  (*(volatile uint32_t *)(0x14030000u + (off)))
static inline void gps_dbg_putc(char c)
{
    /* bounded wait for THR-empty (LSR bit5), then write THR */
    for(uint32_t g = 0; g < 200000u && (GPSDBG_UART0(0x14u) & 0x20u) == 0u; ++g) { }
    GPSDBG_UART0(0x00u) = (uint32_t)(uint8_t)c;
}
#else
static inline void gps_dbg_putc(char c) { (void)c; }
#endif

#define UART_RBR        UART_REG(UART_RBR_OFF)   /* RX buffer (read), DLL (DLAB=1) */
#define UART_THR        UART_REG(UART_THR_OFF)   /* TX holding   (write)           */
#define UART_DLL        UART_REG(UART_DLL_OFF)   /* divisor low  (DLAB=1)          */
#define UART_DLH        UART_REG(UART_DLH_OFF)   /* divisor high (DLAB=1), IER      */
#define UART_IER        UART_REG(UART_IER_OFF)   /* interrupt enable               */
#define UART_FCR        UART_REG(UART_FCR_OFF)   /* FIFO control (write)           */
#define UART_LCR        UART_REG(UART_LCR_OFF)   /* line control                   */
#define UART_LSR        UART_REG(UART_LSR_OFF)   /* line status                    */

#define FCR_ENABLE      0x67u             /* FIFO en + RX/TX reset + trig    */
                                          /* (matches vendor gps_uart_init) */
#define IER_RDA         0x01u             /* RX data-available int (unused)  */
#define LSR_DATA_READY  0x01u             /* RBR holds a received byte       */

/* --- Pin-mux: IOMUX diplex0 @ 0x11000034 -----------------------------
 *
 * Clear bit 11 (uart2_rxd_sel) and bit 12 (uart2_txd_sel) so PTA11/PTA12
 * carry the UART2 function rather than GPIO / MODEM-SPI.  See hd2_regs.h
 * for SOCSYS_IO_DIPLEX0 + DIPLEX0_UART2_RXD/TXD. */

/* Post-PLL UART input clock on the HD2 OpenRTX port.  platform_init()'s
 * clk_init_pll lands the UART clock at 42 MHz (see platform.c
 * uart_recalibrate_57600: DLL=46 gives 57600 at 42 MHz).  The vendor
 * computes the 16550 divisor as (clk >> 4) / baud == clk / (16 * baud). */
#define GPS_UART_CLK_HZ     42000000u

static struct nmeaRbuf nmea;
static bool            initialized = false;

static void gps_HD2_setBaud(uint32_t baud)
{
    uint32_t divisor = GPS_UART_CLK_HZ / (16u * baud);
    if(divisor == 0)
        divisor = 1;

    uint32_t lcr = UART_LCR;
    UART_LCR = lcr | LCR_DLAB;            /* expose DLL/DLH               */
    UART_DLL = divisor & 0xFFu;
    UART_DLH = (divisor >> 8) & 0xFFu;
    UART_LCR = LCR_8N1;                   /* DLAB cleared, 8N1            */
}

static void gps_HD2_drain(void)
{
    /* Pull every byte currently waiting in the RX FIFO into the ring
     * buffer.  nmeaRbuf_putChar runs a small FSM that only stores bytes
     * belonging to a valid NMEA sentence, so feeding it raw UART bytes
     * is safe. */
    while((UART_LSR & LSR_DATA_READY) != 0)
    {
        char c = (char)(UART_RBR & 0xFFu);
        gps_dbg_putc(c);                  /* HD2_GPS_DEBUG: echo raw NMEA to UART0 */
        nmeaRbuf_putChar(&nmea, c);
    }
}

/* u-blox UBX CFG-MSG commands the vendor sends at GPS-enable (gps_init_and_
 * keepalive @ 0x030423a4, data @ 0x0307ae24).  Each sets an NMEA sentence's
 * output rate to 0 (disable), trimming the stream to just GGA:
 *   {sync b5 62}{cls 06 id 01}{len 03 00}{NMEA-class f0, msg-id, rate 0}{ck}{cr lf}
 * msg-ids: 01=GLL 02=GSA 03=GSV 04=RMC 05=VTG.  The module streams NMEA by
 * default; these are config (trim), NOT a wake. */
static const uint8_t GPS_UBX_CFG[5][13] = {
    {0xb5,0x62,0x06,0x01,0x03,0x00,0xf0,0x02,0x00,0xfc,0x13,0x0d,0x0a}, /* GSA off */
    {0xb5,0x62,0x06,0x01,0x03,0x00,0xf0,0x03,0x00,0xfd,0x15,0x0d,0x0a}, /* GSV off */
    {0xb5,0x62,0x06,0x01,0x03,0x00,0xf0,0x01,0x00,0xfb,0x11,0x0d,0x0a}, /* GLL off */
    {0xb5,0x62,0x06,0x01,0x03,0x00,0xf0,0x04,0x00,0xfe,0x17,0x0d,0x0a}, /* RMC off */
    {0xb5,0x62,0x06,0x01,0x03,0x00,0xf0,0x05,0x00,0xff,0x19,0x0d,0x0a}, /* VTG off */
};

#define LSR_THR_EMPTY   0x20u            /* THR empty -- ready for next TX byte  */

static void gps_HD2_txByte(uint8_t b)
{
    uint32_t guard = 200000u;
    while(((UART_LSR & LSR_THR_EMPTY) == 0u) && (--guard != 0u)) { }
    UART_THR = b;
}

/* Send the 5 UBX sentence-trim commands, then LEAVE PTA11/PTA12 muxed to UART2
 * for the polling phase.  Per the HR_C7000 manual (diplex0 0x11000034): bit11
 * uart2_rxd_sel / bit12 uart2_txd_sel, value 0 = UART function, 1 = GPIO.  So
 * RXD must stay 0 for the polled gps_HD2_drain() to receive -- the previous code
 * set the bits back (|= 0x1800) after TX, which re-muxed RXD to GPIO and made
 * RX impossible.  PTA11/12 are UART2-only (no MODEM-SPI alt), so holding them on
 * UART2 for the whole GPS-enabled session is safe; gps_HD2_disable() parks them
 * back to GPIO when GPS is turned off. */
static void gps_HD2_sendConfig(void)
{
    SOCSYS_IO_DIPLEX0 &= ~(DIPLEX0_UART2_RXD | DIPLEX0_UART2_TXD);
    for(unsigned i = 0; i < 5u; ++i)
        for(unsigned j = 0; j < sizeof(GPS_UBX_CFG[0]); ++j)
            gps_HD2_txByte(GPS_UBX_CFG[i][j]);
    /* RXD/TXD left muxed to UART2 (bits 11/12 = 0) for polled reception. */
}

/* GPS module enable line. GPIOB.15 (vendor pin 0x2f) is the HD2's GPS power/
 * enable -- the GPS3V3_EN equivalent (the HD1 had a dedicated GPS3V3_EN GPIO).
 * The vendor drives it HIGH; with it LOW the module is held off and UART2 RX is
 * silent. Isolated live: B.15 high -> NMEA flows, B.15 low -> stops cold.
 * GPIOB.15 is already an output via platform_init's GPIOB_DDR (0x3d7ae51f). */
#define GPS_ENABLE_BIT  (1u << 15)        /* GPIOB.15 = vendor pin 0x2f */

/* Gated entry point: the OpenRTX core gps_task() calls this only when
 * state.settings.gps_enabled flips on (and gps_disable when off), so all GPS
 * hardware setup lives here -- init() stays passive. */
#ifdef HD2_GPS_DEBUG
static void gps_dbg_mark(const char *m) { while(*m) gps_dbg_putc(*m++); }
#else
static inline void gps_dbg_mark(const char *m) { (void)m; }
#endif

static void gps_HD2_enable(void *priv)
{
    (void) priv;

    /* Staged markers (HD2_GPS_DEBUG): enabling GPS has been observed to lock
     * the radio; the LAST marker that reaches the console UART0 (bridge log)
     * identifies which step wedges -- the hang is in the step after it. */
    gps_dbg_mark("\r\n[GPS:enter]\r\n");

    GPIOB_DR |= GPS_ENABLE_BIT;           /* power/enable the GPS module  */
    gps_dbg_mark("[GPS:pwr]\r\n");

    UART_FCR = FCR_ENABLE;                /* FIFO on, reset RX/TX        */
    UART_IER = 0;                         /* polled: no UART interrupts  */
    gps_HD2_setBaud(9600u);               /* vendor: gps_uart_init(0x2580) */
    gps_dbg_mark("[GPS:uart]\r\n");

    gps_HD2_sendConfig();                 /* UBX sentence-trim + leave RX-muxed */
    gps_dbg_mark("[GPS:cfg]\r\n");

    nmeaRbuf_reset(&nmea);
    gps_HD2_drain();                      /* flush any stale FIFO bytes  */
    gps_dbg_mark("[GPS:ready]\r\n");
}

static void gps_HD2_disable(void *priv)
{
    (void) priv;

    GPIOB_DR &= ~GPS_ENABLE_BIT;          /* power the GPS module back off */

    /* Park the shared pins back as GPIO (bits set), matching the
     * vendor's idle state for diplex0 bits 11/12. */
    SOCSYS_IO_DIPLEX0 |= (DIPLEX0_UART2_RXD | DIPLEX0_UART2_TXD);
}

static int gps_HD2_getSentence(void *priv, char *buf, const size_t bufSize)
{
    (void) priv;

    if(!initialized)
        return 0;

    /* Move any pending UART bytes into the ring, then hand the core a
     * complete sentence if one is ready.  Both calls are non-blocking. */
    gps_HD2_drain();

    return nmeaRbuf_getSentence(&nmea, buf, bufSize);
}

/*
 * Passive init at platform_initGps() time: only resets the ring buffer and
 * returns the device handle.  All UART/pin-mux/module setup is deferred to
 * gps_HD2_enable(), which the core gps_task() calls ONLY when the user's
 * GPS-enabled setting (state.settings.gps_enabled) is on.  So a GPS-off (or
 * non-GPS) radio never touches the UART2 pins (shared with MODEM SPI).
 */
const struct gpsDevice *gps_HD2_init(void)
{
    if(!initialized)
    {
        nmeaRbuf_reset(&nmea);
        initialized = true;
    }

    static const struct gpsDevice dev = {
        .priv       = NULL,
        .enable     = gps_HD2_enable,
        .disable    = gps_HD2_disable,
        .getSentence = gps_HD2_getSentence,
    };

    return &dev;
}

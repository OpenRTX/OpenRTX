/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * I2C0 transport for the HD2 (ck803s + HR_C7000) -- the bus the AT1846S RF
 * transceiver (slave 0xE2) and the RDA5802E broadcast tuner (slave 0x20) sit
 * on.  SCL=PTA7, SDA=PTA8.
 *
 * HARDWARE I2C (default, 2026-06-12): PTA7/PTA8 are the pins of the on-chip
 * DesignWare I2C1 controller (base 0x14070000); the SoC pad mux IO_DIPLEX0
 * bits 7/8 (i2c1_scl_sel/i2c1_sda_sel) select GPIO (=1, bit-bang) vs the I2C1
 * function (=0).  The vendor firmware bit-bangs these pins as GPIO, but the
 * HW controller works on them -- LIVE-VERIFIED: an HW-I2C1 read of AT1846S
 * regs 0x00/0x04/0x0a/0x33 returned 0x1846/0x0fd0/0x4c20/0x44a5, byte-for-byte
 * identical to the bit-bang read (reg 0x00 == 0x1846 is the AT1846S chip ID).
 *
 * WHY: the bit-bang path is ~hundreds of us/transfer (delayUs(5) per edge) and
 * was the prime contention/lockup source -- a 33 Hz RSSI poll over it, shared
 * with TX keying and diag pokes, dominated CPU and stalled the bus owner.  The
 * HW controller does the same transfer in microseconds with a FIFO and no
 * IRQ-monopolising busy spins.  See docs/threading_redesign.md.
 *
 * The controller config mirrors the proven RTC driver (rtc_hd2.c, DesignWare
 * I2C2 @ 0x14080000): IC_CON=0x65 (master/7-bit/FS/restart), 400 kHz off the
 * 42 MHz APB clock, and the HR_C7000-specific IC_START (+0xa0) trigger that
 * must be written after loading the FIFO (a non-stock DW quirk -- stock DW
 * starts implicitly; this part waits for IC_START).
 *
 * The i2c0 device lock (pthread_mutex below) serialises every transfer at the
 * single chokepoint all callers funnel through (AT1846S RSSI poll, TX keying,
 * RDA5802E, diag q/Q).  Build-portable: the bare loader links no-op pthread
 * stubs.
 */

#include "i2c_csky.h"
#include "hd2_regs.h"

#include "interfaces/delays.h"
#include <pthread.h>

/* IO_DIPLEX0 pad-mux bits for the I2C1 SCL/SDA pins (PTA7/PTA8).
 * 1 = GPIO (bit-bang), 0 = I2C_SCL_1/I2C_SDA_1 (HW controller). */
#define DIPLEX0_I2C1_SCL_SEL  (1u << 7)
#define DIPLEX0_I2C1_SDA_SEL  (1u << 8)
#define DIPLEX0_I2C1_PINS     (DIPLEX0_I2C1_SCL_SEL | DIPLEX0_I2C1_SDA_SEL)

/* ===================================================================== *
 *  Hardware DesignWare I2C1 controller @ 0x14070000
 * ===================================================================== */

#define I2C1_BASE        0x14070000u
#define I2C1_REG(off)    (*(volatile uint32_t *)(I2C1_BASE + (off)))
#define IC_CON           I2C1_REG(0x00u)   /* master cfg (0x65) */
#define IC_TAR           I2C1_REG(0x04u)   /* target slave 7-bit address */
#define IC_DATA_CMD      I2C1_REG(0x10u)   /* tx data byte / rx command */
#define IC_FS_SCL_HCNT   I2C1_REG(0x1cu)
#define IC_FS_SCL_LCNT   I2C1_REG(0x20u)
#define IC_INTR_MASK     I2C1_REG(0x30u)
#define IC_RX_TL         I2C1_REG(0x38u)
#define IC_TX_TL         I2C1_REG(0x3cu)
#define IC_CLR_TX_ABRT   I2C1_REG(0x54u)   /* read clears the TX_ABRT latch */
#define IC_ENABLE        I2C1_REG(0x6cu)
#define IC_STATUS        I2C1_REG(0x70u)
#define IC_TX_ABRT_SRC   I2C1_REG(0x80u)   /* nonzero = abort latched (NAK/arb-loss) */
#define IC_ENABLE_STATUS I2C1_REG(0x9cu)   /* bit0: IC_EN settled */
#define IC_START         I2C1_REG(0xa0u)   /* HR_C7000-specific transfer trigger */

#define CMD_READ         0x100u            /* IC_DATA_CMD bit8: read request */
#define CMD_STOP         0x200u            /* IC_DATA_CMD bit9: STOP after byte */

#define STA_ACTIVITY     (1u << 0)         /* master/bus activity (1 = busy) */
#define STA_TFNF         (1u << 1)         /* tx FIFO not full  */
#define STA_TFE          (1u << 2)         /* tx FIFO empty     */
#define STA_RFNE         (1u << 3)         /* rx FIFO not empty */

/* Spin until the controller reports the bus idle (ACTIVITY clear).  TFE only
 * means the tx FIFO drained, NOT that the last byte finished shifting out / the
 * STOP completed -- ending the transaction (IC_START=0 / IC_ENABLE=0) before
 * the bus is idle wedges the controller.  At host-paced MMIO this settled for
 * free; at firmware speed it does not.  Bounded so a stuck bus can't hang. */
#define I2C1_WAIT_IDLE() do { \
        int _s; for (_s = I2C1_SPIN_LIMIT; _s && (IC_STATUS & STA_ACTIVITY); --_s) { } \
    } while (0)

/* 400 kHz off the 42 MHz APB clock (matches rtc_hd2.c): clk/400000 = 105,
 * HCNT = 105/2 - 8 = 44 (0x2c), LCNT = 105-1-52 = 52 (0x34). */
#define I2C1_SCL_HCNT    0x2cu
#define I2C1_SCL_LCNT    0x34u

/* Bounded poll limit.  Sized to fail FAST: the longest transfer (reg-ptr +
 * RESTART + 2 read bytes, ~7 byte-times @400 kHz) is ~200 us =~ 1k poll
 * iterations, so 0x2000 gives ~8x margin while a dead bus costs ~ms, not the
 * multi-second CPU burns the old 0x40000 caused (each exhausted spin starved
 * every other thread -- the visible "system wedge" when the pad mux was
 * stolen from the controller, 2026-06-12). */
#define I2C1_SPIN_LIMIT  0x2000

/* Pending no-STOP write bytes (the register pointer of an AT1846S read).
 * i2c0_read flushes these as the write phase of a repeated-START read.
 * Single-threaded under the i2c0 mutex, so static state is safe. */
static uint8_t  s_pend[4];
static size_t   s_pendLen;
static uint16_t s_curTar = 0xffffu;        /* cached IC_TAR (0xffff = none) */

/* Post-transfer health check + recovery.  A completed transfer leaves the TX
 * FIFO empty, the bus idle and no abort latched; anything else means THIS
 * transfer failed (unresponsive slave, arb-loss, pad mux stolen from the
 * controller).  A failed transfer leaves its command bytes rotting in the TX
 * FIFO -- NO abort is raised for the mux-stolen case (LIVE-VERIFIED
 * 2026-06-12: stale TXFLR, TX_ABRT_SRC=0) -- and they corrupt every transfer
 * after it.  Recovery: clear any abort latch, disable the controller (flushes
 * both FIFOs) and drop the TAR cache so the next transfer re-enables from
 * scratch.  Turns a bus fault into ONE failed transfer instead of a
 * permanently poisoned controller. */
static void i2c1_check_recover(void)
{
    if ((IC_STATUS & STA_TFE) && !(IC_STATUS & STA_ACTIVITY) &&
        (IC_TX_ABRT_SRC == 0u))
        return;                            /* healthy */

    (void)IC_CLR_TX_ABRT;                  /* read-to-clear the abort latch */
    IC_ENABLE = 0u;                        /* flush TX+RX FIFOs */
    s_curTar  = 0xffffu;                   /* force full re-enable next xfer */
}

void i2c0_init(void)
{
    /* Configure the I2C1 master, then route PTA7/PTA8 to the controller. */
    IC_ENABLE      = 0u;                   /* off while reconfiguring */
    IC_CON         = 0x65u;                /* master, 7-bit, FS, restart */
    IC_FS_SCL_HCNT = I2C1_SCL_HCNT;
    IC_FS_SCL_LCNT = I2C1_SCL_LCNT;
    IC_INTR_MASK   = 0x40u;
    IC_RX_TL       = 6u;
    IC_TX_TL       = 6u;
    s_curTar       = 0xffffu;              /* force a target set on first xfer */

    /* Pad mux: clear IO_DIPLEX0 bits 7/8 -> PTA7/PTA8 carry I2C_SCL_1/SDA_1
     * (RMW; the rest of DIPLEX0 -- UART2 sel, audio mute, etc. -- is left
     * untouched).  Pull-ups + input-enable on these pads default on. */
    SOCSYS_IO_DIPLEX0 &= ~DIPLEX0_I2C1_PINS;

    s_pendLen = 0u;
}

void i2c0_terminate(void)
{
    IC_ENABLE = 0u;
    s_curTar  = 0xffffu;
    /* Park the pins back as GPIO (mux reset default). */
    SOCSYS_IO_DIPLEX0 |= DIPLEX0_I2C1_PINS;
}

/* Point the controller at a slave, ENABLING IT ONCE and leaving it enabled.
 * IC_TAR can only change while IC_ENABLE=0, so we toggle enable ONLY when the
 * target actually changes (AT1846S 0x71 vs RDA5802E 0x10).  For the common
 * single-target case (the whole radio bring-up + RSSI poll) the controller is
 * enabled once and never toggled -- matching the host-verified sequence.
 * (Toggling IC_ENABLE every transfer, as the first cut did, was a wedge risk;
 * and the controller must be enabled with the bus idle, confirmed via
 * IC_ENABLE_STATUS, before loading the FIFO.) */
static void i2c1_set_target(uint8_t addr8)
{
    uint16_t tar = (uint16_t)(addr8 >> 1);
    int spin;

    if (tar == s_curTar && (IC_ENABLE_STATUS & 1u))
        return;

    IC_ENABLE = 0u;
    for (spin = I2C1_SPIN_LIMIT; spin && (IC_ENABLE_STATUS & 1u); --spin) { }
    IC_TAR    = (uint32_t)tar;
    IC_ENABLE = 1u;
    for (spin = I2C1_SPIN_LIMIT; spin && !(IC_ENABLE_STATUS & 1u); --spin) { }
    s_curTar  = tar;
}

/* Issue a transfer: optional write bytes (wbuf/wlen) followed by optional
 * read bytes into rbuf/rlen, as a single START..STOP (repeated-START between
 * the write and read phases).  All commands are loaded into the FIFO BEFORE
 * IC_START -- the HR_C7000 IC_START quirk transfers whatever is loaded, so
 * starting early (mid-load) fragments the transfer.  wlen+rlen must fit the
 * FIFO (it does: AT1846S reg-ptr + 2 read bytes). */
static void i2c1_xfer(uint8_t addr8, const uint8_t *wbuf, size_t wlen,
                      uint8_t *rbuf, size_t rlen)
{
    int spin;

    i2c1_set_target(addr8);

    for (size_t i = 0; i < wlen; ++i)
    {
        for (spin = I2C1_SPIN_LIMIT; spin && !(IC_STATUS & STA_TFNF); --spin) { }
        IC_DATA_CMD = wbuf[i];
    }
    for (size_t i = 0; i < rlen; ++i)
    {
        for (spin = I2C1_SPIN_LIMIT; spin && !(IC_STATUS & STA_TFNF); --spin) { }
        IC_DATA_CMD = CMD_READ | ((i == rlen - 1u) ? CMD_STOP : 0u);
    }

    IC_START = 1u;             /* now transfer the fully-loaded FIFO */

    for (size_t i = 0; i < rlen; ++i)
    {
        for (spin = I2C1_SPIN_LIMIT; spin && !(IC_STATUS & STA_RFNE); --spin) { }
        rbuf[i] = (uint8_t)IC_DATA_CMD;
    }
    if (rlen == 0u)
        for (spin = I2C1_SPIN_LIMIT; spin && !(IC_STATUS & STA_TFE); --spin) { }

    I2C1_WAIT_IDLE();          /* let the STOP/last byte finish on the wire */
    IC_START = 0u;             /* leave IC_ENABLE on (see i2c1_set_target) */
    i2c1_check_recover();
}

void i2c0_write(uint8_t addr, void *buf, size_t len, bool sendStop)
{
    const uint8_t *p = (const uint8_t *)buf;

    if (!sendStop)
    {
        /* Stage the register pointer; the following i2c0_read flushes it as
         * the write phase of a repeated-START read (the AT1846S read pattern).
         * The slave address is not staged: i2c0_read receives it again. */
        (void)addr;
        s_pendLen  = (len > sizeof(s_pend)) ? sizeof(s_pend) : len;
        for (size_t i = 0; i < s_pendLen; ++i) s_pend[i] = p[i];
        return;
    }

    /* STOP-terminated write.  Load the FIFO, STOP on the last byte, and assert
     * IC_START only AFTER the whole write is loaded (HOST-VERIFIED: starting
     * mid-load fragments the write -- the AT1846S init silently did nothing,
     * the root-cause of the HW-I2C wedge).  For a write longer than the FIFO
     * (the RDA5802E 64-byte init), start once the FIFO fills so it drains
     * while we keep feeding. */
    int spin;
    bool started = false;
    i2c1_set_target(addr);
    for (size_t i = 0; i < len; ++i)
    {
        if (!started && !(IC_STATUS & STA_TFNF))   /* FIFO full -> must start to drain */
        {
            IC_START = 1u;
            started  = true;
        }
        for (spin = I2C1_SPIN_LIMIT; spin && !(IC_STATUS & STA_TFNF); --spin) { }
        IC_DATA_CMD = (uint32_t)p[i] | ((i == len - 1u) ? CMD_STOP : 0u);
    }
    if (!started) IC_START = 1u;                   /* whole write fit: start now */

    for (spin = I2C1_SPIN_LIMIT; spin && !(IC_STATUS & STA_TFE); --spin) { }
    I2C1_WAIT_IDLE();          /* let the STOP/last byte finish on the wire */
    IC_START  = 0u;            /* leave IC_ENABLE on (see i2c1_set_target) */
    i2c1_check_recover();
    s_pendLen = 0u;
}

void i2c0_read(uint8_t addr, void *buf, size_t len)
{
    uint8_t *p = (uint8_t *)buf;

    /* Combined repeated-START read: flush any staged write (the register
     * pointer) then read `len` bytes, in one START..STOP. */
    if (s_pendLen > 0u)
    {
        i2c1_xfer(addr, s_pend, s_pendLen, p, len);
        s_pendLen = 0u;
    }
    else
    {
        i2c1_xfer(addr, NULL, 0u, p, len);
    }
}

bool i2c0_probe(uint8_t addr)
{
    /* A 1-byte read; success is non-wedged completion. (The AT1846S/RDA5802E
     * are the only devices; a finer ACK check isn't needed for our use.) */
    uint8_t v;
    i2c1_xfer(addr, NULL, 0u, &v, 1u);
    return true;
}

/* ===================================================================== *
 *  I2C0 device lock -- serialises all transfers (one bus owner)
 * ===================================================================== */
static pthread_mutex_t i2c0_mutex = PTHREAD_MUTEX_INITIALIZER;

void i2c0_lockDeviceBlocking(void) { pthread_mutex_lock(&i2c0_mutex);   }
void i2c0_releaseDevice(void)      { pthread_mutex_unlock(&i2c0_mutex); }

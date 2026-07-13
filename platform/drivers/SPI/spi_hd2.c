/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Software bitbang SPI master for the HD2's W25Q512 SPI-NOR flash bus.
 *
 * Pin map -- ALL on GPIOA (DesignWare ahb_gpio, base 0x14020000).
 * Live-verified 2026-05-30 (JEDEC = ef4020) and corroborated by the
 * vendor V2.1.3 decomp of flash_spi_send_byte @ 0x030575f4,
 * flash_spi_recv_bytes @ 0x03057598 and flash_probe_jedec_id @ 0x03057790
 * (Ghidra labels these GPIOA accesses with the misleading GBR-relative
 * symbol `_g_lcd_bus_mmio_latch`, but the base is 0x14020000, not the
 * LCD bank at 0x14110000):
 *
 *    bit 18  CS#   (PTA18, active-low)  -- driven EXTERNALLY, see note
 *    bit 20  SCK   (PTA20)
 *    bit 21  MOSI  (PTA21)
 *    bit 22  MISO  (PTA22, sampled via EXT_PORT / input reg at +0x50)
 *
 * Mode: CPOL=0, CPHA=0, MSB-first.  MOSI is presented while SCK is low,
 * MISO is sampled on the SCK rising edge, SCK is parked low between
 * bytes -- this exactly mirrors the proven host-side reference in
 * scripts/probe_init.py (spi_xfer_byte).
 *
 * CS ownership: the chip-select line (bit 18) is NOT toggled by the
 * per-byte transfer path here.  W25Qx.c (and the loader's 'J' handler)
 * own CS through the portable gpioPin vtable bound to hd2_gpioa_dev /
 * EXT_FLASH_CS_BIT, so that the W25Qx chip driver stays target-agnostic.
 * Both this driver and the gpio_csky CS path read-modify-write the same
 * GPIOA DR; every access here is RMW off the live register value so the
 * externally-managed CS bit (and the unrelated peripheral bits 5..8 the
 * vendor keeps as outputs) are preserved untouched.
 *
 * The standalone self-test helpers at the bottom (spi_hd2_wakeup /
 * spi_hd2_read_jedec) DO drive CS themselves, mirroring the vendor
 * primitives, so they can be called independently of W25Qx.c for
 * bring-up probing.  They are not on the nvm_spi data path.
 *
 * Boot-time safety: a prior GPIOA build regressed the flash into a
 * persistent "silent" state, suspected to be garbage clocks / wrong pin
 * drive at boot.  spi_hd2_init() therefore establishes the idle line
 * state (CS=1 high, SCK=0, MOSI=0) BEFORE switching the pins to outputs,
 * and never emits a clock edge during init.
 */

#include "drivers/SPI/spi_hd2.h"
#include <stdint.h>
#include <stddef.h>

#define GPIOA_BASE   0x14020000u
#define GPIOA_DR     (*(volatile uint32_t *)(GPIOA_BASE + 0x00u))
#define GPIOA_DDR    (*(volatile uint32_t *)(GPIOA_BASE + 0x04u))
#define GPIOA_DIN    (*(volatile uint32_t *)(GPIOA_BASE + 0x50u))   /* EXT_PORT */

#define SPI_CS_BIT   (1u << 18)
#define SPI_SCK_BIT  (1u << 20)
#define SPI_MOSI_BIT (1u << 21)
#define SPI_MISO_BIT (1u << 22)

/* CSKY V2 memory barrier -- forces the store buffer to drain so a
 * subsequent MMIO read sees the most recent write.  Without this the
 * bitbang at MCU clock speed silently drops back-to-back GPIOA DR
 * writes: xfer_byte runs but every received bit reads 0 because the
 * MOSI/SCK transitions never reach the pins. */
#define MMIO_BARRIER()  __asm__ volatile ("sync" ::: "memory")

/* Half-period delay between MOSI/SCK transitions, expressed as a busy-loop
 * iteration count.  HOST-TUNABLE: g_spi_half_period is a plain RAM global
 * the loader can poke via the 'W' command (write its .map address) so we
 * can sweep the bitbang clock speed at runtime and find the value at which
 * the W25Q answers, WITHOUT reflashing per experiment.  Default 20.
 * NOTE: firmware bitbang at the compiled-in speed reads MISO flat-0 while
 * the ~1000x slower host-side bitbang reads ef4020 on the same boot, so
 * clock speed is the prime suspect -- this knob is the sweep handle. */
volatile uint32_t g_spi_half_period = 20;

static inline void spi_half_period(void)
{
    for (volatile uint32_t i = 0; i < g_spi_half_period; ++i) { }
}

/*
 * Transfer one byte, MSB-first, SPI mode 0.  Per bit:
 *   1. drive MOSI with SCK low (present data on the trailing edge),
 *   2. raise SCK -> the chip samples MOSI and we sample MISO,
 *   3. (next iteration drops SCK again).
 * SCK is parked low on exit.  CS is left untouched (owner: caller).
 *
 * Every write is read-modify-write off the live DR so the CS bit and
 * the other peripheral output bits are preserved.
 */
#if defined(HD2_SPI_BITBANG) && (HD2_SPI_BITBANG)
static uint8_t xfer_byte(uint8_t out)
{
    uint8_t in = 0;

    for (int i = 0; i < 8; ++i) {
        /* SCK low, present MOSI for this bit. */
        uint32_t v = GPIOA_DR & ~(SPI_SCK_BIT | SPI_MOSI_BIT);
        if (out & 0x80u) v |= SPI_MOSI_BIT;
        GPIOA_DR = v;
        MMIO_BARRIER();
        spi_half_period();

        /* SCK rising edge -- sample MISO here. */
        GPIOA_DR = v | SPI_SCK_BIT;
        MMIO_BARRIER();
        spi_half_period();

        in <<= 1;
        if (GPIOA_DIN & SPI_MISO_BIT) in |= 1u;

        out <<= 1;
    }

    /* Park SCK low between bytes. */
    GPIOA_DR &= ~SPI_SCK_BIT;
    MMIO_BARRIER();

    return in;
}

#else
/*
 * HW SPI0 controller @ 0x140A_0000 (manual 5.2; default transport,
 * LIVE-VERIFIED 2026-06-13: full-duplex JEDEC read returned ef 40 20).
 * A DW-SSI-style block (custom variant: IDR reads 0x12345666, and the
 * EEPROM-read TMOD misbehaves -- the chip's reply died after one byte --
 * so we use plain full-duplex TX&RX, which behaves stock).  SCLK/MOSI/MISO
 * are muxed to the controller by platform_init (DIPLEX1, build-gated
 * constant); CS stays a GPIO (PTA18) owned by the caller exactly like the
 * bit-bang -- this sidesteps the classic DW auto-CS deassert-on-FIFO-empty
 * problem and keeps W25Qx.c's CS ownership unchanged.  SCLK gaps between
 * frames are harmless to the W25Q while CS is held low.
 */
#define SSI_BASE     0x140A0000u
#define SSI_REG(off) (*(volatile uint32_t *)(SSI_BASE + (off)))
#define SSI_CTRLR0   SSI_REG(0x00u)
#define SSI_SSIENR   SSI_REG(0x08u)
#define SSI_SER      SSI_REG(0x10u)
#define SSI_BAUDR    SSI_REG(0x14u)
#define SSI_RXFLR    SSI_REG(0x24u)
#define SSI_SR       SSI_REG(0x28u)
#define SSI_IMR      SSI_REG(0x2cu)
#define SSI_DR       SSI_REG(0x60u)

#define SSI_SR_BUSY  (1u << 0)
/* TX FIFO depth: LIVE-MEASURED 8 (2026-06-13: SER=0, pushed 16 frames,
 * TXFLR pegged at 8 with TFNF=0).  The manual's "128-deep master FIFOs"
 * (5.2.4) does NOT hold for this custom variant -- bursting 128 silently
 * dropped 120 of every 128 frames, which broke every transfer longer than
 * 8 bytes (settings block load/save, codeplug bulk reads) while leaving
 * <=8-byte ops (JEDEC gate, status polls) working.  Burst in chunks of 8. */
#define SSI_FIFO_DEPTH 8u
#define SSI_SPIN_LIMIT 0x8000  /* bounded poll; a byte at 10.5 MHz is ~1 us */

static uint8_t xfer_byte(uint8_t out)
{
    int spin;
    SSI_DR = out;
    for (spin = SSI_SPIN_LIMIT; spin && (SSI_RXFLR == 0u); --spin) { }
    return (uint8_t)SSI_DR;
}
#endif /* HD2_SPI_BITBANG */

static int hd2_transfer(const struct spiDevice *dev, const void *txBuf,
                        void *rxBuf, const size_t size)
{
    (void)dev;
    const uint8_t *tx = (const uint8_t *)txBuf;
    uint8_t       *rx = (uint8_t *)rxBuf;

#if defined(HD2_SPI_BITBANG) && (HD2_SPI_BITBANG)
    for (size_t i = 0; i < size; ++i) {
        uint8_t out = tx ? tx[i] : 0xffu;
        uint8_t in  = xfer_byte(out);
        if (rx) rx[i] = in;
    }
#else
    /* Burst in SSI_FIFO_DEPTH chunks (8 frames -- the live-measured depth,
     * see above): load a chunk of TX frames, then drain the same number of
     * RX frames.  The controller clocks frames back-to-back while the FIFO
     * has data -- bulk codeplug reads run at the full SCLK rate instead of
     * one poll round-trip per byte. */
    size_t done = 0;
    while (done < size) {
        size_t chunk = size - done;
        if (chunk > SSI_FIFO_DEPTH) chunk = SSI_FIFO_DEPTH;

        for (size_t i = 0; i < chunk; ++i)
            SSI_DR = tx ? tx[done + i] : 0xffu;

        int spin;
        for (spin = SSI_SPIN_LIMIT; spin && (SSI_RXFLR < chunk); --spin) { }

        for (size_t i = 0; i < chunk; ++i) {
            uint8_t in = (uint8_t)SSI_DR;
            if (rx) rx[done + i] = in;
        }
        done += chunk;
    }
#endif

    return 0;
}

const struct spiDevice nvm_spi =
{
    .transfer = hd2_transfer,
    .priv     = NULL,
    .mutex    = NULL,
};

void spi_hd2_init(void)
{
    /* Establish the idle line state BEFORE the pins become outputs, so
     * flipping DDR can never emit a stray clock edge: CS high (idle,
     * deasserted), SCK low, MOSI low.  RMW preserves every other bit.
     * (HW build: CS is still this GPIO bit; SCK/MOSI bits are muxed away
     * to the controller, so their DR/DDR state is don't-care but harmless.) */
    GPIOA_DR = (GPIOA_DR & ~(SPI_SCK_BIT | SPI_MOSI_BIT)) | SPI_CS_BIT;
    MMIO_BARRIER();

    /* CS/SCK/MOSI -> outputs, MISO -> input.  RMW so the vendor's other
     * GPIOA output bits (5..8, etc.) are left as-is. */
    GPIOA_DDR = (GPIOA_DDR | (SPI_CS_BIT | SPI_SCK_BIT | SPI_MOSI_BIT))
                & ~SPI_MISO_BIT;
    MMIO_BARRIER();

#if !defined(HD2_SPI_BITBANG) || !(HD2_SPI_BITBANG)
    /* HW SPI0: 8-bit frames, SPI mode 0, full-duplex; 42 MHz / 4 = 10.5 MHz
     * SCLK (W25Q512 is good past 50 MHz -- raise later if wanted).  Polled,
     * no interrupts.  SER=1 starts transfers on FIFO data; the controller's
     * own CSN pads stay muxed as GPIO (CS ownership unchanged, see above). */
    SSI_SSIENR = 0u;
    SSI_CTRLR0 = 7u;            /* TMOD=00 full duplex, DFS=7 (8-bit), mode 0 */
    SSI_BAUDR  = 4u;
    SSI_IMR    = 0u;
    SSI_SER    = 1u;
    SSI_SSIENR = 1u;
#endif
}

/* --------------------------------------------------------------------- *
 * Standalone bring-up / self-test helpers.
 *
 * These drive CS themselves (RMW on bit 18) and are NOT part of the
 * nvm_spi data path -- they exist so the loader can probe the bus without
 * pulling in W25Qx.c.  They mirror the vendor's flash_probe_jedec_id
 * (CS-assert, opcode, recv, CS-deassert) and its 0xAB wakeup.
 * --------------------------------------------------------------------- */

static inline void cs_assert(void)   /* active-low: clear bit 18 */
{
    GPIOA_DR &= ~SPI_CS_BIT;
    MMIO_BARRIER();
}

static inline void cs_deassert(void) /* idle: set bit 18 */
{
    GPIOA_DR |= SPI_CS_BIT;
    MMIO_BARRIER();
}

/* Crude busy-wait, ~milliseconds at ~42 MHz.  Used only for the chip's
 * tRES2 settle after a Release-from-Deep-Power-Down; not timing-critical
 * as long as it's comfortably over the spec'd ~3 us. */
static void spi_delay_ms(unsigned ms)
{
    /* ~ (loops/ms) tuned conservatively long -- this path is not hot. */
    for (unsigned m = 0; m < ms; ++m)
        for (volatile unsigned i = 0; i < 8000u; ++i) { }
}

void spi_hd2_wakeup(void)
{
    /* W25Q Release from Deep Power-Down (0xAB).  The chip can come up in
     * DPD; issue this once with a settle delay before the first read.
     * Mirrors scripts/probe_init.py::spi_wakeup. */
    cs_assert();
    xfer_byte(0xABu);
    cs_deassert();
    spi_delay_ms(2);            /* tRES2 ~= 3 us; 2 ms is ample margin */
}

void spi_hd2_read_jedec(uint8_t id[3])
{
    /* JEDEC-ID (0x9F): assert CS, send opcode, clock 3 bytes, deassert.
     * Wakeup first in case the chip is parked in DPD.
     * Mirrors scripts/probe_init.py::spi_jedec and vendor
     * flash_probe_jedec_id @ 0x03057790. */
    spi_hd2_wakeup();

    cs_assert();
    xfer_byte(0x9Fu);
    id[0] = xfer_byte(0xffu);
    id[1] = xfer_byte(0xffu);
    id[2] = xfer_byte(0xffu);
    cs_deassert();
}

void spi_hd2_jedec_trace(uint32_t lo[32], uint32_t hi[32])
{
    /* DIAGNOSTIC: full JEDEC transaction capture.  Records the GPIOA
     * EXT_PORT at SCK-low and SCK-high for ALL 32 clocked bits:
     *   bits  0..7  = the 0x9F command phase  (MOSI should follow
     *                 0x9F = 1,0,0,1,1,1,1,1 -- this is the phase the
     *                 old 24-bit trace did NOT instrument)
     *   bits  8..31 = the 3 read bytes        (MOSI held 1, MISO = data)
     * Lets the host verify the command MOSI actually toggles at the pad
     * and whether MISO ever responds.  CS (bit18) should read 0 throughout.
     * Drives MOSI inline (not via xfer_byte) so the captured bit pattern
     * is exactly what reaches the pin. */
    uint32_t outbits = ((uint32_t)0x9Fu << 24) | 0x00FFFFFFu; /* MSB-first */
    spi_hd2_wakeup();
    cs_assert();
    for (int i = 0; i < 32; ++i) {
        int bit = (int)((outbits >> (31 - i)) & 1u);
        uint32_t v = GPIOA_DR & ~(SPI_SCK_BIT | SPI_MOSI_BIT);
        if (bit) v |= SPI_MOSI_BIT;
        GPIOA_DR = v;                       /* SCK low, MOSI = this bit */
        MMIO_BARRIER();
        spi_half_period();
        lo[i] = GPIOA_DIN;
        GPIOA_DR = v | SPI_SCK_BIT;         /* SCK rising edge */
        MMIO_BARRIER();
        spi_half_period();
        hi[i] = GPIOA_DIN;                  /* sample point */
    }
    GPIOA_DR &= ~SPI_SCK_BIT;
    MMIO_BARRIER();
    cs_deassert();
}

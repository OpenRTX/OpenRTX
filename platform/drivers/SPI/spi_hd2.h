/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Software bitbang SPI driver for the HD2's W25Q512 SPI-NOR flash.
 *
 * Bus lives on GPIOA (base 0x14020000): SCK=bit 20, MOSI=bit 21,
 * MISO=bit 22 (sampled via the EXT_PORT input register at +0x50).
 * CS# (GPIOA bit 18, active-low) is NOT toggled by the per-byte transfer
 * path -- W25Qx.c drives CS via the portable gpioPin abstraction
 * (hd2_gpioa_dev / EXT_FLASH_CS_BIT) so the chip driver stays
 * target-portable.
 *
 * Mode is CPOL=0/CPHA=0, MSB-first: clock idles low, MOSI is presented
 * while SCK is low, data is sampled on the SCK rising edge.  Live-verified
 * 2026-05-30 (JEDEC = ef4020).  Vendor V2.1.3 reference primitives:
 * flash_spi_send_byte @ 0x030575f4, flash_spi_recv_bytes @ 0x03057598,
 * flash_probe_jedec_id @ 0x03057790.
 */

#ifndef SPI_HD2_H
#define SPI_HD2_H

#include "peripherals/spi.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SPI device driver and config for the W25Q NVM flash on the HD2.
 * Defined in spi_hd2.c.  CS is managed by the caller (W25Qx.c).
 */
extern const struct spiDevice nvm_spi;

/**
 * Configure the GPIOA pin directions for the SPI flash lines and park
 * the bus in its idle state (CS high, SCK low, MOSI low) before the chip
 * driver takes over.  Establishes the idle DR value BEFORE switching the
 * pins to outputs so no stray clock edge is emitted at boot.  Uses
 * read-modify-write throughout so unrelated GPIOA bits are preserved.
 * Call once before W25Qx_init().
 */
void spi_hd2_init(void);

/**
 * Standalone Release-from-Deep-Power-Down (W25Q opcode 0xAB) with a
 * tRES2 settle delay.  Drives CS itself; independent of W25Qx.c.  Useful
 * for bring-up: the chip can power up in DPD.
 */
void spi_hd2_wakeup(void);

/**
 * Standalone JEDEC-ID read (opcode 0x9F) into a 3-byte buffer
 * (manufacturer, memory type, capacity; expect ef/40/20 for the
 * W25Q512).  Issues a 0xAB wakeup first.  Drives CS itself; independent
 * of the nvm_spi data path and of W25Qx.c.
 */
void spi_hd2_read_jedec(uint8_t id[3]);

/**
 * Diagnostic JEDEC read: captures the full GPIOA EXT_PORT (input
 * register at +0x50) at SCK-low and SCK-high for each of the 24 receive
 * bits, into lo[24]/hi[24].  Lets the host see per-bit whether SCK
 * (bit 20) / MOSI (bit 21) toggle at the pad and whether MISO (bit 22)
 * ever moves.  Drives CS itself; for bring-up debugging only.
 */
void spi_hd2_jedec_trace(uint32_t lo[32], uint32_t hi[32]);

#ifdef __cplusplus
}
#endif

#endif /* SPI_HD2_H */

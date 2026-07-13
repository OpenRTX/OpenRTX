/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Shared low-level W25Q512 SPI-NOR access for the HD2, over the HW SPI0 bus
 * (drivers/SPI/spi_hd2.c).  Consolidates the flash primitives that were
 * previously copied into each NVM driver (nvmem_settings, cps_io, vendor
 * settings/records).
 *
 * 4-byte-address opcodes (the W25Q512 is 64 MB), a JEDEC-ID gate before every
 * operation (proves the bus is alive before a destructive erase/program), and
 * IRQ-locked transactions (short IE-off windows so a transfer is atomic vs the
 * rtx RSSI poll's GPIOA chip-select twiddles).  CS# = GPIOA.18, driven here.
 */

#ifndef FLASH_W25Q_HD2_H
#define FLASH_W25Q_HD2_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define W25Q_HD2_SECTOR_SIZE 4096u

/* Bring up the bus and confirm the chip answers JEDEC ef 40 20.  Idempotent;
 * call before any read/erase/program.  Returns true when the flash is usable. */
bool w25q_hd2_probe(void);

/* Read `len` bytes from absolute byte address `addr`. */
void w25q_hd2_read(uint32_t addr, void *buf, size_t len);

/* Erase the 4 kB sector containing `addr`.  Returns 0 on success, -1 on
 * timeout.  Caller must w25q_hd2_probe() first. */
int  w25q_hd2_eraseSector(uint32_t addr);

/* Program `len` bytes at `addr`, splitting on 256-byte page bounds.  The
 * target range must have been erased.  Returns 0 on success, -1 on error. */
int  w25q_hd2_program(uint32_t addr, const void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_W25Q_HD2_H */

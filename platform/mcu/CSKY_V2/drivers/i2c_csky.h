/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Software-bit-bang I2C0 for the HR_C7000 / ck803s SoC, matching the
 * public surface of the MK22FN512xxx12 I2C0 driver so chip-driver code
 * (AT1846S_HD2.cpp) can call the same i2c0_* entry points used on GDx.
 *
 * SCL = GPIOA pin 7 (HD2 vendor-firmware GPIO id 7)
 * SDA = GPIOA pin 8 (HD2 vendor-firmware GPIO id 8)
 *
 * The HD2's separate Designware-style I2C engine at MMIO 0x04000000 is
 * NOT used by the AT1846S path -- vendor V2.1.3 bit-bangs SDA/SCL via
 * GPIO writes (see src/firmware/at1846s/at1846s.c i2c_send_byte_ack +
 * i2c_xfer_write).  We mirror that here.
 */

#ifndef I2C_CSKY_H
#define I2C_CSKY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void i2c0_init(void);
void i2c0_terminate(void);

void i2c0_write(uint8_t addr, void *buf, size_t len, bool sendStop);
void i2c0_read(uint8_t addr, void *buf, size_t len);

/* Address-presence probe: START + (addr & 0xFE) write byte, sample ACK, STOP.
 * Returns true iff a slave ACKed the address.  Used to confirm a chip is on the
 * bus (e.g. the FM-broadcast tuner at 0x20) without writing any register. */
bool i2c0_probe(uint8_t addr);

void i2c0_lockDeviceBlocking(void);
void i2c0_releaseDevice(void);

#ifdef __cplusplus
}
#endif

#endif /* I2C_CSKY_H */

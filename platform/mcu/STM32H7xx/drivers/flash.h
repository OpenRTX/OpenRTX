/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Driver for MCU's internal flash management, allowing for sector erase and data
 * writing.
 */

/**
 * Erase one sector of the MCU flash memory.
 *
 * @param secNum: sector number. Sectors from bank 2 are numbered 8->15. 
 * @return true for successful erase, false otherwise.
 */
bool flash_eraseSector(const uint8_t secNum);

/**
 * Write data to the MCU flash memory. The address MUST be a multiple of 32
 * (32 bytes aligned). The length MUST be a multiple of 32. The length MUST
 * be a multiple of 32 bytes. This is to prevent ECC errors as the memory is
 * designed with 10 bits of ECC for 256 bits of memory.
 *
 * @param address: starting address for the write operation.
 * @param data: data to be written.
 * @param len: data length.

 * @return true on successful write, false otherwise.
 */
bool flash_write(const uint32_t address, const void *data, const size_t len);

/**
 * Returns the flash size in kB as stored in the device itself.
 *
 * @return device flash size in kB
*/
uint16_t flash_size();

#ifdef __cplusplus
}
#endif

#endif /* FLASH_H */

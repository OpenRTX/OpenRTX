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
 * @param secNum: sector number.
 * @return true for successful erase, false otherwise.
 */
bool flash_eraseSector(const uint8_t secNum);

/**
 * Write data to the MCU flash memory.
 *
 * @param address: starting address for the write operation.
 * @param data: data to be written.
 * @param len: data length.
 */
void flash_write(const uint32_t address, const void *data, const size_t len);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_H */

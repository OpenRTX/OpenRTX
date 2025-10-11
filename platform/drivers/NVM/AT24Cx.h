/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef AT24Cx_H
#define AT24Cx_H

#include <stdint.h>
#include <sys/types.h>
#include "interfaces/nvmem.h"

/**
 * Driver for ATMEL AT24Cx family of I2C EEPROM devices, used as external non
 * volatile memory on various radios to store global settings and contact data.
 */

/**
 * Device driver and information block for AT24Cx EEPROM memory.
 */
extern const struct nvmOps  AT24Cx_ops;
extern const struct nvmInfo AT24Cx_info;

/**
 * Instantiate an AT24Cx nonvolatile memory device.
 *
 * @param name: instance name.
 * @param sz: memory size, in bytes.
 */
#define AT24Cx_DEVICE_DEFINE(name)     \
struct nvmDevice name =                \
{                                      \
    .ops  = &AT24Cx_ops,               \
    .info = &AT24Cx_info,              \
};

/**
 * Initialise driver for external EEPROM.
 */
void AT24Cx_init();

/**
 * Terminate driver for external EEPROM.
 */
void AT24Cx_terminate();

/**
 * Read data from EEPROM memory.
 *
 * @param addr: start address for read operation.
 * @param buf: pointer to a buffer where data is written to.
 * @param len: number of bytes to read.
 * @return zero on success, negative errno code on fail.
 */
int AT24Cx_readData(uint32_t addr, void *buf, size_t len);

/**
 * Write data to EEPROM memory.
 *
 * @param addr: start address for write operation.
 * @param buf: pointer to the data to be written.
 * @param len: number of bytes to write.
 * @return zero on success, negative errno code on fail.
 */
int AT24Cx_writeData(uint32_t addr, const void *buf, size_t len);

#endif /* AT24Cx_H */

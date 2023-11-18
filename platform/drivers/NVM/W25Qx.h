/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef W25Qx_H
#define W25Qx_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <interfaces/nvmem.h>

/**
 * Driver for Winbond W25Qx family of SPI flash devices, used as external non
 * volatile memory on various radios to store both calibration and contact data.
 */

/**
 * Device driver API for W25Qx main memory.
 */
extern const struct nvmApi W25Qx_api;

/**
 * Device driver API for W25Qx security registers.
 */
extern const struct nvmApi W25Qx_secReg_api;


/**
 *  Instantiate an W25Qx nonvolatile memory device.
 *
 * @param name: device name.
 * @param driver: device driver API.
 */
#define W25Qx_DEVICE_DEFINE(name, driver) \
struct nvmDevice name =                   \
{                                         \
    .config = NULL,                       \
    .priv   = NULL,                       \
    .api    = &driver                     \
};

/**
 * Initialise driver for external flash.
 */
void W25Qx_init();

/**
 * Terminate driver for external flash.
 */
void W25Qx_terminate();

/**
 * Release flash chip from power down mode, this function should be called at
 * least once after the initialisation of the driver and every time after the
 * chip has been put in low power mode.
 * Application code must wait at least 3us before issuing any other command
 * after this one.
 */
void W25Qx_wakeup();

/**
 * Put flash chip in low power mode.
 */
void W25Qx_sleep();

/**
 * Read data from one of the flash security registers, located at addresses
 * 0x1000, 0x2000 and 0x3000 and 256-byte wide.
 * NOTE: If a read operation goes beyond the 256 byte boundary, length will be
 * truncated to the one reaching the end of the register.
 *
 * @param addr: start address for read operation, must be the full register address.
 * @param buf: pointer to a buffer where data is written to.
 * @param len: number of bytes to read.
 * @return: -1 if address is not whithin security registers address range, the
 * number of bytes effectively read otherwise.
 */
ssize_t W25Qx_readSecurityRegister(uint32_t addr, void *buf, size_t len);

/**
 * Read data from flash memory.
 *
 * @param addr: start address for read operation.
 * @param buf: pointer to a buffer where data is written to.
 * @param len: number of bytes to read.
 * @return zero on success, negative errno code on fail.
 */
int W25Qx_readData(uint32_t addr, void *buf, size_t len);

/**
 * Erase a flash memory area.
 * The start address and erase size must be aligned to page size. The function
 * blocks until the erase process terminates.
 *
 * @param addr: start address.
 * @param size: size of the area to be erased.
 * @return zero on success, negative errno code on fail.
 */
int W25Qx_erase(uint32_t addr, size_t size);

/**
 * Full chip erase.
 * Function returns when erase process terminated.
 *
 * @return true on success, false on failure.
 */
bool W25Qx_eraseChip();

/**
 * Write data to a 256-byte flash memory page.
 * NOTE: if data size goes beyond the 256 byte boundary, length will be truncated
 * to the one reaching the end of the page.
 *
 * @param addr: start address for write operation.
 * @param buf: pointer to data buffer.
 * @param len: number of bytes to written.
 * @return: -1 on error, the number of bytes effectively written otherwise.
 */
ssize_t W25Qx_writePage(uint32_t addr, const void *buf, size_t len);

/**
 * Write data to flash memory.
 *
 * @param addr: start address for read operation.
 * @param buf: pointer to a buffer where data is written to.
 * @param len: number of bytes to read.
 * @return zero on success, negative errno code on fail.
 */
int W25Qx_writeData(uint32_t addr, const void *buf, size_t len);

#endif /* W25Qx_H */

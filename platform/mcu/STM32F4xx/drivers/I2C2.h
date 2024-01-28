/***************************************************************************
 *   Copyright (C) 2021 - 2024 by Morgan Diepart ON4MOD                    *
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

#ifndef I2C2_H
#define I2C2_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>

#define SMB2_MAX_ARA_DEVICES        1   // Max number of devices that can use the alert pin

#define smb2_terminate              i2c2_terminate
#define smb2_lockDevice             i2c2_lockDevice
#define smb2_lockDeviceBlocking     i2c2_lockDeviceBlocking
#define smb2_releaseDevice          i2c2_releaseDevice
#define smb2_read_bytes             i2c2_read_bytes
#define smb2_write_bytes            i2c2_write_bytes
#define SMB2_ARA                    0x0C

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise I2C2 peripheral with a clock frequency of ~100kHz.
 * NOTE: this driver does not configure the I2C GPIOs, which have to be put in
 * alternate mode by application code.
 */
void i2c2_init();

/**
 * Initialize I2C2 peripheral in SMBus mode with a clock frequency of ~100kHz.
 * NOTE: this driver does not configure the I2C GPIOs, which have to be put in
 * alternate mode by application code.
 * @param alert: Set to true to enable SMB Alerts
 */
void smb2_init();

/**
 * Add a callback function if SMBALERT# signal is asserted by a specific address
 * NOTE: This driver does not respond to the SMBALERT# signal being asserted, so these functions will not be called as of now.
 * @param addr: 7 bits device address to which the callback is tied
 * @param callback: pointer to the function that will be called if the device specified by addr asserts the SMBALERT# signal.
 * @return 0 if successful, error code otherwise
 */
error_t smb2_add_alert_response(uint8_t addr, void (*callback)());

/**
 * Shut down I2C2 peripheral.
 * NOTE: is left to application code to change the operating mode of the I2C
 * GPIOs
 */
void i2c2_terminate();

/**
 * Acquire exclusive ownership on the I2C peripheral by locking an internal
 * mutex. This function is nonblocking and returs true if mutex has been
 * successfully locked by the caller.
 * @return true if device has been locked.
 */
bool i2c2_lockDevice();

/**
 * Acquire exclusive ownership on the I2C peripheral by locking an internal
 * mutex. In case mutex is already locked, this function blocks the execution
 * flow until it becomes free again.
 */
void i2c2_lockDeviceBlocking();

/**
 * Release exclusive ownership on the I2C peripheral.
 */
void i2c2_releaseDevice();

/**
 * Send bytes over the I2C bus.
 * @param addr: 7 bits device address to send the bytes to
 * @param bytes: array containing the bytes to send
 * @param length: number of bytes to send
 * @param stop: whether to send a stop bit or not after the transmission
 * @return 0 if successful, error code otherwise
 */
error_t i2c2_write_bytes(uint8_t addr, uint8_t *bytes, uint16_t length, bool stop);

/**
 * Receives bytes over the I2C bus.
 * @param addr: 7 bits device address to receive the bytes from
 * @param bytes: array in which to store the received bytes
 * @param length: number of bytes to receive
 * @param stop: whether to send a stop bit or not after the reception
 * @return 0 if successful, error code otherwise
 */
error_t i2c2_read_bytes(uint8_t addr, uint8_t *bytes, uint16_t length, bool stop);

/**
 * Performs an SMB2 Quick Command protocol transfer
 * @param addr: 7 bits target address
 * @param rw: read-write bit
 * @return 0 if successful, error code otherwise
 */
error_t smb2_quick_command(uint8_t addr, bool rw);

/**
 * Performs an SMB2 Send Byte protocol transfer
 * @param addr: 7 bits target address
 * @param byte: byte to send
 * @return 0 if successful, error code otherwise
 */
error_t smb2_send_byte(uint8_t addr, uint8_t byte);

/**
 * Performs an SMB2 Receive Byte protocol transfer
 * @param addr: 7 bits target address
 * @param byte: variable in which the received byte will be stored
 * @return 0 if successful, error code otherwise
 */
error_t smb2_receive_byte(uint8_t addr, uint8_t *byte);

/**
 * Performs an SMB2 Write Byte protocol transfer
 * @param addr: 7 bits target address
 * @param command: command to send to
 * @param byte: byte to send
 * @return 0 if successful, error code otherwise
 */
error_t smb2_write_byte(uint8_t addr, uint8_t command, uint8_t byte);

/**
 * Performs an SMB2 Write Word protocol transfer
 * @param addr: 7 bits target address
 * @param command: command to send to
 * @param word: word to send
 * @return 0 if successful, error code otherwise
 */
error_t smb2_write_word(uint8_t addr, uint8_t command, uint16_t word);

/**
 * Performs an SMB2 Read Byte protocol transfer
 * @param addr: 7 bits target address
 * @param command: command to read from
 * @param byte: variable in which the received byte will be stored
 * @return 0 if successful, error code otherwise
 */
error_t smb2_read_byte(uint8_t addr, uint8_t command, uint8_t *byte);

/**
 * Performs an SMB2 Read Word protocol transfer
 * @param addr: 7 bits target address
 * @param command: command to read from
 * @param word: variable in which the received word will be stored
 * @return 0 if successful, error code otherwise
 */
error_t smb2_read_word(uint8_t addr, uint8_t command, uint16_t *word);

/**
 * Performs an SMB2 Process Call protocol transfer
 * @param addr: 7 bits target address
 * @param command: command to call
 * @param word: word containing the data to send to the target.
 *              Will contain the returned data upon sucessful completion.
 * @return 0 if successful, error code otherwise
 */
error_t smb2_process_call(uint8_t addr, uint8_t command, uint16_t *word);

/**
 * Performs an SMB2 Block Write protocol transfer
 * @param addr: 7 bits target address
 * @param command: command to write the block to
 * @param bytes: array containing the bytes to send to the target
 * @param length: number of bytes in the block
 * @return 0 if successful, error code otherwise
 */
error_t smb2_block_write(uint8_t addr, uint8_t command, uint8_t *bytes, uint8_t length);

/**
 * Performs an SMB2 Block Read protocol transfer
 * @param addr: 7 bits target address
 * @param command: command to read the block from
 * @param bytes: array in which the bytes received from the target will be stored
 * @param length: number of bytes returned by the target
 * @return 0 if successful, error code otherwise
 * @note if 0 is returned, the caller is responsible for freeing the bytes array.
 */
error_t smb2_block_read(uint8_t addr, uint8_t command, uint8_t *bytes, uint8_t *length);

/**
 * Performs an SMB2 Block Write Block Read Process Call protocol transfer
 * @param addr: 7 bits target address
 * @param command: command to call
 * @param bytesW: array containing the bytes to send to the target
 * @param bytesR: array in which the bytes received from the target will be stored
 * @param length: number of bytes to send to the target.
 *                Will contain the number of bytes received from the target
 *                upon sucessful completion
 * @return 0 if successful, error code otherwise
 * @note if 0 is returned, the caller is responsible for freeing the bytesR array.
 */
error_t smb2_block_write_block_read_process_call(uint8_t addr, uint8_t command, uint8_t *bytesW, uint8_t *bytesR, uint8_t *length);

#ifdef __cplusplus
}
#endif

#endif /* I2C2_H */

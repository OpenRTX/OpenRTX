/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
 *                         Silvano Seva IU2KWO                             *
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

#ifndef CPS_IO_H
#define CPS_IO_H

#include <stdint.h>
#include <cps.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Interface for codeplug memory management.
 */

/**
 * Open a codeplug identified by its name.
 *
 * @param name: name of the codeplug, if omitted the default codeplug is used
 * @return 0 on success, -1 on failure
 */
int cps_open(char *cps_name);

/**
 * Close a codeplug.
 */
void cps_close();

/**
 * Initializes a new cps or clears cps data if one already exists.
 *
 * @param cps_name: codeplug name, if empty the default cps is initialized
 * @return 0 on success, -1 on failure
 */
int cps_create(char *cps_name);

/**
 * Read one contact from table stored in nonvolatile memory.
 *
 * @param contact: pointer to the contact_t data structure to be populated.
 * @param pos: position, inside the bank table, from which read data.
 * @return 0 on success, -1 on failure
 */
int cps_readContact(contact_t *contact, uint16_t pos);

/**
 * Read one channel entry from table stored in nonvolatile memory.
 *
 * @param channel: pointer to the channel_t data structure to be populated.
 * @param pos: position, inside the channel table, from which read data.
 * @return 0 on success, -1 on failure
 */
int cps_readChannel(channel_t *channel, uint16_t pos);

/**
 * Read one bank header from the codeplug stored in the radio's filesystem.
 *
 * @param b_header: pointer to the struct to be populated with the bank header.
 * @param pos: position, inside the bank table, from which read data.
 * @return 0 on success, -1 on failure
 */
int cps_readBankHeader(bankHdr_t *b_header, uint16_t pos);

/**
 * Read one channel index from a bank of the codeplug stored in NVM.
 *
 * @param bank_pos: position of the bank inside the cps.
 * @param pos: position of the channel index inside the bank.
 * @return the retrieved channel index on success, -1 on failure
 */
int32_t cps_readBankData(uint16_t bank_pos, uint16_t pos);

/**
 * Overwrite one contact to the codeplug stored in nonvolatile memory.
 *
 * @param contact: data structure to be written.
 * @param pos: position, inside the contact table, in which to insert data.
 * @return 0 on success, -1 on failure
 */
int cps_writeContact(contact_t contact, uint16_t pos);

/**
 * Overwrite one channel to the codeplug stored in nonvolatile memory.
 *
 * @param channel: data structure to be written.
 * @param pos: position, inside the channel table, in which to insert data.
 * @return 0 on success, -1 on failure
 */
int cps_writeChannel(channel_t channel, uint16_t pos);

/**
 * Overwrite one bank header to the codeplug stored in nonvolatile memory.
 *
 * @param b_header: data structure to be written.
 * @param pos: position, inside the bank table, in which to insert data.
 * @return 0 on success, -1 on failure
 */
int cps_writeBankHeader(bankHdr_t b_header, uint16_t pos);

/**
 * Write one channel index in a bank entry stored in the codeplug.
 *
 * @param ch: index of the new channel to be written
 * @param bank_pos: index of the bank to be written.
 * @param pos: position, inside the bank table, in which to write data.
 * @return 0 on success, -1 on failure
 */
int cps_writeBankData(uint32_t ch, uint16_t bank_pos, uint16_t pos);

/**
 * Insert one contact to the codeplug stored in nonvolatile memory,
 * updating channels accordingly.
 *
 * @param contact: data structure to be written.
 * @param pos: position, inside the contact table, in which to insert data.
 * @return 0 on success, -1 on failure
 */
int cps_insertContact(contact_t contact, uint16_t pos);

/**
 * Insert one channel to the codeplug stored in nonvolatile memory,
 * updating banks accordingly.
 *
 * @param channel: data structure to be written.
 * @param pos: position, inside the channel table, in which to insert data.
 * @return 0 on success, -1 on failure
 */
int cps_insertChannel(channel_t channel, uint16_t pos);

/**
 * Insert one bank header to the codeplug stored in nonvolatile memory.
 *
 * @param bank: data structure to be written.
 * @param pos: position, inside the bank table, in which to insert data.
 * @return 0 on success, -1 on failure
 */
int cps_insertBankHeader(bankHdr_t b_header, uint16_t pos);

/**
 * Insert one channel index in a bank entry stored in the codeplug.
 *
 * @param ch: index of the new channel to be written
 * @param bank_pos: index of the bank to be written.
 * @param pos: position, inside the bank table, in which to insert data.
 * @return 0 on success, -1 on failure
 */
int cps_insertBankData(uint32_t ch, uint16_t bank_pos, uint16_t pos);

/**
 * Delete one contact to the codeplug stored in nonvolatile memory,
 * updating channels accordingly.
 *
 * @param pos: position, inside the contact table, to delete
 * @return 0 on success, -1 on failure
 */
int cps_deleteContact(uint16_t pos);

/**
 * Delete one channel to the codeplug stored in nonvolatile memory,
 * updating banks accordingly.
 *
 * @param pos: position, inside the channel table, to delete
 * @return 0 on success, -1 on failure
 */
int cps_deleteChannel(channel_t channel, uint16_t pos);

/**
 * Delete one bank header to the codeplug stored in nonvolatile memory.
 *
 * @param pos: position, inside the bank table, to delete
 * @return 0 on success, -1 on failure
 */
int cps_deleteBankHeader(uint16_t pos);

/**
 * Delete one bank entry to the codeplug stored in nonvolatile memory.
 *
 * @param bank_pos: index of the bank to be deleted
 * @param pos: position, inside the bank table, to delete
 * @return 0 on success, -1 on failure
 */
int cps_deleteBankData(uint16_t bank_pos, uint16_t pos);

#ifdef __cplusplus
}
#endif

#endif // CPS_IO_H

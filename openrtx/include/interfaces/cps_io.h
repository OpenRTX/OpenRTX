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
 * Read one channel entry from table stored in nonvolatile memory.
 *
 * @param channel: pointer to the channel_t data structure to be populated.
 * @param pos: position, inside the channel table, from which read data.
 * @return 0 on success, -1 on failure
 */
int cps_readChannelData(channel_t *channel, uint16_t pos);

/**
 * Read one bank from the codeplug stored in the radio's filesystem.
 *
 * @param bank: pointer to the struct to be populated with the bank data.
 * @param pos: position, inside the bank table, from which read data.
 * @return 0 on success, -1 on failure
 */
int cps_readBankData(bank_t *bank, uint16_t pos);

/**
 * Read one contact from table stored in nonvolatile memory.
 *
 * @param contact: pointer to the contact_t data structure to be populated.
 * @param pos: position, inside the bank table, from which read data.
 * @return 0 on success, -1 on failure
 */
int cps_readContactData(contact_t *contact, uint16_t pos);


#ifdef __cplusplus
}
#endif

#endif // CPS_IO_H

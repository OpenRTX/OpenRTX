/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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

#ifndef RTXLINK_DAT_H
#define RTXLINK_DAT_H

#include <stdbool.h>

enum DatStatus
{
    RTXLINK_DAT_IDLE,
    RTXLINK_DAT_START_READ,
    RTXLINK_DAT_READ,
    RTXLINK_DAT_WRITE
};


/**
 * Start an rtxlink data transfer from a nonvolatile memory area to the host
 * using the DAT protocol. The function returns immediately and data transfer
 * is done in background until all the content of the area is read.
 *
 * @param area: NVM area to be read.
 * @return zero on success, a negative error code otherwhise.
 */
int dat_readNvmArea(const struct nvmDescriptor *area);

/**
 * Start an rtxlink data transfer from the host to a nonvolatile memory area
 * using the DAT protocol. The function returns immediately and data transfer
 * is done in background until all the content of the area is written.
 *
 * @param area: NVM area to be written.
 * @return zero on success, a negative error code otherwhise.
 */
int dat_writeNvmArea(const struct nvmDescriptor *area);

/**
 * Get the current status of the DAT endpoint.
 *
 * @return DAT endpoint status.
 */
enum DatStatus dat_getStatus();

/**
 * Reset the DAT endpoint interrupting any transfer eventually ongoing.
 */
void dat_reset();

#endif

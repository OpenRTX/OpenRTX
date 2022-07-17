/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
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

#include "interfaces/cps_io.h"

/**
 * This function does not apply to address-based codeplugs
 */
int cps_open(char *cps_name)
{
    (void) cps_name;
    return 0;
}

/**
 * This function does not apply to address-based codeplugs
 */
void cps_close()
{
}

/**
 * This function does not apply to address-based codeplugs
 */
int cps_create(char *cps_name)
{
    (void) cps_name;
    return 0;
}

int cps_readChannel(channel_t *channel, uint16_t pos)
{
    (void) channel;
    (void) pos;
    return -1;
}

int cps_readBankHeader(bankHdr_t *b_header, uint16_t pos)
{
    (void) b_header;
    (void) pos;
    return -1;
}

int32_t cps_readBankData(uint16_t bank_pos, uint16_t ch_pos)
{
    (void) bank_pos;
    (void) ch_pos;
    return -1;
}

int cps_readContact(contact_t *contact, uint16_t pos)
{
    (void) contact;
    (void) pos;
    return -1;
}

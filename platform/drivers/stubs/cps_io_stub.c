/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/cps_io.h"

int cps_open(char *cps_name)
{
    (void) cps_name;

    /*
    * Return 0 as if the codeplug has been correctly opened to avoid having the
    * system stuck in the attempt creating and then opening an empty codeplug.
    */
    return 0;
}

void cps_close()
{

}

int cps_create(char *cps_name)
{
    (void) cps_name;

    return -1;
}

int cps_readContact(contact_t *contact, uint16_t pos)
{
    (void) contact;
    (void) pos;

    return -1;
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

int cps_readBankData(uint16_t bank_pos, uint16_t pos)
{
    (void) bank_pos;
    (void) pos;

    return -1;
}

int cps_writeContact(contact_t contact, uint16_t pos)
{
    (void) contact;
    (void) pos;

    return -1;
}

int cps_writeChannel(channel_t channel, uint16_t pos)
{
    (void) channel;
    (void) pos;

    return -1;
}

int cps_writeBankHeader(bankHdr_t b_header, uint16_t pos)
{
    (void) b_header;
    (void) pos;

    return -1;
}

int cps_writeBankData(uint32_t ch, uint16_t bank_pos, uint16_t pos)
{
    (void) ch;
    (void) bank_pos;
    (void) pos;

    return -1;
}

int cps_insertContact(contact_t contact, uint16_t pos)
{
    (void) contact;
    (void) pos;

    return -1;
}

int cps_insertChannel(channel_t channel, uint16_t pos)
{
    (void) channel;
    (void) pos;

    return -1;
}

int cps_insertBankHeader(bankHdr_t b_header, uint16_t pos)
{
    (void) b_header;
    (void) pos;

    return -1;
}

int cps_insertBankData(uint32_t ch, uint16_t bank_pos, uint16_t pos)
{
    (void) ch;
    (void) bank_pos;
    (void) pos;

    return -1;
}

int cps_deleteContact(uint16_t pos)
{
    (void) pos;

    return -1;
}

int cps_deleteChannel(channel_t channel, uint16_t pos)
{
    (void) channel;
    (void) pos;

    return -1;
}

int cps_deleteBankHeader(uint16_t pos)
{
    (void) pos;

    return -1;
}

int cps_deleteBankData(uint16_t bank_pos, uint16_t pos)
{
    (void) bank_pos;
    (void) pos;

    return -1;
}

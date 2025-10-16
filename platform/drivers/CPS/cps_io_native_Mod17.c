/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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

int cps_readBankData(uint16_t bank_pos, uint16_t ch_pos)
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

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <interfaces/cps_io.h>
#include <interfaces/nvmem.h>
#include "core/nvmem_device.h"
#include "rtx/rtx.h"
#include <stdio.h>
#include <string.h>

#include "cps_data_RT4D.h"

static const uint32_t maxNumChannels = 1024;

int cps_open(char *cps_name)
{
    (void)cps_name;

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
    (void)cps_name;

    return -1;
}

int cps_readContact(contact_t *contact, uint16_t pos)
{
    (void)contact;
    (void)pos;

    return -1;
}

int cps_readChannel(channel_t *channel, uint16_t pos)
{
    RT4DChannel_t rt4d_channel;

    if (pos >= maxNumChannels)
        return -1;

    nvm_read(0, 2, pos * sizeof(rt4d_channel), &(rt4d_channel),
             sizeof(rt4d_channel));
    memset(channel, 0x00, sizeof(channel_t));

    // Analog Channel
    if (rt4d_channel.channel_type == 0x01 && rt4d_channel.flags == 0x00) {
        RT4DAnalogChannel analog_channel = rt4d_channel.channel_data.analog;
        channel->mode = OPMODE_FM;
        channel->bandwidth = analog_channel.bandwidth == 1 ? BW_12_5 : BW_25;
        channel->power = analog_channel.tx_power == 0 ? 1000 : 5000;
        channel->rx_frequency = analog_channel.rx_frequency * 10;
        channel->tx_frequency = analog_channel.tx_frequency * 10;
        // We only handle CTCSS tone
        if (analog_channel.rx_code_type == 0x01) {
            channel->fm.rxToneEn = 1;
            channel->fm.rxTone = analog_channel.rx_code;
            // Also set tx tone, as it is used in the UI to display the tone.
            // If tx is also configured, it will overwrite this.
            channel->fm.txTone = analog_channel.rx_code;
        }
        if (analog_channel.tx_code_type == 0x01) {
            channel->fm.txToneEn = 1;
            channel->fm.txTone = analog_channel.tx_code;
        }

    } else if (rt4d_channel.channel_type == 0x00) {
        RT4DDigitalChannel digital_channel = rt4d_channel.channel_data.digital;
        channel->mode = OPMODE_DMR;
        channel->bandwidth = BW_12_5;
        channel->power = digital_channel.tx_power == 0 ? 1000 : 5000;
        channel->rx_frequency = digital_channel.rx_frequency * 10;
        channel->tx_frequency = digital_channel.tx_frequency * 10;

        channel->dmr.dmr_timeslot = digital_channel.timeslot;
        channel->dmr.contact_index = digital_channel.contact;
        // TODO: Load DMR color code
    } else {
        return -1;
    }
    // RT-4D only uses 16 Byte channel names
    if (rt4d_channel.name[0] == 0xFF) {
        sprintf(channel->name, "Channel-%d", pos);
    } else {
        memcpy(channel->name, rt4d_channel.name, 16);
        memset(&channel->name[16], 0x00, 16);
    }

    return 0;
}

int cps_readBankHeader(bankHdr_t *b_header, uint16_t pos)
{
    (void)b_header;
    (void)pos;

    return -1;
}

int cps_readBankData(uint16_t bank_pos, uint16_t pos)
{
    (void)bank_pos;
    (void)pos;

    return -1;
}

int cps_writeContact(contact_t contact, uint16_t pos)
{
    (void)contact;
    (void)pos;

    return -1;
}

int cps_writeChannel(channel_t channel, uint16_t pos)
{
    (void)channel;
    (void)pos;

    return -1;
}

int cps_writeBankHeader(bankHdr_t b_header, uint16_t pos)
{
    (void)b_header;
    (void)pos;

    return -1;
}

int cps_writeBankData(uint32_t ch, uint16_t bank_pos, uint16_t pos)
{
    (void)ch;
    (void)bank_pos;
    (void)pos;

    return -1;
}

int cps_insertContact(contact_t contact, uint16_t pos)
{
    (void)contact;
    (void)pos;

    return -1;
}

int cps_insertChannel(channel_t channel, uint16_t pos)
{
    (void)channel;
    (void)pos;

    return -1;
}

int cps_insertBankHeader(bankHdr_t b_header, uint16_t pos)
{
    (void)b_header;
    (void)pos;

    return -1;
}

int cps_insertBankData(uint32_t ch, uint16_t bank_pos, uint16_t pos)
{
    (void)ch;
    (void)bank_pos;
    (void)pos;

    return -1;
}

int cps_deleteContact(uint16_t pos)
{
    (void)pos;

    return -1;
}

int cps_deleteChannel(channel_t channel, uint16_t pos)
{
    (void)channel;
    (void)pos;

    return -1;
}

int cps_deleteBankHeader(uint16_t pos)
{
    (void)pos;

    return -1;
}

int cps_deleteBankData(uint16_t bank_pos, uint16_t pos)
{
    (void)bank_pos;
    (void)pos;

    return -1;
}

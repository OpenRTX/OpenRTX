/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wchar.h"
#include <string.h>
#include "core/nvmem_access.h"
#include "core/nvmem_device.h"
#include "interfaces/cps_io.h"
#include "core/utils.h"
#include "cps_data_MDUV3x0.h"
#include "drivers/NVM/W25Qx.h"

extern const struct nvmDevice eflash;

static const uint32_t zoneBaseAddr       = 0x149E0;  /**< Base address of zones                                 */
static const uint32_t zoneExtBaseAddr    = 0x31000;  /**< Base address of zone extensions                       */
static const uint32_t chDataBaseAddr     = 0x110000; /**< Base address of channel data                          */
static const uint32_t contactBaseAddr    = 0x140000; /**< Base address of contacts                              */
static const uint32_t maxNumChannels     = 3000;     /**< Maximum number of channels in memory                  */
static const uint32_t maxNumZones        = 250;      /**< Maximum number of zones and zone extensions in memory */
static const uint32_t maxNumContacts     = 10000;    /**< Maximum number of contacts in memory                  */

static inline void W25Qx_readData(uint32_t addr, void *buf, size_t len)
{
    nvm_devRead(&eflash, addr, buf, len);
}

/**
 * Used to read channel data from SPI flash into a channel_t struct
 */
static int _readChannelAtAddress(channel_t *channel, uint32_t addr)
{
    mduv3x0Channel_t chData;
    W25Qx_readData(addr, ((uint8_t *) &chData), sizeof(mduv3x0Channel_t));

    // Check if the channel is empty
    #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
    if(wcslen((wchar_t *) chData.name) == 0) return -1;

    channel->mode            = chData.channel_mode;
    channel->bandwidth       = (chData.bandwidth == 0) ? 0 : 1;     // Consider 20kHz as 25kHz
    channel->rx_only         = chData.rx_only;
    channel->rx_frequency    = bcdToBin(chData.rx_frequency) * 10;
    channel->tx_frequency    = bcdToBin(chData.tx_frequency) * 10;
    channel->scanList_index  = chData.scan_list_index;
    channel->groupList_index = chData.group_list_index;

    if(chData.power == 3)
    {
        channel->power = 5000;  /* High power, 5W */
    }
    else if(chData.power == 2)
    {
        channel->power = 2500;  /* Mid power, 2.5W */
    }
    else
    {
        channel->power = 1000;  /* Low power, 1W */
    }

    /*
     * Brutally convert channel name from unicode to char by truncating the most
     * significant byte
     */
    for(uint16_t i = 0; i < 16; i++)
    {
        channel->name[i] = ((char) (chData.name[i] & 0x00FF));
    }

    /* Load mode-specific parameters */
    if(channel->mode == OPMODE_FM)
    {
        channel->fm.txToneEn = 0;
        channel->fm.rxToneEn = 0;
        uint16_t rx_css = chData.ctcss_dcs_receive;
        uint16_t tx_css = chData.ctcss_dcs_transmit;

        // TODO: Implement binary search to speed up this lookup
        if((rx_css != 0) && (rx_css != 0xFFFF))
        {
            for(int i = 0; i < CTCSS_FREQ_NUM; i++)
            {
                if(ctcss_tone[i] == ((uint16_t) bcdToBin(rx_css)))
                {
                    channel->fm.rxTone = i;
                    channel->fm.rxToneEn = 1;
                    break;
                }
            }
        }

        if((tx_css != 0) && (tx_css != 0xFFFF))
        {
            for(int i = 0; i < CTCSS_FREQ_NUM; i++)
            {
                if(ctcss_tone[i] == ((uint16_t) bcdToBin(tx_css)))
                {
                    channel->fm.txTone = i;
                    channel->fm.txToneEn = 1;
                    break;
                }
            }
        }

        // TODO: Implement warning screen if tone was not found
    }
    else if(channel->mode == OPMODE_DMR)
    {
        channel->dmr.contact_index = chData.contact_name_index;
        channel->dmr.dmr_timeslot      = chData.repeater_slot;
        channel->dmr.rxColorCode       = chData.colorcode;
        channel->dmr.txColorCode       = chData.colorcode;
    }

    return 0;
}


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
    if(pos >= maxNumChannels) return -1;

    memset(channel, 0x00, sizeof(channel_t));

    // Note: pos is 1-based because an empty slot in a zone contains index 0
    uint32_t readAddr = chDataBaseAddr + pos * sizeof(mduv3x0Channel_t);
    return _readChannelAtAddress(channel, readAddr);
}

int cps_readBankHeader(bankHdr_t *b_header, uint16_t pos)
{
    if(pos >= maxNumZones) return -1;

    mduv3x0Zone_t zoneData;
    mduv3x0ZoneExt_t zoneExtData;
    // Note: pos is 1-based to be consistent with channels
    uint32_t zoneAddr = zoneBaseAddr + pos * sizeof(mduv3x0Zone_t);
    uint32_t zoneExtAddr = zoneExtBaseAddr + pos * sizeof(mduv3x0ZoneExt_t);
    W25Qx_readData(zoneAddr, ((uint8_t *) &zoneData), sizeof(mduv3x0Zone_t));
    W25Qx_readData(zoneExtAddr, ((uint8_t *) &zoneExtData), sizeof(mduv3x0ZoneExt_t));

    // Check if zone is empty
    #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
    if(wcslen((wchar_t *) zoneData.name) == 0) return -1;
    /*
     * Brutally convert channel name from unicode to char by truncating the most
     * significant byte
     */
    for(uint16_t i = 0; i < 16; i++)
    {
        b_header->name[i] = ((char) (zoneData.name[i] & 0x00FF));
    }
    b_header->ch_count = 64;
    return 0;
}

int cps_readBankData(uint16_t bank_pos, uint16_t ch_pos)
{
    if(bank_pos >= maxNumZones) return -1;

    mduv3x0Zone_t zoneData;
    mduv3x0ZoneExt_t zoneExtData;
    uint32_t zoneAddr = zoneBaseAddr + bank_pos * sizeof(mduv3x0Zone_t);
    uint32_t zoneExtAddr = zoneExtBaseAddr + bank_pos * sizeof(mduv3x0ZoneExt_t);
    W25Qx_readData(zoneAddr, ((uint8_t *) &zoneData), sizeof(mduv3x0Zone_t));
    W25Qx_readData(zoneExtAddr, ((uint8_t *) &zoneExtData), sizeof(mduv3x0ZoneExt_t));

    // Check if zone is empty
    #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
    if(wcslen((wchar_t *) zoneData.name) == 0) return -1;
    // Channel index in zones are 1 based because an empty zone contains 0s
    // OpenRTX CPS interface is 0-based so we decrease by one
    if (ch_pos < 16)
        return zoneData.member_a[ch_pos] - 1;
    else
        return zoneExtData.ext_a[ch_pos - 16] - 1;
}

int cps_readContact(contact_t *contact, uint16_t pos)
{
    if(pos >= maxNumContacts) return -1;

    mduv3x0Contact_t contactData;
    // Note: pos is 1-based to be consistent with channels
    uint32_t contactAddr = contactBaseAddr + pos * sizeof(mduv3x0Contact_t);
    W25Qx_readData(contactAddr, ((uint8_t *) &contactData), sizeof(mduv3x0Contact_t));

    // Check if contact is empty
    if(wcslen((wchar_t *) contactData.name) == 0) return -1;
    /*
     * Brutally convert channel name from unicode to char by truncating the most
     * significant byte
     */
    for(uint16_t i = 0; i < 16; i++)
    {
        contact->name[i] = ((char) (contactData.name[i] & 0x00FF));
    }

    contact->mode = OPMODE_DMR;

    // Copy contact DMR ID
    contact->info.dmr.id = contactData.id[0]
                         | (contactData.id[1] << 8)
                         | (contactData.id[2] << 16);

    // Copy contact details
    contact->info.dmr.contactType = contactData.type;
    contact->info.dmr.rx_tone     = contactData.receive_tone ? true : false;

    return 0;
}

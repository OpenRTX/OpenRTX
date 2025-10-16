/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <string.h>
#include "wchar.h"
#include "interfaces/delays.h"
#include "interfaces/cps_io.h"
#include "core/nvmem_access.h"
#include "core/utils.h"
#include "drivers/NVM/AT24Cx.h"
#include "drivers/NVM/W25Qx.h"
#include "cps_data_GDx.h"

//static const uint32_t zoneBaseAddr        = 0x149e0;  /**< Base address of zones                */
//static const uint32_t vfoChannelBaseAddr  = 0x7590;   /**< Base address of VFO channel          */
static const uint32_t channelBaseAddrEEPROM = 0x03780;  /**< Base address of channel data         */
static const uint32_t channelBaseAddrFlash  = 0x7b1c0;  /**< Base address of channel data         */
static const uint32_t zoneBaseAddr          = 0x8010;   /**< Base address of zones                */
static const uint32_t contactBaseAddr       = 0x87620;  /**< Base address of contacts             */
static const uint32_t maxNumChannels        = 1024;     /**< Maximum number of channels in memory */
static const uint32_t maxNumZones           = 68;       /**< Maximum number of zones in memory    */
static const uint32_t maxNumContacts        = 1024;     /**< Maximum number of contacts in memory */

extern const struct nvmDevice eflash;

// Strings in GD-77 codeplug are terminated with 0xFF,
// replace 0xFF terminator with 0x00 to be compatible with C strings
static void _addStringTerminator(char *buf, uint8_t max_len)
{
    for(int i=0; i<max_len; i++)
    {
        if(buf[i] == 0xFF)
        {
            buf[i] = 0x00;
            break;
        }
    }
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
    if(pos >= maxNumChannels)
        return -1;

    memset(channel, 0x00, sizeof(channel_t));

    // Channels are organized in 128-channel banks
    uint8_t bank_num = pos / 128;
    uint8_t bank_channel = pos % 128;

    // ### Read channel bank bitmap ###
    uint8_t bitmap[16];
    // First channel bank (128 channels) is saved in EEPROM
    if(pos < 128)
    {
        uint32_t readAddr = channelBaseAddrEEPROM + bank_num * sizeof(gdxChannelBank_t);
        AT24Cx_readData(readAddr, ((uint8_t *) &bitmap), sizeof(bitmap));
    }
    // Remaining 7 channel banks (896 channels) are saved in SPI Flash
    else
    {
        uint32_t readAddr = channelBaseAddrFlash + (bank_num - 1) * sizeof(gdxChannelBank_t);
        nvm_devRead(&eflash, readAddr, ((uint8_t *) &bitmap), sizeof(bitmap));
    }
    uint8_t bitmap_byte = bank_channel / 8;
    uint8_t bitmap_bit = bank_channel % 8;
    gdxChannel_t chData;
    // The channel is marked not valid in the bitmap
    if(!(bitmap[bitmap_byte] & (1 << bitmap_bit)))
        return -1;
    // The channel is marked valid in the bitmap
    // ### Read desired channel from the correct bank ###
    else
    {
        uint32_t channelOffset = sizeof(bitmap) + pos * sizeof(gdxChannel_t);
        // First channel bank (128 channels) is saved in EEPROM
        if(pos < 128)
        {
            uint32_t bankAddr = channelBaseAddrEEPROM + bank_num * sizeof(gdxChannelBank_t);
            AT24Cx_readData(bankAddr + channelOffset, ((uint8_t *) &chData), sizeof(gdxChannel_t));
        }
        // Remaining 7 channel banks (896 channels) are saved in SPI Flash
        else
        {
            uint32_t bankAddr = channelBaseAddrFlash + bank_num * sizeof(gdxChannelBank_t);
            nvm_devRead(&eflash, bankAddr + channelOffset, ((uint8_t *) &chData), sizeof(gdxChannel_t));
        }
    }
    // Copy data to OpenRTX channel_t
    channel->mode            = chData.channel_mode + 1;
    channel->bandwidth       = chData.bandwidth;
    channel->rx_only         = chData.rx_only;
    channel->power           = ((chData.power == 1) ? 5000 : 1000); // 5W or 1W
    channel->rx_frequency    = bcdToBin(chData.rx_frequency) * 10;
    channel->tx_frequency    = bcdToBin(chData.tx_frequency) * 10;
    channel->scanList_index  = chData.scan_list_index;
    channel->groupList_index = chData.group_list_index;
    memcpy(channel->name, chData.name, sizeof(chData.name));
    // Terminate string with 0x00 instead of 0xFF
    _addStringTerminator(channel->name, sizeof(chData.name));

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
        channel->dmr.rxColorCode       = chData.colorcode_rx;
        channel->dmr.txColorCode       = chData.colorcode_tx;
    }
    return 0;
}

int cps_readBankHeader(bankHdr_t *b_header, uint16_t pos)
{
    if(pos >= maxNumZones) return -1;

    // ### Read bank bank bitmap ###
    uint8_t bitmap[32];
    AT24Cx_readData(zoneBaseAddr, ((uint8_t *) &bitmap), sizeof(bitmap));

    uint8_t bitmap_byte = pos / 8;
    uint8_t bitmap_bit = pos % 8;
    // The bank is marked not valid in the bitmap
    if(!(bitmap[bitmap_byte] & (1 << bitmap_bit))) return -1;

    gdxZone_t zoneData;
    uint32_t zoneAddr = zoneBaseAddr + sizeof(bitmap) + pos * sizeof(gdxZone_t);
    AT24Cx_readData(zoneAddr, ((uint8_t *) &zoneData), sizeof(gdxZone_t));

    // Check if bank is empty
    if(wcslen((wchar_t *) zoneData.name) == 0) return -1;

    memcpy(b_header->name, zoneData.name, sizeof(zoneData.name));
    // Terminate string with 0x00 instead of 0xFF
    _addStringTerminator(b_header->name, sizeof(zoneData.name));
    return 0;
}

int cps_readBankData(uint16_t bank_pos, uint16_t ch_pos)
{
    if(bank_pos >= maxNumZones) return -1;

    // ### Read bank bank bitmap ###
    uint8_t bitmap[32];
    AT24Cx_readData(zoneBaseAddr, ((uint8_t *) &bitmap), sizeof(bitmap));

    uint8_t bitmap_byte = bank_pos / 8;
    uint8_t bitmap_bit = bank_pos % 8;
    // The bank is marked not valid in the bitmap
    if(!(bitmap[bitmap_byte] & (1 << bitmap_bit))) return -1;

    gdxZone_t zoneData;
    uint32_t zoneAddr = zoneBaseAddr + sizeof(bitmap) + bank_pos * sizeof(gdxZone_t);
    AT24Cx_readData(zoneAddr, ((uint8_t *) &zoneData), sizeof(gdxZone_t));

    // Check if bank is empty
    if(wcslen((wchar_t *) zoneData.name) == 0) return -1;
    return zoneData.member[ch_pos];
}

int cps_readContact(contact_t *contact, uint16_t pos)
{
    if(pos >= maxNumContacts) return -1;

    gdxContact_t contactData;
    uint32_t contactAddr = contactBaseAddr + pos * sizeof(gdxContact_t);
    nvm_devRead(&eflash, contactAddr, ((uint8_t *) &contactData), sizeof(gdxContact_t));

    // Check if contact is empty
    if(wcslen((wchar_t *) contactData.name) == 0) return -1;

    // Copy contact name
    memcpy(contact->name, contactData.name, sizeof(contactData.name));
    // Terminate string with 0x00 instead of 0xFF
    _addStringTerminator(contact->name, sizeof(contactData.name));

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

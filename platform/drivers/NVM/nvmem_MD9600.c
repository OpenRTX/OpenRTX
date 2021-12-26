/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <wchar.h>
#include <string.h>
#include <interfaces/nvmem.h>
#include <interfaces/delays.h>
#include <calibInfo_MDx.h>
#include "nvmData_MDUV3x0.h"
#include "W25Qx.h"

const uint32_t zoneBaseAddr    = 0x149E0;   /**< Base address of zones                  */
const uint32_t zoneExtBaseAddr = 0x31000;   /**< Base address of zone extensions        */
const uint32_t vfoChannelBaseAddr = 0x2EF00;  /**< Base address of VFO channel          */
const uint32_t chDataBaseAddr  = 0x110000;  /**< Base address of channel data           */
const uint32_t contactBaseAddr = 0x140000;  /**< Base address of contacts               */
const uint32_t maxNumChannels  = 3000;      /**< Maximum number of channels in memory   */
const uint32_t maxNumZones     = 250;       /**< Maximum number of zones and zone extensions in memory */
const uint32_t maxNumContacts  = 10000;     /**< Maximum number of contacts in memory   */
/* This address has been chosen by OpenRTX to store the settings
 * because it is empty (0xFF) and has enough free space */
const uint32_t settingsAddr  = 0x6000;

/**
 * \internal Utility function to convert 4 byte BCD values into a 32-bit
 * unsigned integer ones.
 */
uint32_t _bcd2bin(uint32_t bcd)
{
    return ((bcd >> 28) & 0x0F) * 10000000 +
           ((bcd >> 24) & 0x0F) * 1000000 +
           ((bcd >> 20) & 0x0F) * 100000 +
           ((bcd >> 16) & 0x0F) * 10000 +
           ((bcd >> 12) & 0x0F) * 1000 +
           ((bcd >> 8) & 0x0F)  * 100 +
           ((bcd >> 4) & 0x0F)  * 10 +
           (bcd & 0x0F);
}

/**
 * Used to read channel data from SPI flash into a channel_t struct
 */
int _nvm_readChannelAtAddress(channel_t *channel, uint32_t addr)
{
    W25Qx_wakeup();
    delayUs(5);
    mduv3x0Channel_t chData;
    W25Qx_readData(addr, ((uint8_t *) &chData), sizeof(mduv3x0Channel_t));
    W25Qx_sleep();

    // Check if the channel is empty
    #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
    if(wcslen((wchar_t *) chData.name) == 0) return -1;

    channel->mode            = chData.channel_mode;
    channel->bandwidth       = chData.bandwidth;
    channel->admit_criteria  = chData.admit_criteria;
    channel->squelch         = chData.squelch;
    channel->rx_only         = chData.rx_only;
    channel->vox             = chData.vox;
    channel->rx_frequency    = _bcd2bin(chData.rx_frequency) * 10;
    channel->tx_frequency    = _bcd2bin(chData.tx_frequency) * 10;
    channel->tot             = chData.tot;
    channel->tot_rekey_delay = chData.tot_rekey_delay;
    channel->emSys_index     = chData.emergency_system_index;
    channel->scanList_index  = chData.scan_list_index;
    channel->groupList_index = chData.group_list_index;

    if(chData.power == 3)
    {
        channel->power = 5.0f;  /* High power -> 5W */
    }
    else if(chData.power == 2)
    {
        channel->power = 2.5f;  /* Mid power -> 2.5W */
    }
    else
    {
        channel->power = 1.0f;  /* Low power -> 1W */
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
            for(int i = 0; i < MAX_TONE_INDEX; i++)
            {
                if(ctcss_tone[i] == ((uint16_t) _bcd2bin(rx_css)))
                {
                    channel->fm.rxTone = i;
                    channel->fm.rxToneEn = 1;
                    break;
                }
            }
        }

        if((tx_css != 0) && (tx_css != 0xFFFF))
        {
            for(int i = 0; i < MAX_TONE_INDEX; i++)
            {
                if(ctcss_tone[i] == ((uint16_t) _bcd2bin(tx_css)))
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
        channel->dmr.contactName_index = chData.contact_name_index;
        channel->dmr.dmr_timeslot      = chData.repeater_slot;
        channel->dmr.rxColorCode       = chData.colorcode;
        channel->dmr.txColorCode       = chData.colorcode;
    }

    return 0;
}

void nvm_init()
{
    W25Qx_init();
}

void nvm_terminate()
{
    W25Qx_terminate();
}

void nvm_readCalibData(void *buf)
{
    return;
}

/*
TODO: temporarily implemented in "nvmem_settings_MDx.c"

int nvm_readVFOChannelData(channel_t *channel)
{
    return _nvm_readChannelAtAddress(channel, vfoChannelBaseAddr);
}
*/

int nvm_readChannelData(channel_t *channel, uint16_t pos)
{
    if((pos <= 0) || (pos > maxNumChannels)) return -1;

    // Note: pos is 1-based because an empty slot in a zone contains index 0
    uint32_t readAddr = chDataBaseAddr + (pos - 1) * sizeof(mduv3x0Channel_t);
    return _nvm_readChannelAtAddress(channel, readAddr);
}

int nvm_readZoneData(zone_t *zone, uint16_t pos)
{
    if((pos <= 0) || (pos > maxNumZones)) return -1;

    W25Qx_wakeup();
    delayUs(5);

    mduv3x0Zone_t zoneData;
    mduv3x0ZoneExt_t zoneExtData;
    // Note: pos is 1-based to be consistent with channels
    uint32_t zoneAddr = zoneBaseAddr + (pos - 1) * sizeof(mduv3x0Zone_t);
    uint32_t zoneExtAddr = zoneExtBaseAddr + (pos - 1) * sizeof(mduv3x0ZoneExt_t);
    W25Qx_readData(zoneAddr, ((uint8_t *) &zoneData), sizeof(mduv3x0Zone_t));
    W25Qx_readData(zoneExtAddr, ((uint8_t *) &zoneExtData), sizeof(mduv3x0ZoneExt_t));
    W25Qx_sleep();

    // Check if zone is empty
    #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
    if(wcslen((wchar_t *) zoneData.name) == 0) return -1;
    /*
     * Brutally convert channel name from unicode to char by truncating the most
     * significant byte
     */
    for(uint16_t i = 0; i < 16; i++)
    {
        zone->name[i] = ((char) (zoneData.name[i] & 0x00FF));
    }
    // Copy zone channel indexes
    for(uint16_t i = 0; i < 16; i++)
    {
        zone->member[i] = zoneData.member_a[i];
    }
    // Copy zone extension channel indexes
    for(uint16_t i = 0; i < 48; i++)
    {
        zone->member[16 + i] = zoneExtData.ext_a[i];
    }

    return 0;
}

int nvm_readContactData(contact_t *contact, uint16_t pos)
{
    if((pos <= 0) || (pos > maxNumContacts)) return -1;

    W25Qx_wakeup();
    delayUs(5);

    mduv3x0Contact_t contactData;
    // Note: pos is 1-based to be consistent with channels
    uint32_t contactAddr = contactBaseAddr + (pos - 1) * sizeof(mduv3x0Contact_t);
    W25Qx_readData(contactAddr, ((uint8_t *) &contactData), sizeof(mduv3x0Contact_t));
    W25Qx_sleep();

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
    // Copy contact DMR ID
    contact->id = (contactData.id[0] | contactData.id[1] << 8 | contactData.id[2] << 16);
    // Copy contact details
    contact->type = contactData.type;
    contact->receive_tone = contactData.receive_tone ? true : false;

    return 0;
}

/*

TODO: temporarily implemented in "nvmem_settings_MDx.c"

int nvm_readSettings(settings_t *settings)
{
    settings_t newSettings;
    W25Qx_wakeup();
    delayUs(5);
    W25Qx_readData(settingsAddr, ((uint8_t *) &newSettings), sizeof(settings_t));
    W25Qx_sleep();
    if(memcmp(newSettings.valid, default_settings.valid, 6) != 0)
        return -1;
    memcpy(settings, &newSettings, sizeof(settings_t));
    return 0;
}
*/

int nvm_writeSettings(const settings_t *settings)
{
    // Disable settings write until DFU is implemented for flash backups
    return -1;

    W25Qx_wakeup();
    delayUs(5);
    bool success = W25Qx_writeData(settingsAddr, ((uint8_t *) &settings), sizeof(settings_t));
    W25Qx_sleep();
    return success? 0 : -1;
}

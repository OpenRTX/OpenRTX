/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
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
#include <interfaces/nvmem.h>
#include <interfaces/delays.h>
#include <calibInfo_MDx.h>
#include "W25Qx.h"

/**
 * \internal Data structure matching the one used by original MD3x0 firmware to
 * manage codeplug data inside nonvolatile flash memory.
 *
 * Taken by dmrconfig repository: https://github.com/sergev/dmrconfig/blob/master/uv380.c
 */
typedef struct
{
    // Byte 0
    uint8_t channel_mode        : 2,
            bandwidth           : 2,
            autoscan            : 1,
            _unused1            : 2,
            lone_worker         : 1;

    // Byte 1
    uint8_t _unused2            : 1,
            rx_only             : 1,
            repeater_slot       : 2,
            colorcode           : 4;

    // Byte 2
    uint8_t privacy_no          : 4,
            privacy             : 2,
            private_call_conf   : 1,
            data_call_conf      : 1;

    // Byte 3
    uint8_t rx_ref_frequency    : 2,
            _unused3            : 1,
            emergency_alarm_ack : 1,
            _unused4            : 3,
            display_pttid_dis   : 1;

    // Byte 4
    uint8_t tx_ref_frequency    : 2,
            _unused5            : 2,
            vox                 : 1,
            _unused6            : 1,
            admit_criteria      : 2;

    // Byte 5
    uint8_t _unused7            : 4,
            in_call_criteria    : 2,
            turn_off_freq       : 2;

    // Bytes 6-7
    uint16_t contact_name_index;

    // Bytes 8-9
    uint8_t tot                 : 6,
            _unused13           : 2;
    uint8_t tot_rekey_delay;

    // Bytes 10-11
    uint8_t emergency_system_index;
    uint8_t scan_list_index;

    // Bytes 12-13
    uint8_t group_list_index;
    uint8_t _unused8;

    // Bytes 14-15
    uint8_t _unused9;
    uint8_t squelch;

    // Bytes 16-23
    uint32_t rx_frequency;
    uint32_t tx_frequency;

    // Bytes 24-27
    uint16_t ctcss_dcs_receive;
    uint16_t ctcss_dcs_transmit;

    // Bytes 28-29
    uint8_t rx_signaling_syst;
    uint8_t tx_signaling_syst;

    // Byte 30
    uint8_t power               : 2,
            _unused10           : 6;

    // Byte 31
    uint8_t _unused11           : 3,
            dcdm_switch_dis     : 1,
            leader_ms           : 1,
            _unused12           : 3;

    // Bytes 32-63
    uint16_t name[16];
} mduv3x0Channel_t;

typedef struct {
    // Bytes 0-31
    uint16_t name[16];                  // Zone Name (Unicode)

    // Bytes 32-63
    uint16_t member_a[16];              // Member A: channels 1...16
} mduv3x0Zone_t;

typedef struct {
    // Bytes 0-95
    uint16_t ext_a[48];                 // Member A: channels 17...64

    // Bytes 96-223
    uint16_t member_b[64];              // Member B: channels 1...64
} mduv3x0ZoneExt_t;

typedef struct {
    // Bytes 0-2
    uint8_t id[3];                      // Call ID: 1...16777215

    // Byte 3
    uint8_t type                : 5,    // Call Type: Group Call, Private Call or All Call
            receive_tone        : 1,    // Call Receive Tone: No or yes
            _unused2            : 2;    // 0b11

    // Bytes 4-35
    uint16_t name[16];                  // Contact Name (Unicode)
} mduv3x0Contact_t;

const uint32_t zoneBaseAddr = 0x149e0; /**< Base address of zones         */
const uint32_t zoneExtBaseAddr = 0x31000; /**< Base address of zone extensions         */
const uint32_t chDataBaseAddr = 0x110000; /**< Base address of channel data         */
const uint32_t contactBaseAddr = 0x140000; /**< Base address of contacts         */
const uint32_t maxNumChannels = 3000;    /**< Maximum number of channels in memory */
const uint32_t maxNumZones = 250;    /**< Maximum number of zones and zone extensions in memory */
const uint32_t maxNumContacts = 10000;    /**< Maximum number of contacts in memory */

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
    W25Qx_wakeup();
    delayUs(5);

    mduv3x0Calib_t *calib = ((mduv3x0Calib_t *) buf);

    /* Common calibration data */
    (void) W25Qx_readSecurityRegister(0x1000, (&calib->vox1), 6);

    /* UHF-band calibration data */
    (void) W25Qx_readSecurityRegister(0x1009, (&calib->uhfCal.freqAdjustMid), 1);
    (void) W25Qx_readSecurityRegister(0x1010, calib->uhfCal.txHighPower, 9);
    (void) W25Qx_readSecurityRegister(0x2090, calib->uhfCal.txMidPower, 9);
    (void) W25Qx_readSecurityRegister(0x1020, calib->uhfCal.txLowPower, 9);
    (void) W25Qx_readSecurityRegister(0x1030, calib->uhfCal.rxSensitivity, 9);
    (void) W25Qx_readSecurityRegister(0x1040, calib->uhfCal.openSql9, 9);
    (void) W25Qx_readSecurityRegister(0x1050, calib->uhfCal.closeSql9, 9);
    (void) W25Qx_readSecurityRegister(0x1070, calib->uhfCal.closeSql1, 9);
    (void) W25Qx_readSecurityRegister(0x1060, calib->uhfCal.openSql1, 9);
    (void) W25Qx_readSecurityRegister(0x1090, calib->uhfCal.ctcss67Hz, 9);
    (void) W25Qx_readSecurityRegister(0x10a0, calib->uhfCal.ctcss151Hz, 9);
    (void) W25Qx_readSecurityRegister(0x10b0, calib->uhfCal.ctcss254Hz, 9);
    (void) W25Qx_readSecurityRegister(0x10d0, calib->uhfCal.dcsMod1, 9);
    (void) W25Qx_readSecurityRegister(0x2030, calib->uhfCal.sendIrange, 9);
    (void) W25Qx_readSecurityRegister(0x2040, calib->uhfCal.sendQrange, 9);
    (void) W25Qx_readSecurityRegister(0x2070, calib->uhfCal.analogSendIrange, 9);
    (void) W25Qx_readSecurityRegister(0x2080, calib->uhfCal.analogSendQrange, 9);

    uint32_t freqs[18];
    (void) W25Qx_readSecurityRegister(0x20b0, ((uint8_t *) &freqs), 72);

    for(uint8_t i = 0; i < 9; i++)
    {
        calib->uhfCal.rxFreq[i] = ((freq_t) _bcd2bin(freqs[2*i]));
        calib->uhfCal.txFreq[i] = ((freq_t) _bcd2bin(freqs[2*i+1]));
    }

    /* VHF-band calibration data */
    (void) W25Qx_readSecurityRegister(0x100c, (&calib->vhfCal.freqAdjustMid), 1);
    (void) W25Qx_readSecurityRegister(0x1019, calib->vhfCal.txHighPower, 5);
    (void) W25Qx_readSecurityRegister(0x2099, calib->vhfCal.txMidPower, 5);
    (void) W25Qx_readSecurityRegister(0x1029, calib->vhfCal.txLowPower, 5);
    (void) W25Qx_readSecurityRegister(0x1039, calib->vhfCal.rxSensitivity, 5);
    (void) W25Qx_readSecurityRegister(0x109b, calib->vhfCal.ctcss67Hz, 5);
    (void) W25Qx_readSecurityRegister(0x10ab, calib->vhfCal.ctcss151Hz, 5);
    (void) W25Qx_readSecurityRegister(0x10bb, calib->vhfCal.ctcss254Hz, 5);
    (void) W25Qx_readSecurityRegister(0x10e0, calib->vhfCal.openSql9, 5);
    (void) W25Qx_readSecurityRegister(0x10e5, calib->vhfCal.closeSql9, 5);
    (void) W25Qx_readSecurityRegister(0x10ea, calib->vhfCal.closeSql1, 5);
    (void) W25Qx_readSecurityRegister(0x10ef, calib->vhfCal.openSql1, 5);
    (void) W25Qx_readSecurityRegister(0x10db, calib->vhfCal.dcsMod1, 5);
    (void) W25Qx_readSecurityRegister(0x2039, calib->vhfCal.sendIrange, 5);
    (void) W25Qx_readSecurityRegister(0x2049, calib->vhfCal.sendQrange, 5);
    (void) W25Qx_readSecurityRegister(0x2079, calib->uhfCal.analogSendIrange, 5);
    (void) W25Qx_readSecurityRegister(0x2089, calib->vhfCal.analogSendQrange, 5);

    (void) W25Qx_readSecurityRegister(0x2000, ((uint8_t *) &freqs), 40);
    W25Qx_sleep();

    for(uint8_t i = 0; i < 5; i++)
    {
        calib->vhfCal.rxFreq[i] = ((freq_t) _bcd2bin(freqs[2*i]));
        calib->vhfCal.txFreq[i] = ((freq_t) _bcd2bin(freqs[2*i+1]));
    }
}

int nvm_readChannelData(channel_t *channel, uint16_t pos)
{
    if(pos >= maxNumChannels) return -1;

    W25Qx_wakeup();
    delayUs(5);

    mduv3x0Channel_t chData;
    uint32_t readAddr = chDataBaseAddr + pos * sizeof(mduv3x0Channel_t);
    W25Qx_readData(readAddr, ((uint8_t *) &chData), sizeof(mduv3x0Channel_t));
    W25Qx_sleep();

    // Check if the channel is empty
    if(wcslen((wchar_t *) chData.name) == 0) return -1;

    channel->mode            = chData.channel_mode - 1;
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
    if(channel->mode == FM)
    {
        channel->fm.rxToneEn = chData.ctcss_dcs_receive != 0;
        channel->fm.txToneEn = chData.ctcss_dcs_transmit != 0;
        // TODO: Implement binary search to speed up this lookup
        if (channel->fm.rxToneEn)
        {
            for(int i = 0; i < MAX_TONE_INDEX; i++)
            {
                if (ctcss_tone[i] == chData.ctcss_dcs_receive)
                {
                    channel->fm.rxTone = i;
                    break;
                }
            }

        }
        if (channel->fm.txToneEn)
        {
            for(int i = 0; i < MAX_TONE_INDEX; i++)
            {
                if (ctcss_tone[i] == chData.ctcss_dcs_transmit)
                {
                    channel->fm.txTone = i;
                    break;
                }
            }
        }
        // TODO: Implement warning screen if tone was not found
    }
    else if(channel->mode == DMR)
    {
        channel->dmr.contactName_index = chData.contact_name_index;
        channel->dmr.dmr_timeslot      = chData.repeater_slot;
        channel->dmr.rxColorCode       = chData.colorcode;
        channel->dmr.txColorCode       = chData.colorcode;
    }

    return 0;
}

int nvm_readZoneData(zone_t *zone, uint16_t pos)
{
    if(pos >= maxNumZones) return -1;

    W25Qx_wakeup();
    delayUs(5);

    mduv3x0Zone_t zoneData;
    mduv3x0ZoneExt_t zoneExtData;
    uint32_t zoneAddr = zoneBaseAddr + pos * sizeof(mduv3x0Zone_t);
    uint32_t zoneExtAddr = zoneExtBaseAddr + pos * sizeof(mduv3x0ZoneExt_t);
    W25Qx_readData(zoneAddr, ((uint8_t *) &zoneData), sizeof(mduv3x0Zone_t));
    W25Qx_readData(zoneExtAddr, ((uint8_t *) &zoneExtData), sizeof(mduv3x0ZoneExt_t));
    W25Qx_sleep();

    // Check if zone is empty
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
    if(pos >= maxNumContacts) return -1;

    W25Qx_wakeup();
    delayUs(5);

    mduv3x0Contact_t contactData;
    uint32_t contactAddr = contactBaseAddr + pos * sizeof(mduv3x0Contact_t);
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

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

#include <interfaces/nvmem.h>
#include <interfaces/delays.h>
#include <calibInfo_MDx.h>
#include <wchar.h>
#include "W25Qx.h"

/**
 * \internal Data structure matching the one used by original MD3x0 firmware to
 * manage channel data inside nonvolatile flash memory.
 *
 * Taken by dmrconfig repository: https://github.com/sergev/dmrconfig/blob/master/md380.c
 */
typedef struct
{
    // Byte 0
    uint8_t channel_mode        : 2,
            bandwidth           : 2,
            autoscan            : 1,
            squelch             : 1,
            _unused1            : 1,
            lone_worker         : 1;

    // Byte 1
    uint8_t talkaround          : 1,
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
            _unused2            : 1,
            emergency_alarm_ack : 1,
            _unused3            : 2,
            uncompressed_udp    : 1,
            display_pttid_dis   : 1;

    // Byte 4
    uint8_t tx_ref_frequency    : 2,
            _unused4            : 2,
            vox                 : 1,
            power               : 1,
            admit_criteria      : 2;

    // Byte 5
    uint8_t _unused5            : 4,
            in_call_criteria    : 2,
            _unused6            : 2;

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
    uint8_t _unused7;

    // Bytes 14-15
    uint8_t _unused8;
    uint8_t _unused9;

    // Bytes 16-23
    uint32_t rx_frequency;
    uint32_t tx_frequency;

    // Bytes 24-27
    uint16_t ctcss_dcs_receive;
    uint16_t ctcss_dcs_transmit;

    // Bytes 28-29
    uint8_t rx_signaling_syst;
    uint8_t tx_signaling_syst;

    // Bytes 30-31
    uint8_t _unused10;
    uint8_t _unused11;

    // Bytes 32-63
    uint16_t name[16];
}
md3x0Channel_t;

typedef struct {
    // Bytes 0-31
    uint16_t name[16];                  // Zone Name (Unicode)

    // Bytes 32-63
    uint16_t member[16];               // Member: channels 1...16
} md3x0Zone_t;

typedef struct {
    // Bytes 0-2
    uint8_t id[3];                      // Call ID: 1...16777215

    // Byte 3
    uint8_t type                : 5,    // Call Type: Group Call, Private Call or All Call
            receive_tone        : 1,    // Call Receive Tone: No or yes
            _unused2            : 2;    // 0b11

    // Bytes 4-35
    uint16_t name[16];                  // Contact Name (Unicode)
} md3x0Contact_t;

const uint32_t zoneBaseAddr = 0x149e0; /**< Base address of zones         */
const uint32_t chDataBaseAddr = 0x1ee00; /**< Base address of channel data         */
const uint32_t contactBaseAddr = 0x05f80; /**< Base address of contacts         */
const uint32_t maxNumChannels = 1000;    /**< Maximum number of channels in memory */
const uint32_t maxNumZones = 250;    /**< Maximum number of zones in memory */
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

    md3x0Calib_t *calib = ((md3x0Calib_t *) buf);

    (void) W25Qx_readSecurityRegister(0x1000, &(calib->vox1), 11);
    (void) W25Qx_readSecurityRegister(0x1010, calib->txHighPower, 9);
    (void) W25Qx_readSecurityRegister(0x1020, calib->txLowPower, 9);
    (void) W25Qx_readSecurityRegister(0x1030, calib->rxSensitivity, 9);
    (void) W25Qx_readSecurityRegister(0x1040, calib->openSql9, 9);
    (void) W25Qx_readSecurityRegister(0x1050, calib->closeSql9, 9);
    (void) W25Qx_readSecurityRegister(0x1060, calib->openSql1, 9);
    (void) W25Qx_readSecurityRegister(0x1070, calib->closeSql1, 9);
    (void) W25Qx_readSecurityRegister(0x1080, calib->maxVolume, 9);
    (void) W25Qx_readSecurityRegister(0x1090, calib->ctcss67Hz, 9);
    (void) W25Qx_readSecurityRegister(0x10a0, calib->ctcss151Hz, 9);
    (void) W25Qx_readSecurityRegister(0x10b0, calib->ctcss254Hz, 9);
    (void) W25Qx_readSecurityRegister(0x10c0, calib->dcsMod2, 9);
    (void) W25Qx_readSecurityRegister(0x10d0, calib->dcsMod1, 9);
    (void) W25Qx_readSecurityRegister(0x10e0, calib->mod1Partial, 9);
    (void) W25Qx_readSecurityRegister(0x10f0, calib->analogVoiceAdjust, 9);

    (void) W25Qx_readSecurityRegister(0x2000, calib->lockVoltagePartial, 9);
    (void) W25Qx_readSecurityRegister(0x2010, calib->sendIpartial, 9);
    (void) W25Qx_readSecurityRegister(0x2020, calib->sendQpartial, 9);
    (void) W25Qx_readSecurityRegister(0x2030, calib->sendIrange, 9);
    (void) W25Qx_readSecurityRegister(0x2040, calib->sendQrange, 9);
    (void) W25Qx_readSecurityRegister(0x2050, calib->rxIpartial, 9);
    (void) W25Qx_readSecurityRegister(0x2060, calib->rxQpartial, 9);
    (void) W25Qx_readSecurityRegister(0x2070, calib->analogSendIrange, 9);
    (void) W25Qx_readSecurityRegister(0x2080, calib->analogSendQrange, 9);

    uint32_t freqs[18];
    (void) W25Qx_readSecurityRegister(0x20b0, ((uint8_t *) &freqs), 72);
    W25Qx_sleep();

    /*
     * Ugly quirk: frequency stored in calibration data is divided by ten, so,
     * after bcd2bin conversion we have something like 40'135'000. To ajdust
     * things, frequency has to be multiplied by ten.
     */
    for(uint8_t i = 0; i < 9; i++)
    {
        calib->rxFreq[i] = ((freq_t) _bcd2bin(freqs[2*i])) * 10;
        calib->txFreq[i] = ((freq_t) _bcd2bin(freqs[2*i+1])) * 10;
    }
}

void nvm_loadHwInfo(hwInfo_t *info)
{
    uint16_t freqMin = 0;
    uint16_t freqMax = 0;
    uint8_t  lcdInfo = 0;

    /*
     * Hardware information data in MD3x0 devices is stored in security register
     * 0x3000.
     */
    W25Qx_wakeup();
    delayUs(5);

    (void) W25Qx_readSecurityRegister(0x3000, info->name, 8);
    (void) W25Qx_readSecurityRegister(0x3014, &freqMin, 2);
    (void) W25Qx_readSecurityRegister(0x3016, &freqMax, 2);
    (void) W25Qx_readSecurityRegister(0x301D, &lcdInfo, 1);
    W25Qx_sleep();

    /* Ensure correct null-termination of device name by removing the 0xff. */
    for(uint8_t i = 0; i < sizeof(info->name); i++)
    {
        if(info->name[i] == 0xFF) info->name[i] = '\0';
    }

    /* These devices are single-band only, either VHF or UHF. */
    freqMin = ((uint16_t) _bcd2bin(freqMin))/10;
    freqMax = ((uint16_t) _bcd2bin(freqMax))/10;

    if(freqMin < 200)
    {
        info->vhf_maxFreq = freqMax;
        info->vhf_minFreq = freqMin;
        info->vhf_band    = 1;
    }
    else
    {
        info->uhf_maxFreq = freqMax;
        info->uhf_minFreq = freqMin;
        info->uhf_band    = 1;
    }

    info->lcd_type = lcdInfo & 0x03;
}

int nvm_readChannelData(channel_t *channel, uint16_t pos)
{
    if(pos > maxNumChannels) return -1;

    W25Qx_wakeup();
    delayUs(5);

    md3x0Channel_t chData;
    uint32_t readAddr = chDataBaseAddr + pos * sizeof(md3x0Channel_t);
    W25Qx_readData(readAddr, ((uint8_t *) &chData), sizeof(md3x0Channel_t));
    W25Qx_sleep();

    channel->mode            = chData.channel_mode - 1;
    channel->bandwidth       = chData.bandwidth;
    channel->admit_criteria  = chData.admit_criteria;
    channel->squelch         = chData.squelch;
    channel->rx_only         = chData.rx_only;
    channel->vox             = chData.vox;
    channel->power           = ((chData.power == 1) ? 5.0f : 1.0f);
    channel->rx_frequency    = _bcd2bin(chData.rx_frequency) * 10;
    channel->tx_frequency    = _bcd2bin(chData.tx_frequency) * 10;
    channel->tot             = chData.tot;
    channel->tot_rekey_delay = chData.tot_rekey_delay;
    channel->emSys_index     = chData.emergency_system_index;
    channel->scanList_index  = chData.scan_list_index;
    channel->groupList_index = chData.group_list_index;

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

    md3x0Zone_t zoneData;
    uint32_t zoneAddr = zoneBaseAddr + pos * sizeof(md3x0Zone_t);
    W25Qx_readData(zoneAddr, ((uint8_t *) &zoneData), sizeof(md3x0Zone_t));
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
        zone->member[i] = zoneData.member[i];
    }

    return 0;
}

int nvm_readContactData(contact_t *contact, uint16_t pos)
{
    if(pos >= maxNumContacts) return -1;

    W25Qx_wakeup();
    delayUs(5);

    md3x0Contact_t contactData;
    uint32_t contactAddr = contactBaseAddr + pos * sizeof(md3x0Contact_t);
    W25Qx_readData(contactAddr, ((uint8_t *) &contactData), sizeof(md3x0Contact_t));
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


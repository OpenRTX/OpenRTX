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
#include "nvmData_MD3x0.h"
#include "W25Qx.h"

const uint32_t zoneBaseAddr    = 0x149e0;  /**< Base address of zones                */
const uint32_t chDataBaseAddr  = 0x1ee00;  /**< Base address of channel data         */
const uint32_t contactBaseAddr = 0x05f80;  /**< Base address of contacts             */
const uint32_t maxNumChannels  = 1000;     /**< Maximum number of channels in memory */
const uint32_t maxNumZones     = 250;      /**< Maximum number of zones in memory    */
const uint32_t maxNumContacts  = 10000;    /**< Maximum number of contacts in memory */

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

/**
 * The MD380 stock CPS does not have a VFO channel slot
 * because the stock firmware does not have a VFO
 * To enable this functionality reserve a Flash portion for saving the VFO
 *
 * TODO: temporarily implemented in "nvmem_settings_MDx.c"

int nvm_readVFOChannelData(channel_t *channel)
{
    (void) channel;
    return -1;
}
*/

int nvm_readChannelData(channel_t *channel, uint16_t pos)
{
    if((pos <= 0) || (pos > maxNumChannels)) return -1;

    W25Qx_wakeup();
    delayUs(5);

    md3x0Channel_t chData;
    // Note: pos is 1-based because an empty slot in a zone contains index 0
    uint32_t readAddr = chDataBaseAddr + (pos - 1) * sizeof(md3x0Channel_t);
    W25Qx_readData(readAddr, ((uint8_t *) &chData), sizeof(md3x0Channel_t));
    W25Qx_sleep();

    channel->mode            = chData.channel_mode;
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

int nvm_readZoneData(zone_t *zone, uint16_t pos)
{
    if((pos <= 0) || (pos > maxNumZones)) return -1;

    W25Qx_wakeup();
    delayUs(5);

    md3x0Zone_t zoneData;
    // Note: pos is 1-based to be consistent with channels
    uint32_t zoneAddr = zoneBaseAddr + (pos - 1) * sizeof(md3x0Zone_t);
    W25Qx_readData(zoneAddr, ((uint8_t *) &zoneData), sizeof(md3x0Zone_t));
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
        zone->member[i] = zoneData.member[i];
    }

    return 0;
}

int nvm_readContactData(contact_t *contact, uint16_t pos)
{
    if((pos <= 0) || (pos > maxNumContacts)) return -1;

    W25Qx_wakeup();
    delayUs(5);

    md3x0Contact_t contactData;
    // Note: pos is 1-based to be consistent with channels
    uint32_t contactAddr = contactBaseAddr + (pos - 1) * sizeof(md3x0Contact_t);
    W25Qx_readData(contactAddr, ((uint8_t *) &contactData), sizeof(md3x0Contact_t));
    W25Qx_sleep();

    // Check if contact is empty
    #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
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
    (void) settings;
    return -1;
}
*/

int nvm_writeSettings(const settings_t *settings)
{
    (void) settings;
    return -1;
}

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

#include <string.h>
#include <wchar.h>
#include <interfaces/delays.h>
#include <interfaces/nvmem.h>
#include <calibInfo_GDx.h>
#include "AT24Cx.h"
#include "W25Qx.h"
#include "nvmData_GDx.h"

#if defined(PLATFORM_GD77)
static const uint32_t UHF_CAL_BASE = 0x8F000;
static const uint32_t VHF_CAL_BASE = 0x8F070;
#elif defined(PLATFORM_DM1801)
static const uint32_t UHF_CAL_BASE = 0x6F000;
static const uint32_t VHF_CAL_BASE = 0x6F070;
#else
#warning GDx calibration: platform not supported
#endif

//const uint32_t zoneBaseAddr    = 0x149e0;      /**< Base address of zones                */
const uint32_t channelBaseAddrEEPROM = 0x03780;  /**< Base address of channel data         */
const uint32_t channelBaseAddrFlash  = 0x7b1c0;  /**< Base address of channel data         */
const uint32_t vfoChannelBaseAddr = 0x7590;      /**< Base address of VFO channel          */
const uint32_t zoneBaseAddr = 0x8010;            /**< Base address of zones                */
const uint32_t contactBaseAddr = 0x87620;        /**< Base address of contacts             */
const uint32_t maxNumChannels  = 1024;           /**< Maximum number of channels in memory */
const uint32_t maxNumZones     = 68;             /**< Maximum number of zones in memory    */
const uint32_t maxNumContacts  = 1024;           /**< Maximum number of contacts in memory */

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
 * \internal Utility function for loading band-specific calibration data into
 * the corresponding data structure.
 */
void _loadBandCalData(uint32_t baseAddr, bandCalData_t *cal)
{
    W25Qx_readData(baseAddr + 0x08, &(cal->modBias),               2);
    W25Qx_readData(baseAddr + 0x0A, &(cal->mod2Offset),            1);
    W25Qx_readData(baseAddr + 0x3F, cal->analogSqlThresh,          8);
    W25Qx_readData(baseAddr + 0x47, &(cal->noise1_HighTsh_Wb),     1);
    W25Qx_readData(baseAddr + 0x48, &(cal->noise1_LowTsh_Wb),      1);
    W25Qx_readData(baseAddr + 0x49, &(cal->noise2_HighTsh_Wb),     1);
    W25Qx_readData(baseAddr + 0x4A, &(cal->noise2_LowTsh_Wb),      1);
    W25Qx_readData(baseAddr + 0x4B, &(cal->rssi_HighTsh_Wb),       1);
    W25Qx_readData(baseAddr + 0x4C, &(cal->rssi_LowTsh_Wb),        1);
    W25Qx_readData(baseAddr + 0x4D, &(cal->noise1_HighTsh_Nb),     1);
    W25Qx_readData(baseAddr + 0x4E, &(cal->noise1_LowTsh_Nb),      1);
    W25Qx_readData(baseAddr + 0x4F, &(cal->noise2_HighTsh_Nb),     1);
    W25Qx_readData(baseAddr + 0x50, &(cal->noise2_LowTsh_Nb),      1);
    W25Qx_readData(baseAddr + 0x51, &(cal->rssi_HighTsh_Nb),       1);
    W25Qx_readData(baseAddr + 0x52, &(cal->rssi_LowTsh_Nb),        1);
    W25Qx_readData(baseAddr + 0x53, &(cal->RSSILowerThreshold),    1);
    W25Qx_readData(baseAddr + 0x54, &(cal->RSSIUpperThreshold),    1);
    W25Qx_readData(baseAddr + 0x55, cal->mod1Amplitude,            8);
    W25Qx_readData(baseAddr + 0x5D, &(cal->digAudioGain),          1);
    W25Qx_readData(baseAddr + 0x5E, &(cal->txDev_DTMF),            1);
    W25Qx_readData(baseAddr + 0x5F, &(cal->txDev_tone),            1);
    W25Qx_readData(baseAddr + 0x60, &(cal->txDev_CTCSS_wb),        1);
    W25Qx_readData(baseAddr + 0x61, &(cal->txDev_CTCSS_nb),        1);
    W25Qx_readData(baseAddr + 0x62, &(cal->txDev_DCS_wb),          1);
    W25Qx_readData(baseAddr + 0x63, &(cal->txDev_DCS_nb),          1);
    W25Qx_readData(baseAddr + 0x64, &(cal->PA_drv),                1);
    W25Qx_readData(baseAddr + 0x65, &(cal->PGA_gain),              1);
    W25Qx_readData(baseAddr + 0x66, &(cal->analogMicGain),         1);
    W25Qx_readData(baseAddr + 0x67, &(cal->rxAGCgain),             1);
    W25Qx_readData(baseAddr + 0x68, &(cal->mixGainWideband),       2);
    W25Qx_readData(baseAddr + 0x6A, &(cal->mixGainNarrowband),     2);
    W25Qx_readData(baseAddr + 0x6C, &(cal->rxDacGain),             1);
    W25Qx_readData(baseAddr + 0x6D, &(cal->rxVoiceGain),           1);

    uint8_t txPwr[32] = {0};
    W25Qx_readData(baseAddr + 0x0B, txPwr, 32);

    for(uint8_t i = 0; i < 16; i++)
    {
        cal->txLowPower[i]  = txPwr[2*i];
        cal->txHighPower[i] = txPwr[2*i+1];
    }
}

// Strings in GD-77 codeplug are terminated with 0xFF,
// replace 0xFF terminator with 0x00 to be compatible with C strings

void _addStringTerminator(char *buf, uint8_t max_len)
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

void nvm_init()
{
    W25Qx_init();
    AT24Cx_init();
}

void nvm_terminate()
{
    W25Qx_terminate();
    AT24Cx_terminate();
}

void nvm_readCalibData(void *buf)
{
    W25Qx_wakeup();
    delayUs(5);

    gdxCalibration_t *calib = ((gdxCalibration_t *) buf);

    _loadBandCalData(VHF_CAL_BASE, &(calib->data[0]));  /* Load VHF band calibration data */
    _loadBandCalData(UHF_CAL_BASE, &(calib->data[1]));  /* Load UHF band calibration data */

    W25Qx_sleep();

    /*
     * Finally, load calibration points. These are common among all the GDx
     * devices.
     * VHF calibration head and tail are not equally spaced as the other points,
     * so we manually override the values.
     */
    for(uint8_t i = 0; i < 16; i++)
    {
        uint8_t ii = i/2;
        calib->uhfCalPoints[ii]     = 405000000 + (5000000 * ii);
        calib->uhfPwrCalPoints[i]   = 400000000 + (5000000 * i);
    }

    for(uint8_t i = 0; i < 8; i++)
    {
        calib->vhfCalPoints[i] = 135000000 + (5000000 * i);
    }

    calib->vhfCalPoints[0] = 136000000;
    calib->vhfCalPoints[7] = 172000000;
}

void nvm_loadHwInfo(hwInfo_t *info)
{
    /* GDx devices does not have any hardware info in the external flash. */
    (void) info;
}

int nvm_readVFOChannelData(channel_t *channel)
{
    gdxChannel_t chData;

    AT24Cx_readData(vfoChannelBaseAddr, ((uint8_t *) &chData), sizeof(gdxChannel_t));

    // Copy data to OpenRTX channel_t
    channel->mode            = chData.channel_mode + 1;
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
        channel->dmr.rxColorCode       = chData.colorcode_rx;
        channel->dmr.txColorCode       = chData.colorcode_tx;
    }
    return 0;
}

int nvm_readChannelData(channel_t *channel, uint16_t pos)
{
    if((pos <= 0) || (pos > maxNumChannels))
        return -1;

    // Channels are organized in 128-channel banks
    uint8_t bank_num = (pos - 1) / 128;
    // Note: pos is 1-based because an empty slot in a zone contains index 0
    uint8_t bank_channel = (pos - 1) % 128;

    // ### Read channel bank bitmap ###
    uint8_t bitmap[16];
    // First channel bank (128 channels) is saved in EEPROM
    if(pos <= 128)
    {
        uint32_t readAddr = channelBaseAddrEEPROM + bank_num * sizeof(gdxChannelBank_t);
        AT24Cx_readData(readAddr, ((uint8_t *) &bitmap), sizeof(bitmap));
    }
    // Remaining 7 channel banks (896 channels) are saved in SPI Flash
    else
    {
        W25Qx_wakeup();
        delayUs(5);
        uint32_t readAddr = channelBaseAddrFlash + (bank_num - 1) * sizeof(gdxChannelBank_t);
        W25Qx_readData(readAddr, ((uint8_t *) &bitmap), sizeof(bitmap));
        W25Qx_sleep();
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
        uint32_t channelOffset = sizeof(bitmap) + (pos - 1) * sizeof(gdxChannel_t);
        // First channel bank (128 channels) is saved in EEPROM
        if(pos <= 128)
        {
            uint32_t bankAddr = channelBaseAddrEEPROM + bank_num * sizeof(gdxChannelBank_t);
            AT24Cx_readData(bankAddr + channelOffset, ((uint8_t *) &chData), sizeof(gdxChannel_t));
        }
        // Remaining 7 channel banks (896 channels) are saved in SPI Flash
        else
        {
            W25Qx_wakeup();
            delayUs(5);
            uint32_t bankAddr = channelBaseAddrFlash + bank_num * sizeof(gdxChannelBank_t);
            W25Qx_readData(bankAddr + channelOffset, ((uint8_t *) &chData), sizeof(gdxChannel_t));
            W25Qx_sleep();
        }
    }
    // Copy data to OpenRTX channel_t
    channel->mode            = chData.channel_mode + 1;
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
        channel->dmr.rxColorCode       = chData.colorcode_rx;
        channel->dmr.txColorCode       = chData.colorcode_tx;
    }
    return 0;
}

int nvm_readZoneData(zone_t *zone, uint16_t pos)
{
    if((pos <= 0) || (pos > maxNumZones)) return -1;

    // zone number is 1-based to be consistent with channels
    // Convert to 0-based index to fetch data from flash
    uint16_t index = pos - 1;
    // ### Read zone bank bitmap ###
    uint8_t bitmap[32];
    AT24Cx_readData(zoneBaseAddr, ((uint8_t *) &bitmap), sizeof(bitmap));

    uint8_t bitmap_byte = index / 8;
    uint8_t bitmap_bit = index % 8;
    // The zone is marked not valid in the bitmap
    if(!(bitmap[bitmap_byte] & (1 << bitmap_bit))) return -1;

    gdxZone_t zoneData;
    uint32_t zoneAddr = zoneBaseAddr + sizeof(bitmap) + index * sizeof(gdxZone_t);
    AT24Cx_readData(zoneAddr, ((uint8_t *) &zoneData), sizeof(gdxZone_t));

    // Check if zone is empty
    if(wcslen((wchar_t *) zoneData.name) == 0) return -1;

    memcpy(zone->name, zoneData.name, sizeof(zoneData.name));
    // Terminate string with 0x00 instead of 0xFF
    _addStringTerminator(zone->name, sizeof(zoneData.name));
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

    gdxContact_t contactData;
    // Note: pos is 1-based to be consistent with channels
    uint32_t contactAddr = contactBaseAddr + (pos - 1) * sizeof(gdxContact_t);
    W25Qx_readData(contactAddr, ((uint8_t *) &contactData), sizeof(gdxContact_t));
    W25Qx_sleep();

    // Check if contact is empty
    if(wcslen((wchar_t *) contactData.name) == 0) return -1;

    // Copy contact name
    memcpy(contact->name, contactData.name, sizeof(contactData.name));
    // Terminate string with 0x00 instead of 0xFF
    _addStringTerminator(contact->name, sizeof(contactData.name));
    // Copy contact DMR ID
    contact->id = (contactData.id[0] | contactData.id[1] << 8 | contactData.id[2] << 16);
    // Copy contact details
    contact->type = contactData.type;
    contact->receive_tone = contactData.receive_tone ? true : false;

    return 0;
}

int nvm_readSettings(settings_t *settings)
{
    (void) settings;
    return -1;
}

int nvm_writeSettings(const settings_t *settings)
{
    (void) settings;
    return -1;
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    (void) settings;
    (void) vfo;
    return -1;
}

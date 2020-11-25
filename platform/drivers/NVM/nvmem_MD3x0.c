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

#include <nvmem.h>
#include <delays.h>
#include "extFlash_MDx.h"
#include "calibInfo_MDx.h"

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

const uint32_t chDataBaseAddr = 0x1ee00; /**< Base address of channel data         */
const uint32_t maxNumChannels = 1000;    /**< Maximum number of channels in memory */

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
    extFlash_init();
}

void nvm_terminate()
{
    extFlash_terminate();
}

void nvm_readCalibData(void *buf)
{
    extFlash_wakeup();
    delayUs(5);

    md3x0Calib_t *calib = ((md3x0Calib_t *) buf);

    (void) extFlash_readSecurityRegister(0x1000, &(calib->vox1), 11);
    (void) extFlash_readSecurityRegister(0x1010, calib->txHighPower, 9);
    (void) extFlash_readSecurityRegister(0x1020, calib->txLowPower, 9);
    (void) extFlash_readSecurityRegister(0x1030, calib->rxSensitivity, 9);
    (void) extFlash_readSecurityRegister(0x1040, calib->openSql9, 9);
    (void) extFlash_readSecurityRegister(0x1050, calib->closeSql9, 9);
    (void) extFlash_readSecurityRegister(0x1060, calib->openSql1, 9);
    (void) extFlash_readSecurityRegister(0x1070, calib->closeSql1, 9);
    (void) extFlash_readSecurityRegister(0x1080, calib->maxVolume, 9);
    (void) extFlash_readSecurityRegister(0x1090, calib->ctcss67Hz, 9);
    (void) extFlash_readSecurityRegister(0x10a0, calib->ctcss151Hz, 9);
    (void) extFlash_readSecurityRegister(0x10b0, calib->ctcss254Hz, 9);
    (void) extFlash_readSecurityRegister(0x10c0, calib->dcsMod2, 9);
    (void) extFlash_readSecurityRegister(0x10d0, calib->dcsMod1, 9);
    (void) extFlash_readSecurityRegister(0x10e0, calib->mod1Partial, 9);
    (void) extFlash_readSecurityRegister(0x10f0, calib->analogVoiceAdjust, 9);

    (void) extFlash_readSecurityRegister(0x2000, calib->lockVoltagePartial, 9);
    (void) extFlash_readSecurityRegister(0x2010, calib->sendIpartial, 9);
    (void) extFlash_readSecurityRegister(0x2020, calib->sendQpartial, 9);
    (void) extFlash_readSecurityRegister(0x2030, calib->sendIrange, 9);
    (void) extFlash_readSecurityRegister(0x2040, calib->sendQrange, 9);
    (void) extFlash_readSecurityRegister(0x2050, calib->rxIpartial, 9);
    (void) extFlash_readSecurityRegister(0x2060, calib->rxQpartial, 9);
    (void) extFlash_readSecurityRegister(0x2070, calib->analogSendIrange, 9);
    (void) extFlash_readSecurityRegister(0x2080, calib->analogSendQrange, 9);

    uint32_t freqs[18];
    (void) extFlash_readSecurityRegister(0x20b0, ((uint8_t *) &freqs), 72);
    extFlash_sleep();

    for(uint8_t i = 0; i < 9; i++)
    {
        calib->rxFreq[i] = ((freq_t) _bcd2bin(freqs[2*i]));
        calib->txFreq[i] = ((freq_t) _bcd2bin(freqs[2*i+1]));
    }
}

int nvm_readChannelData(channel_t *channel, uint16_t pos)
{
    if(pos > maxNumChannels) return -1;

    extFlash_wakeup();
    delayUs(5);

    md3x0Channel_t chData;
    uint32_t readAddr = chDataBaseAddr + pos * sizeof(md3x0Channel_t);
    extFlash_readData(readAddr, ((uint8_t *) &chData), sizeof(md3x0Channel_t));
    extFlash_sleep();

    channel->mode            = chData.channel_mode - 1;
    channel->bandwidth       = chData.bandwidth;
    channel->admit_criteria  = chData.admit_criteria;
    channel->squelch         = chData.squelch;
    channel->rx_only         = chData.rx_only;
    channel->vox             = chData.vox;
    channel->power           = ((chData.power == 1) ? 5.0f : 1.0f);
    channel->rx_frequency    = _bcd2bin(chData.rx_frequency);
    channel->tx_frequency    = _bcd2bin(chData.tx_frequency);
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
        channel->fm.ctcDcs_rx = chData.ctcss_dcs_receive;
        channel->fm.ctcDcs_tx = chData.ctcss_dcs_transmit;
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

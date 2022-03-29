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

#ifndef NVMDATA_GDx_H
#define NVMDATA_GDx_H

#include <stdint.h>

/**
 * \internal Data structures matching the one used by original GDx firmware to
 * manage channel data inside nonvolatile flash memory.
 *
 * Taken by dmrconfig repository: https://github.com/sergev/dmrconfig/blob/master/gd77.c
 */
typedef struct {
    // Bytes 0-15
    uint8_t name[16];                   // Channel Name

    // Bytes 16-23
    uint32_t rx_frequency;              // RX Frequency: 8 digits BCD
    uint32_t tx_frequency;              // TX Frequency: 8 digits BCD

    // Byte 24
    uint8_t channel_mode;               // Mode: Analog or Digital

    // Bytes 25-26
    uint8_t _unused25[2];               // 0

    // Bytes 27-28
    uint8_t tot;                        // TOT x 15sec: 0-Infinite, 1=15s... 33=495s
    uint8_t tot_rekey_delay;            // TOT Rekey Delay: 0s...255s

    // Byte 29
    uint8_t admit_criteria;             // Admit Criteria: Always, Channel Free or Color Code

    // Bytes 30-31
    uint8_t _unused30;                  // 0x50
    uint8_t scan_list_index;            // Scan List: None, ScanList1...250

    // Bytes 32-35
    uint16_t ctcss_dcs_receive;         // CTCSS/DCS Dec: 4 digits BCD or 0xffff
    uint16_t ctcss_dcs_transmit;        // CTCSS/DCS Enc: 4 digits BCD

    // Bytes 36-39
    uint8_t _unused36;                  // 0
    uint8_t tx_signaling_syst;          // Tx Signaling System: Off, DTMF
    uint8_t _unused38;                  // 0
    uint8_t rx_signaling_syst;          // Rx Signaling System: Off, DTMF

    // Bytes 40-43
    uint8_t _unused40;                  // 0x16
    uint8_t privacy_group;              // Privacy Group: 0=None, 1=53474c39

    uint8_t colorcode_tx;               // Color Code: 0...15
    uint8_t group_list_index;           // Group List: None, GroupList1...128

    // Bytes 44-47
    uint8_t colorcode_rx;               // Color Code: 0...15
    uint8_t emergency_system_index;     // Emergency System: None, System1...32
    uint16_t contact_name_index;        // Contact Name: Contact1...

    // Byte 48
    uint8_t _unused48           : 6,    // 0
            emergency_alarm_ack : 1,    // Emergency Alarm Ack
            data_call_conf      : 1;    // Data Call Confirmed

    // Byte 49
    uint8_t private_call_conf   : 1,    // Private Call Confirmed
            _unused49_1         : 3,    // 0
            privacy             : 1,    // Privacy: Off or On
            _unused49_5         : 1,    // 0
            repeater_slot       : 1,    // Repeater Slot: 0=slot1 or 1=slot2
            _unused49_7         : 1;    // 0

    // Byte 50
    uint8_t dcdm                : 1,    // Dual Capacity Direct Mode
            _unused50_1         : 4,    // 0
            non_ste_frequency   : 1,    // Non STE = Frequency
            _unused50_6         : 2;    // 0

    // Byte 51
    uint8_t squelch             : 1,    // Squelch
            bandwidth           : 1,    // Bandwidth: 12.5 or 25 kHz

            rx_only             : 1,    // RX Only Enable
            talkaround          : 1,    // Allow Talkaround
            _unused51_4         : 2,    // 0
            vox                 : 1,    // VOX Enable
            power               : 1;    // Power: Low, High

    // Bytes 52-55
    uint8_t _unused52[4];               // 0
}
__attribute__((packed)) gdxChannel_t;

typedef struct {
    uint8_t bitmap[16];                 // bit set when channel valid
    gdxChannel_t chan[128];
}
__attribute__((packed)) gdxChannelBank_t;

// This corresponds to OpenGD77 extended zones
// TODO: Find a way to distinguish between a stock and OpenGD77 CPS
typedef struct {
    uint8_t name[16];                   // Zone Name
    uint16_t member[80];                // Member: channels 1...80
}
__attribute__((packed)) gdxZone_t;

typedef struct {
    uint8_t bitmap[32];                 // bit set when bank valid
    gdxZone_t bank[250];
}
__attribute__((packed)) gdxZoneBank_t;

typedef struct {
    // Bytes 0-15
    uint8_t name[16];                   // Contact Name, ff terminated

    // Bytes 16-19
    uint8_t id[4];                      // BCD coded 8 digits

    // Byte 20
    uint8_t type;                       // Call Type: Group Call, Private Call or All Call

    // Bytes 21-23
    uint8_t receive_tone;               // Call Receive Tone: 0=Off, 1=On
    uint8_t ring_style;                 // Ring style: 0-10
    uint8_t _unused23;                  // 0xff for used contact, 0 for blank entry
}
__attribute__((packed)) gdxContact_t;

#endif /* NVMDATA_GDx_H */

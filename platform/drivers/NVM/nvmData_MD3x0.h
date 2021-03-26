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

#ifndef NVMDATA_MD3x0_H
#define NVMDATA_MD3x0_H

#include <stdint.h>

/**
 * \internal Data structures matching the one used by original MD3x0 firmware to
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
__attribute__((packed)) md3x0Channel_t;

typedef struct
{
    // Bytes 0-31
    uint16_t name[16];      // Zone Name (Unicode)

    // Bytes 32-63
    uint16_t member[16];    // Member: channels 1...16
}
__attribute__((packed)) md3x0Zone_t;

typedef struct
{
    // Bytes 0-2
    uint8_t id[3];              // Call ID: 1...16777215

    // Byte 3
    uint8_t type         : 5,   // Call Type: Group Call, Private Call or All Call
            receive_tone : 1,   // Call Receive Tone: No or yes
            _unused2     : 2;   // 0b11

    // Bytes 4-35
    uint16_t name[16];          // Contact Name (Unicode)
}
__attribute__((packed)) md3x0Contact_t;

#endif /* NVMDATA_MD3x0_H */

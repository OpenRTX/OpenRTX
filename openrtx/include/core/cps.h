/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
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

#ifndef CPS_H
#define CPS_H

#include <stdint.h>
#include <stdbool.h>
#include <datatypes.h>
#include <rtx.h>

/**
 * \enum admit_t Enumeration type defining the admission criteria to a the
 * channel.
 */
enum admit_t
{
    ALWAYS = 0,  /**< Always transmit when PTT is pressed         */
    FREE   = 1,  /**< Transmit only if channel si free            */
    TONE   = 2,  /**< Transmit on matching tone                   */
    COLOR  = 3   /**< Transmit only if color code is not used yet */
};

/**
 * Data structure containing all and only the information for analog FM channels,
 * like CTC/DCS tones.
 */
#define MAX_TONE_INDEX 50
static const uint16_t ctcss_tone[MAX_TONE_INDEX] = {
    670, 693, 719, 744, 770, 797, 825, 854, 885, 915, 948, 974, 1000, 1034,
    1072, 1109, 1148, 1188, 1230, 1273, 1318, 1365, 1413, 1462, 1514, 1567,
    1598, 1622, 1655, 1679, 1713, 1738, 1773, 1799, 1835, 1862, 1899, 1928,
    1966, 1995, 2035, 2065, 2107, 2181, 2257, 2291, 2336, 2418, 2503, 2541
};

typedef struct
{
    uint8_t rxToneEn : 1, /**< RX CTC/DCS tone enable                        */
            rxTone   : 7; /**< RX CTC/DCS tone index, squelch opens on match */
    uint8_t txToneEn : 1, /**< TX CTC/DCS tone enable                        */
            txTone   : 7; /**< TX CTC/DCS tone index, sent alongside voice   */
}
__attribute__((packed)) fmInfo_t;

/**
 * Data structure containing all and only the information for DMR channels.
 */
typedef struct
{
    uint8_t  rxColorCode : 4,   /**< Color code for RX squelch opening        */
             txColorCode : 4;   /**< Color code sent during transmission      */

    uint8_t  dmr_timeslot;      /**< DMR timeslot, either 1 or 2              */
    uint16_t contactName_index; /**< Index to retrieve data from contact list */
}
__attribute__((packed)) dmrInfo_t;

/**
 * Data structure containing all and only the information for M17 channels.
 */
typedef struct
{
    uint8_t  rxCan : 4,         /**< Channel Access Number for RX squelch     */
             txCan : 4;         /**< Channel Access Number for TX squelch     */

    uint16_t contactName_index; /**< Index to retrieve data from contact list */
}
__attribute__((packed)) m17Info_t;

/**
 * Data structure containing all the information of a channel, either FM or DMR.
 */
typedef struct
{
    uint8_t mode;               /**< Operating mode                           */

    uint8_t bandwidth      : 2, /**< Bandwidth                                */
            admit_criteria : 2, /**< Admit criterion                          */
            squelch        : 1, /**< Squelch type: 0 = tight, 1 = normal      */
            rx_only        : 1, /**< 1 means RX-only channel                  */
            vox            : 1, /**< VOX enable                               */
            _padding       : 1; /**< Padding to 8 bits                        */

    float power;                /**< Transmission power, in watt              */

    freq_t rx_frequency;        /**< RX Frequency, in Hz                      */
    freq_t tx_frequency;        /**< TX Frequency, in Hz                      */

    uint8_t tot;                /**< TOT x 15sec: 0-Infinite, 1=15s...33=495s */
    uint8_t tot_rekey_delay;    /**< TOT Rekey Delay: 0...255s                */

    uint8_t emSys_index;        /**< Emergency System: None, System1...32     */
    uint8_t scanList_index;     /**< Scan List: None, ScanList1...250         */
    uint8_t groupList_index;    /**< Group List: None, GroupList1...128       */

    char name[16];              /**< Channel name                             */

    union
    {
        fmInfo_t  fm;           /**< Information block for FM channels        */
        dmrInfo_t dmr;          /**< Information block for DMR channels       */
        m17Info_t m17;          /**< Information block for M17 channels       */
    };
}
__attribute__((packed)) channel_t;


/**
 * Data structure containing all the information of a bank.
 */
typedef struct
{
    char name[16];              /**< Bank name                             */
    uint16_t member[64];        /**< Channel indexes                       */
}
__attribute__((packed)) bank_t;

/**
 * Data structure containing all the information of a contact.
 */
typedef struct
{
    char name[16];              /**< Contact name                             */
    uint32_t id;                /**< DMR ID: 24bit number, 1...16777215       */
    uint8_t type;               /**< Call Type: Group Call, Private Call or All Call */
    bool receive_tone;          /**< Call Receive Tone: No or yes             */
}
__attribute__((packed)) contact_t;

/*
 * Create and return a viable channel for this radio.
 * Suitable for default VFO settings or the creation of a new channel.
 * Needs to be generated by a function frequency settings require details from the running hardware on limitations
 */
channel_t get_default_channel();

#endif

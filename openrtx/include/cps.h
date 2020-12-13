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
#include <datatypes.h>

/**
 * \enum mode_t Enumeration type defining the operating mode associated to each
 * channel.
 */
enum mode_t
{
    FM  = 0,      /**< Analog FM mode */
    DMR = 1          /**< DMR mode */
};

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
 * \enum bw_t Enumeration type defining the bandwidth of a the channel.
 */
enum bw_t
{
    BW_12_5 = 0, /**< Bandwidth is 12.5kHz */
    BW_20   = 1, /**< Bandwidth is 20kHz   */
    BW_25   = 2  /**< Bandwidth is 25kHz   */
};

/**
 * Data structure containing all and only the information for analog FM channels,
 * like CTC/DCS tones.
 */
typedef struct
{
    uint16_t ctcDcs_rx;     /**< RX CTCSS or DCS code, squelch opens on match */
    uint16_t ctcDcs_tx;     /**< TX CTCSS or DCS code, sent alongside voice   */
} fmInfo_t;

/**
 * Data structure containing all and only the information for DMR channels.
 */
typedef struct
{
    uint8_t  rxColorCode : 4,   /**< Color code for RX squelch opening        */
             txColorCode : 4;   /**< Color code sent during transmission      */

    uint8_t  dmr_timeslot;      /**< DMR timeslot, either 1 or 2              */
    uint16_t contactName_index; /**< Index to retrieve data from contact list */
} dmrInfo_t;

/**
 * Data structure containing all the information of a channel, either FM or DMR.
 */
typedef struct
{
    uint8_t mode           : 1, /**< Operating mode                           */
            bandwidth      : 2, /**< Bandwidth                                */
            admit_criteria : 2, /**< Admit criterion                          */
            squelch        : 1, /**< Squelch type: 0 = tight, 1 = normal      */
            rx_only        : 1, /**< 1 means RX-only channel                  */
            vox            : 1; /**< VOX enable                               */

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
    };
} channel_t;

#endif

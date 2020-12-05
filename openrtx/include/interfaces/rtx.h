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

#ifndef RTX_H
#define RTX_H

#include <stdint.h>
#include <datatypes.h>

typedef struct
{
    uint8_t opMode    : 2,  /**< Operating mode (FM, DMR, ...) */
            bandwidth : 2,  /**< Channel bandwidth             */
            txDisable : 1,  /**< Disable TX operation          */
            _padding  : 3;

    freq_t rxFrequency;     /**< RX frequency, in Hz           */
    freq_t txFrequency;     /**< TX frequency, in Hz           */

    float txPower;          /**< TX power, in W                */
    uint8_t sqlLevel;       /**< Squelch opening level         */

    tone_t rxTone;          /**< RX CTC/DCS tone               */
    tone_t txTone;          /**< TX CTC/DCS tone               */
}
rtxConfig_t;

enum bandwidth
{
    BW_12_5 = 0,    /**< 12.5kHz bandwidth */
    BW_20   = 1,    /**< 20kHz bandwidth   */
    BW_25   = 2     /**< 25kHz bandwidth   */
};

enum opmode
{
    FM  = 0,        /**< Analog FM */
    DMR = 1         /**< DMR       */
};

enum opstatus
{
    OFF = 0,        /**< OFF          */
    RX  = 1,        /**< Receiving    */
    TX  = 2         /**< Transmitting */
};


/**
 * Initialise rtx stage
 */
void rtx_init();

/**
 * Shut down rtx stage
 */
void rtx_terminate();

/**
 * Post a new RTX configuration on the internal message queue. Data structure
 * \b must be heap-allocated and \b must not be modified after this function has
 * been called. The driver takes care of its deallocation.
 * @param cfg: pointer to a structure containing the new RTX configuration.
 */
void rtx_configure(const rtxConfig_t *cfg);

/**
 * High-level code is in charge of calling this function periodically, since it
 * contains all the RTX management functionalities.
 */
void rtx_taskFunc();

float rtx_getRssi();

#endif /* RTX_H */

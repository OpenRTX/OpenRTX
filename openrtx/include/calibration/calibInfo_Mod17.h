/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                         Mathis Schmieder DB9MAT                         *
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

#ifndef CALIBINFO_MOD17_H
#define CALIBINFO_MOD17_H

#include <datatypes.h>
#include <stdint.h>

/**
 * \brief Calibration data for Module17.
 */
typedef struct
{
    uint16_t tx_wiper;              ///< Baseband TX potentiometer
    uint16_t rx_wiper;              ///< Baseband RX potentiometer
    uint8_t  mic_gain;              ///< Microphone gain
    uint8_t  tx_invert     : 1,     ///< Invert TX baseband
             rx_invert     : 1,     ///< Invert RX baseband
             ptt_in_level  : 1,     ///< PTT in acive level
             ptt_out_level : 1,     ///< PTT out active level
             _padding      : 4;
}
mod17Calib_t;

#endif /* CALIBINFO_MOD17_H */

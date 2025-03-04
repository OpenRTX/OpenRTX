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
 *                                                                         *
 *   (2025) Modified by KD0OSS for DSTAR use in Module17/OpenRTX           *
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
    #if defined(CONFIG_DSTAR)
    uint8_t  dstar_tx_level;        ///< DSTAR TX level
    uint8_t  dstar_rx_level;        ///< DSTAR RX level
    #endif
    #if defined(CONFIG_P25)
    uint8_t  p25_tx_level;          ///< P25 TX level
    uint8_t  p25_rx_level;          ///< P25 RX level
    #endif
    uint8_t  fm_rx_level;           ///< FM RX level
    uint8_t  fm_tx_level;           ///< FM RX level
    uint8_t  mic_gain;              ///< Microphone gain
    uint8_t  ctcssrx_freq;          ///< CTCSS RX Freq index
    uint8_t  ctcsstx_freq;          ///< CTCSS TX Freq index
    uint8_t  ctcssrx_thrshhi;       ///< CTCSS RX upper threshold
    uint8_t  ctcssrx_thrshlo;       ///< CTCSS RX lower threshold
    uint8_t  ctcsstx_level;         ///< CTCSS TX level
    uint8_t  noisesq_thrshhi;       ///< FM noise squelch upper threshold
    uint8_t  noisesq_thrshlo;       ///< FM noise squelch lower threshold
    uint8_t  maxdev;                ///< Max FM TX deviation
    uint8_t  bb_tx_invert  : 1,     ///< Invert TX baseband
             bb_rx_invert  : 1,     ///< Invert RX baseband
             noisesq_on    : 1,   ///< Enable FM noise squelch
             ptt_in_level  : 1,     ///< PTT in acive level
             ptt_out_level : 1,     ///< PTT out active level
             _padding      : 3;
}
mod17Calib_t;

#endif /* CALIBINFO_MOD17_H */

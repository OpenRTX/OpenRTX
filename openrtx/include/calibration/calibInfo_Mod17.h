/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CALIBINFO_MOD17_H
#define CALIBINFO_MOD17_H

#include "core/datatypes.h"
#include <stdint.h>

/**
 * \brief Calibration data for Module17.
 */
typedef struct
{
    uint16_t tx_wiper;              ///< Baseband TX potentiometer
    uint16_t rx_wiper;              ///< Baseband RX potentiometer
    uint8_t  mic_gain;              ///< Microphone gain
    uint8_t  bb_tx_invert  : 1,     ///< Invert TX baseband
             bb_rx_invert  : 1,     ///< Invert RX baseband
             ptt_in_level  : 1,     ///< PTT in acive level
             ptt_out_level : 1,     ///< PTT out active level
             _padding      : 4;
}
mod17Calib_t;

#endif /* CALIBINFO_MOD17_H */

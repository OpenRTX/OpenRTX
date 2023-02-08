/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#ifndef AT1846S_WRAPPER_H
#define AT1846S_WRAPPER_H

#include <stdint.h>
#include <stdbool.h>
#include <datatypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This file provides a C-callable wrapper for the AT1846S driver, which is
 * written in C++.
 */

/**
 * \enum AT1846S_bw_t Enumeration type defining the bandwidth settings supported
 * by the AT1846S chip.
 */
typedef enum
{
    AT1846S_BW_12P5 = 0,
    AT1846S_BW_25   = 1
}
AT1846S_bw_t;

/**
 * \enum AT1846S_op_t Enumeration type defining the possible operating mode
 * configurations for the AT1846S chip.
 */
typedef enum
{
    AT1846S_OP_FM  = 0,
    AT1846S_OP_DMR = 1
}
AT1846S_op_t;

/**
 * \enum AT1846S_func_t Enumeration type defining the AT1846S functional modes.
 */
typedef enum
{
    AT1846S_OFF = 0,
    AT1846S_RX  = 1,
    AT1846S_TX  = 2,
}
AT1846S_func_t;

/**
 * Initialise the AT146S chip.
 */
void AT1846S_init();

/**
 * Shut down the AT146S chip.
 */
void AT1846S_terminate();

/**
 * Set the VCO frequency, either for transmission or reception.
 * @param freq: VCO frequency.
 */
void AT1846S_setFrequency(const freq_t freq);

/**
 * Set the transmission and reception bandwidth.
 * @param band: bandwidth, from \enum AT1846S_bw_t.
 */
void AT1846S_setBandwidth(const AT1846S_bw_t band);

/**
 * Set the operating mode.
 * @param mode: operating mode, from \enum AT1846S_op_t.
 */
void AT1846S_setOpMode(const AT1846S_op_t mode);

/**
 * Set the functional mode.
 * @param mode: functional mode, from \enum AT1846S_func_t.
 */
void AT1846S_setFuncMode(const AT1846S_func_t mode);

/**
 * Enable the CTCSS tone for transmission.
 * @param freq: CTCSS tone frequency.
 */
void AT1846S_enableTxCtcss(const tone_t freq);

/**
 * Turn off both transmission CTCSS tone and reception CTCSS tone decoding.
 */
void AT1846S_disableCtcss();

/**
 * Get current RSSI value.
 * @return current RSSI in dBm.
 */
int16_t AT1846S_readRSSI();

/**
 * Set the gain of internal programmable gain amplifier.
 * @param gain: PGA gain.
 */
void AT1846S_setPgaGain(const uint8_t gain);

/**
 * Set microphone gain for transmission.
 * @param gain: microphone gain.
 */
void AT1846S_setMicGain(const uint8_t gain);

/**
 * Set maximum FM transmission deviation.
 * @param dev: maximum allowed deviation.
 */
void AT1846S_setTxDeviation(const uint16_t dev);

/**
 * Set the gain for internal automatic gain control system.
 * @param gain: AGC gain.
 */
void AT1846S_setAgcGain(const uint8_t gain);

/**
 * Set audio gain for recepion.
 * @param gainWb: gain for wideband Rx (25kHz).
 * @param gainNb: gain for narrowband Rx (12.5kHz).
 */
void AT1846S_setRxAudioGain(const uint8_t gainWb, const uint8_t gainNb);

/**
 * Set noise1 thresholds for squelch opening and closing.
 * @param highTsh: upper threshold.
 * @param lowTsh: lower threshold.
 */
void AT1846S_setNoise1Thresholds(const uint8_t highTsh, const uint8_t lowTsh);

/**
 * Set noise2 thresholds for squelch opening and closing.
 * @param highTsh: upper threshold.
 * @param lowTsh: lower threshold.
 */
void AT1846S_setNoise2Thresholds(const uint8_t highTsh, const uint8_t lowTsh);

/**
 * Set RSSI thresholds for squelch opening and closing.
 * @param highTsh: upper threshold.
 * @param lowTsh: lower threshold.
 */
void AT1846S_setRssiThresholds(const uint8_t highTsh, const uint8_t lowTsh);

/**
 * Set PA drive control bits.
 * @param value: PA drive value.
 */
void AT1846S_setPaDrive(const uint8_t value);

/**
 * Set threshold for analog FM squelch opening.
 * @param thresh: squelch threshold.
 */
void AT1846S_setAnalogSqlThresh(const uint8_t thresh);

#ifdef __cplusplus
}
#endif

#endif /* AT1846S_WRAPPER_H */

/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
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

#ifndef CALIB_UTILS_H
#define CALIB_UTILS_H

#include <datatypes.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function allows to obtain the value of a given calibration parameter for
 * frequencies outside the calibration points. It works by searching the two
 * calibration points containing the target frequency and then by linearly
 * interpolating the calibration parameter among these two points.
 *
 * @param freq: target frequency for which a calibration value has to be
 * computed.
 * @param calPoints: pointer to the vector containing the frequencies of the
 * calibration points.
 * @param param: pointer to the vector containing the values for the calibration
 * parameter, it must have the same length of the one containing the frequencies
 * of calibration points.
 * @param elems: number of elements of both the vectors for calibration parameter
 * and frequencies.
 * @return value for the calibration parameter at the given frequency point.
 */
uint8_t interpCalParameter(const freq_t freq, const freq_t *calPoints,
                           const uint8_t *param, const uint8_t elems);

/**
 * Convert from "OpenRTX dBm" to watt.
 * In OpenRTX cps power is stored as the coefficient n of the equation
 * P(dBm) = 10 + 2*0.2.
 *
 * @param n: coefficient of the dBm equation.
 * @return power in watt.
 */
float dBmToWatt(const uint8_t n);

/**
 * \internal Utility function to convert 4 byte BCD values into a 32-bit
 * unsigned integer ones.
 */
uint32_t bcd2bin(uint32_t bcd);

#ifdef __cplusplus
}
#endif

#endif /* CALIB_UTILS_H */

/***************************************************************************
 *   Copyright (C) 2020 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef UTILS_H
#define UTILS_H

#include <datatypes.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get the number of elements of an array.
 *
 * @param x: array.
 * @return number of elements.
 */
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

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
 * Convert a 4 byte BCD number to a 32-bit unsigned integer one.
 *
 * @param bcd: BCD number
 * @return unsigned integer representation of the BCD input.
 */
uint32_t bcdToBin(uint32_t bcd);

/**
 * Given a string containing a number expressed in decimal notation, remove all
 * the unnecessary trailing zeroes. I.e. the string "123.4560000" will be trimmed
 * down to "123.456". This function requires that the input string has at least
 * one decimal point and proceeds stripping the zeroes from the end to the beginning.
 *
 * @param str: string to be processed.
 */
void stripTrailingZeroes(char *str);

/**
 * Get the S-level corresponding to a given RSSI value in dBm.
 *
 * @param rssi: RSSI in dBm
 * @return S level, from S0 to S11
 */
uint8_t rssiToSlevel(const rssi_t rssi);

/**
 * Retrieve the CTCSS tone index given its frequency in tenths of Hz.
 *
 * @param freq: CTCSS frequency
 * @return tone index or 255 if the tone has not been found
 */
uint8_t ctcssFreqToIndex(const uint16_t freq);

#ifdef __cplusplus
}
#endif

#endif /* CALIB_UTILS_H */

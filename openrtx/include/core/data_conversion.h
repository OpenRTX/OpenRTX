/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef DATA_CONVERSION_H
#define DATA_CONVERSION_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * In-place conversion of data elements from int16_t to unsigned 16 bit values
 * ranging from 0 to 4095.
 *
 * @param buffer: data buffer.
 * @param length: buffer length, in elements.
 */
void S16toU12(int16_t *buffer, const size_t length);

/**
 * In-place conversion of data elements from int16_t to unsigned 8 bit values
 * ranging from 0 to 255.
 *
 * @param buffer: data buffer.
 * @param length: buffer length, in elements.
 */
void S16toU8(int16_t *buffer, const size_t length);

#ifdef __cplusplus
}
#endif

#endif /* DATA_CONVERSION_H */

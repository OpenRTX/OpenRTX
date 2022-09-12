/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
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

#ifndef SLIP_H
#define SLIP_H

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Frame context for SLIP encoding and decoding.
 */
struct FrameCtx
{
    uint8_t *data;      ///< Output data buffer.
    size_t   maxLen;    ///< Maximum number of bytes storable in the output data buffer.
    size_t   oPos;      ///< Current index of output data buffer.
    size_t   iPos;      ///< Current index of input data buffer.
};

/**
 * Setup the SLIP frame context for a new frame encode or decode operation.
 *
 * @param frame: frame context.
 * @param buf: frame data buffer.
 * @param size: size of the frame data buffer.
 */
static inline void slip_initFrame(struct FrameCtx *frame, const void *buf,
                                  const size_t size)
{
    frame->data   = (uint8_t *) buf;
    frame->maxLen = size;
    frame->iPos   = 0;
    frame->oPos   = 0;
}

/**
 * Get the resulting data from a SLIP encode or decode operation and prepare the
 * frame context for a new operation.
 *
 * @param frame: frame context.
 * @param data: pointer to the result.
 * @return data lenght in bytes.
 */
static inline size_t slip_popFrame(struct FrameCtx *frame, void **data)
{
    size_t ret  = frame->oPos;
    frame->oPos = 0;
    *data = (void *) frame->data;

    return ret;
}

/**
 * Encode a block of data according to the SLIP protocol.
 * The frame start is automatically prepended to the first data block encoded,
 * the frame end marker is appended only when explicitly indicated by the caller.
 *
 * @param frame: frame context.
 * @param data: data to be encoded.
 * @param len: length of orginal data, in bytes.
 * @param last: set to true to append the frame end marker.
 * @return cumulative size of the output buffer or a negative error code in case
 * of errors.
 */
int slip_encode(struct FrameCtx *frame, const void *data, const size_t len,
                const bool last);

/**
 * Decode a block of data encoded following the SLIP protocol.
 * The function returns either when the input buffer is empty or a frame end
 * marker is detected.
 *
 * @param frame: frame context.
 * @param data: data to be decoded.
 * @param len: length of input data, in bytes.
 * @param end: pointer to a boolean flag set to true when a frame end marker is
 * found.
 * @return number of bytes effectively read from the input buffer or a negative
 * error code in case of errors.
 */
int slip_decode(struct FrameCtx *frame, const void *data, const size_t len,
                bool *end);

#ifdef __cplusplus
}
#endif

#endif

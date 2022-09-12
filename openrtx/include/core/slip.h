/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SLIP_H
#define SLIP_H

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SLIP state object data structure.
 */
struct slip {
    uint8_t *buf;
    size_t len;
    size_t idx;
    uint8_t state;
    uint8_t inFrame;
};

/**
 * Declare a SLIP state object.
 *
 * @param name: SLIP object name.
 * @param size: size of the frame data buffer.
 */
#define SLIP_OBJECT_DECLARE(name, size)   \
    static uint8_t slip_buf_##name[size]; \
    static struct slip name = {           \
        .buf = slip_buf_##name,           \
        .len = size,                      \
        .idx = 0,                         \
        .state = 0,                       \
        .inFrame = 0,                     \
    };

/**
 * Initialize a SLIP state object.
 *
 * @param sl: pointer to SLIP object.
 * @param buf: pointer to frame data buffer.
 * @param size: size of the frame data buffer.
 * @return zero on success or a negative error code.
 */
int slip_init(struct slip *sl, void *buf, const size_t size);

/**
 * Allocate a SLIP state object.
 *
 * @param size: size of the frame data buffer.
 * @return pointer to an heap-allocated SLIP object.
 */
struct slip *slip_alloc(const size_t size);

/**
 * Release an heap-allocated SLIP state object.
 *
 * @param sl: pointer to SLIP object.
 */
void slip_free(struct slip *sl);

/**
 * Reset a SLIP state object.
 *
 * @param sl: pointer to SLIP object.
 */
void slip_reset(struct slip *sl);

/**
 * Access data managed by a SLIP state object.
 *
 * @param sl: pointer to SLIP object.
 * @param len: pointer to data length.
 * @return pointer to SLIP object data buffer.
 */
void *slip_getData(const struct slip *sl, size_t *len);

/**
 * Encode a block of data according to the SLIP protocol.
 * This function supports incremental operation and may be called repeatedly
 * with small output buffers until the entire frame has been emitted. The full
 * payload has been encoded when the return value is zero. Application code
 * must reset the internal state before encoding a new payload block.
 *
 * @param sl: pointer to SLIP object.
 * @param data: pointer to encoded data buffer.
 * @param len: size of encoded data buffer.
 * @return number of bytes written to the encoded data buffer or a negative
 * error code.
 */
int slip_encode(struct slip *sl, void *data, const size_t len);

/**
 * Decode and append a data byte according to the SLIP protocol.
 *
 * @param sl: pointer to SLIP object.
 * @param byte: incoming data byte.
 * @return < 0: Error
 *         = 0: Byte decoded
 *         > 0: Length of decoded data
 */
int slip_decodeByte(struct slip *sl, const uint8_t byte);

/**
 * Decode a block of data encoded following the SLIP protocol.
 * The function returns either when the input buffer is empty or a frame end
 * marker is detected.
 *
 * @param sl: pointer to SLIP object.
 * @param data: data to be decoded.
 * @param len: length of input data, in bytes.
 * @return number of bytes consumed from the input buffer or a negative error
 * code.
 */
int slip_decode(struct slip *sl, const void *data, const size_t len);

#ifdef __cplusplus
}
#endif

#endif

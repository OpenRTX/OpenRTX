/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include "core/slip.h"

enum slipFlags {
    END = 0xC0,
    ESC = 0xDB,
    ESC_END = 0xDC,
    ESC_ESC = 0xDD,
};

enum decodeState {
    DECODE_STATE_MESSAGE,
    DECODE_STATE_ESCAPE,
    DECODE_STATE_ERROR,
};

int slip_init(struct slip *sl, void *buf, const size_t size)
{
    if ((sl == NULL) || (buf == NULL))
        return -EINVAL;

    sl->buf = (uint8_t *)buf;
    sl->len = size;
    slip_reset(sl);

    return 0;
}

struct slip *slip_alloc(const size_t size)
{
    struct slip *sl;
    uint8_t *mem;

    mem = malloc(sizeof(struct slip) + size);
    if (!mem)
        return NULL;

    sl = (struct slip *)mem;
    sl->buf = mem + sizeof(struct slip);
    sl->len = size;
    slip_reset(sl);

    return sl;
}

void slip_free(struct slip *sl)
{
    free(sl);
}

void slip_reset(struct slip *sl)
{
    if (!sl)
        return;

    sl->idx = 0;
    sl->state = DECODE_STATE_MESSAGE;
    sl->inFrame = 0;
}

void *slip_getData(const struct slip *sl, size_t *len)
{
    if (!sl)
        return NULL;

    if (len)
        *len = sl->idx;

    return sl->buf;
}

int slip_encode(struct slip *sl, void *data, const size_t len)
{
    size_t outIdx = 0;
    uint8_t *outBuf = (uint8_t *)data;

    if ((sl == NULL) || (data == NULL) || (len < 1))
        return -EINVAL;

    while (outIdx < len) {
        uint8_t cur;

        // Prepend a frame start marker when beginning a new frame
        if (sl->idx == 0) {
            outBuf[outIdx] = END;
            outIdx += 1;
            sl->inFrame = 1;
        }

        // End of payload: append EOF marker
        if (sl->idx == sl->len) {
            if (sl->inFrame) {
                outBuf[outIdx] = END;
                outIdx += 1;
                sl->inFrame = 0;
            }

            break;
        }

        cur = sl->buf[sl->idx];
        sl->idx += 1;

        if ((cur == ESC) || (cur == END)) {
            if ((outIdx + 2) > len)
                break;

            outBuf[outIdx] = ESC;
            outBuf[outIdx + 1] = (cur == ESC) ? ESC_ESC : ESC_END;
            outIdx += 2;
        } else {
            outBuf[outIdx] = cur;
            outIdx += 1;
        }
    }

    return outIdx;
}

int slip_decodeByte(struct slip *sl, const uint8_t byte)
{
    if (!sl)
        return -EINVAL;

    if (sl->state == DECODE_STATE_MESSAGE) {
        switch (byte) {
            case END:
                // SOF or EOF delimiter
                if (sl->idx != 0)
                    return sl->idx;
                break;

            case ESC:
                sl->state = DECODE_STATE_ESCAPE;
                break;

            default:
                if (sl->idx >= sl->len) {
                    return -ENOBUFS;
                } else {
                    sl->buf[sl->idx] = byte;
                    sl->idx += 1;
                }
                break;
        }
    } else if (sl->state == DECODE_STATE_ESCAPE) {
        if (sl->idx >= sl->len)
            return -ENOBUFS;

        switch (byte) {
            case ESC_END:
                sl->buf[sl->idx] = END;
                sl->idx += 1;
                sl->state = DECODE_STATE_MESSAGE;
                break;

            case ESC_ESC:
                sl->buf[sl->idx] = ESC;
                sl->idx += 1;
                sl->state = DECODE_STATE_MESSAGE;
                break;

            default:
                sl->state = DECODE_STATE_ERROR;
                return -EPROTO;
                break;
        }
    } else {
        // Decoder in error state
        return -EPERM;
    }

    return 0;
}

int slip_decode(struct slip *sl, const void *data, const size_t len)
{
    uint8_t *buf = (uint8_t *)data;
    size_t i;

    if ((sl == NULL) || (data == NULL))
        return -EINVAL;

    for (i = 0; i < len; i++) {
        int ret = slip_decodeByte(sl, buf[i]);
        if (ret < 0)
            // Decoding error
            return ret;
        else if (ret > 0)
            // Frame end
            return i + 1;
    }

    return i;
}

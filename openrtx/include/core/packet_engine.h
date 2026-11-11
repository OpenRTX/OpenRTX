/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PACKET_ENGINE_H
#define PACKET_ENGINE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Send one received frame to the host as a SLIP-framed KISS data frame on
 * the virtual serial port, in a single write so nothing else written to the
 * port can land inside it.
 *
 * @param frame: the frame, an AX.25 frame without its frame check sequence.
 * @param len: frame length in bytes.
 * @return 0 on success, a negative error code when the frame was not sent,
 * for example because no host is reading.
 */
int packetEngine_send(const uint8_t *frame, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* PACKET_ENGINE_H */

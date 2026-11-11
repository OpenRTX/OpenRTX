/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef APRS_CONSTANTS_H
#define APRS_CONSTANTS_H

/* AX.25 frame limits: a destination, a source and up to eight digipeaters of
 * seven bytes each, the control and PID bytes, an info field of up to 256
 * bytes (N1) and the two-byte FCS, which the decoder holds too. */
#define APRS_MAX_ADDRESSES 10
#define APRS_ADDR_LEN 7
#define APRS_CTRL_PID_LEN 2
#define APRS_MAX_INFO 256
#define APRS_FCS_LEN 2
#define APRS_PACLEN                                                           \
    ((APRS_MAX_ADDRESSES * APRS_ADDR_LEN) + APRS_CTRL_PID_LEN + APRS_MAX_INFO \
     + APRS_FCS_LEN)

#define APRS_BUF_SIZE 64
#define APRS_SAMPLE_RATE 9600
#define APRS_SYMBOL_RATE 1200

#endif /* APRS_CONSTANTS_H */

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef APRS_CONSTANTS_H
#define APRS_CONSTANTS_H

#define APRS_BUF_SIZE 64
#define APRS_SAMPLE_RATE 9600
#define APRS_SYMBOL_RATE 1200

/* #define APRS_DEBUG */

#ifdef APRS_DEBUG
#include <cstdio>
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

#endif /* APRS_CONSTANTS_H */

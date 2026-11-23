/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PADDLE_H
#define PADDLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * CW paddle input, implemented by targets defining CONFIG_CW. The paddle may
 * share its pins with another function, such as a serial port, so the CW
 * operating mode takes it over in paddle_init() and gives it back in
 * paddle_terminate().
 */

/**
 * Take over the paddle input.
 */
void paddle_init(void);

/**
 * Give the paddle pins back to their other function.
 */
void paddle_terminate(void);

/**
 * Read the paddle contacts.
 *
 * @return paddles closed, enum cwPaddle bits of core/cw_keyer.h.
 */
uint8_t paddle_read(void);

#ifdef __cplusplus
}
#endif

#endif // PADDLE_H

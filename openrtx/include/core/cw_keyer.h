/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CW_KEYER_H
#define CW_KEYER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Paddle contacts, as a bit mask.
 */
enum cwPaddle {
    CW_PADDLE_DIT = 1, ///< Dit paddle closed.
    CW_PADDLE_DAH = 2, ///< Dah paddle closed.
};

/**
 * Iambic keyer, mode B: an element runs to its end once started, and the
 * opposite paddle pressed during an element or the space after it is
 * remembered and sent next. Squeezing both paddles alternates dits and dahs,
 * starting with a dit.
 */
struct cwKeyer {
    long long end;    ///< End of the current element or space, in ms.
    uint16_t ditTime; ///< Dit length, in ms.
    uint8_t state;    ///< Keyer state.
    uint8_t element;  ///< Element being sent or last sent, enum cwPaddle.
    uint8_t memory;   ///< Opposite paddle pressed meanwhile, enum cwPaddle.
};

/**
 * Initialise the keyer, idle.
 *
 * @param keyer: keyer state.
 * @param wpm: speed, in words per minute (PARIS), at least 1.
 */
void cwKeyer_init(struct cwKeyer *keyer, uint8_t wpm);

/**
 * Change the speed, from the next element on.
 *
 * @param keyer: keyer state.
 * @param wpm: speed, in words per minute (PARIS), at least 1.
 */
void cwKeyer_setSpeed(struct cwKeyer *keyer, uint8_t wpm);

/**
 * Advance the keyer. Call it periodically, at a period well below a dit: the
 * element timing follows the calls, not the period.
 *
 * @param keyer: keyer state.
 * @param paddles: paddles closed now, enum cwPaddle bits.
 * @param now: current time, in ms.
 * @return true while the key is down.
 */
bool cwKeyer_update(struct cwKeyer *keyer, uint8_t paddles, long long now);

#ifdef __cplusplus
}
#endif

#endif // CW_KEYER_H

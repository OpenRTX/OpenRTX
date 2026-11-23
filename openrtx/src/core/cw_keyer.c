/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/cw_keyer.h"

/*
 * PARIS timing: a dit lasts 1200 ms divided by the speed in words per
 * minute, a dah three dits, and the space between elements one dit.
 */
#define DIT_MS_WPM 1200
#define DAH_DITS 3

enum keyerState {
    KEYER_IDLE,  ///< No element, key up.
    KEYER_MARK,  ///< Sending an element, key down.
    KEYER_SPACE, ///< Space after an element, key up.
};

/**
 * Start an element where the previous one ended.
 *
 * @param keyer: keyer state.
 * @param element: element to send, CW_PADDLE_DIT or CW_PADDLE_DAH.
 * @param start: start time, in ms.
 */
static void startElement(struct cwKeyer *keyer, uint8_t element,
                         long long start)
{
    uint16_t length = keyer->ditTime;

    if (element == CW_PADDLE_DAH)
        length *= DAH_DITS;

    keyer->element = element;
    keyer->memory = 0;
    keyer->end = start + length;
    keyer->state = KEYER_MARK;
}

void cwKeyer_init(struct cwKeyer *keyer, uint8_t wpm)
{
    keyer->end = 0;
    keyer->state = KEYER_IDLE;
    keyer->element = CW_PADDLE_DAH;
    keyer->memory = 0;
    cwKeyer_setSpeed(keyer, wpm);
}

void cwKeyer_setSpeed(struct cwKeyer *keyer, uint8_t wpm)
{
    keyer->ditTime = DIT_MS_WPM / wpm;
}

bool cwKeyer_update(struct cwKeyer *keyer, uint8_t paddles, long long now)
{
    uint8_t opposite = keyer->element ^ (CW_PADDLE_DIT | CW_PADDLE_DAH);

    switch (keyer->state) {
        case KEYER_IDLE:
            // A squeeze starts with a dit
            if (paddles & CW_PADDLE_DIT)
                startElement(keyer, CW_PADDLE_DIT, now);
            else if (paddles & CW_PADDLE_DAH)
                startElement(keyer, CW_PADDLE_DAH, now);
            break;

        case KEYER_MARK:
            keyer->memory |= paddles & opposite;
            if (now >= keyer->end) {
                keyer->end += keyer->ditTime;
                keyer->state = KEYER_SPACE;
            }
            break;

        case KEYER_SPACE:
            keyer->memory |= paddles & opposite;
            if (now < keyer->end)
                break;

            if (keyer->memory != 0)
                startElement(keyer, opposite, keyer->end);
            else if (paddles & keyer->element)
                startElement(keyer, keyer->element, keyer->end);
            else
                keyer->state = KEYER_IDLE;
            break;
    }

    return keyer->state == KEYER_MARK;
}

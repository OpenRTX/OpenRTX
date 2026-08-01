/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * @file rtttl.h
 * @brief Minimal RTTTL ringtone string parser.
 *
 * RTTTL ("Ring Tone Text Transfer Language") encodes a melody as a compact
 * ASCII string: a name, a set of defaults, and a comma-separated note list.
 * This parser is pure logic with no hardware dependencies (no timers, no
 * audio calls), so it can be unit tested directly and reused by any future
 * notification/tone consumer.
 */

#ifndef RTTTL_H
#define RTTTL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of notes a single RTTTL string can decode to. Chosen to
 * comfortably fit every tone in this project's tone table; parsing simply
 * stops (without failing) if a string has more notes than this. */
#define RTTTL_MAX_NOTES 64

/**
 * @brief A single decoded note: a frequency and a duration.
 */
struct rtttl_note {
    uint16_t freq_hz;     /**< Tone frequency in Hz, or 0 for a pause/rest. */
    uint16_t duration_ms; /**< Note (or pause) duration in milliseconds. */
};

/**
 * @brief Parse an RTTTL string into a sequence of notes.
 *
 * Format: "name:d=N,o=N,b=N:note1,note2,...".  Each note is
 * `[duration]pitch[#][octave][.]` or `[duration]P[.]` for a pause.
 *
 * @param rtttl:     NUL-terminated RTTTL string.
 * @param notes:     output array of at least RTTTL_MAX_NOTES entries.
 * @param out_count: set to the number of notes parsed (0 on failure).
 * @return true if at least one note was parsed, false on malformed input.
 */
bool rtttl_parse(const char *rtttl, struct rtttl_note *notes,
                 uint8_t *out_count);

#ifdef __cplusplus
}
#endif

#endif /* RTTTL_H */

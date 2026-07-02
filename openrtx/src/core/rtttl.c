/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/rtttl.h"
#include <stdlib.h>

/* ------------------------------------------------------------------
 * Frequency lookup — 12 pitch classes × octaves 4..7
 * ------------------------------------------------------------------ */

static const uint16_t note_freq[12][4] = {
    { 262, 523, 1047, 2093 }, /* C   */
    { 277, 554, 1109, 2217 }, /* C#  */
    { 294, 587, 1175, 2349 }, /* D   */
    { 311, 622, 1245, 2489 }, /* D#  */
    { 330, 659, 1319, 2637 }, /* E   */
    { 349, 698, 1397, 2794 }, /* F   */
    { 370, 740, 1480, 2960 }, /* F#  */
    { 392, 784, 1568, 3136 }, /* G   */
    { 415, 831, 1661, 3322 }, /* G#  */
    { 440, 880, 1760, 3520 }, /* A   */
    { 466, 932, 1865, 3729 }, /* A#  */
    { 494, 988, 1976, 3951 }, /* B   */
};

/* Map note letter -> pitch-class index.
 * 'A'..'G' map to 9,11,0,2,4,5,7  (A,B,C,D,E,F,G). */
static int pitch_class_of_char(char c)
{
    switch (c) {
        case 'C':
        case 'c':
            return 0;
        case 'D':
        case 'd':
            return 2;
        case 'E':
        case 'e':
            return 4;
        case 'F':
        case 'f':
            return 5;
        case 'G':
        case 'g':
            return 7;
        case 'A':
        case 'a':
            return 9;
        case 'B':
        case 'b':
            return 11;
        default:
            return -1;
    }
}

/* Parse a "key=<int>" default-field assignment (e.g. "d=8").  On a match,
 * store the value in *out, advance *sp past the digits, and return true.
 * Leaves *sp untouched and returns false when the field does not match.
 * The caller guarantees **sp is non-NUL, so reading (*sp)[1] is safe. */
static bool parse_kv_int(const char **sp, char key, int *out)
{
    const char *s = *sp;
    if (s[0] != key || s[1] != '=')
        return false;
    s += 2;
    *out = atoi(s);
    while (*s >= '0' && *s <= '9')
        s++;
    *sp = s;
    return true;
}

/* ------------------------------------------------------------------
 * RTTTL parser
 *
 * Format:
 *   name:d=N,o=N,b=N:note1,note2,...
 *
 * Each note:  [dur]pitch[octave][.]
 * or pause:   P[dur]
 * ------------------------------------------------------------------ */

bool rtttl_parse(const char *s, struct rtttl_note *notes, uint8_t *out_count)
{
    uint8_t count = 0;

    /* ---- skip name field ---- */
    while (*s && *s != ':')
        s++;
    if (!*s)
        return false;
    s++; /* skip ':' */

    /* ---- parse defaults ---- */
    int def_dur = 4; /* default note length (1=whole, 2=half, 4=quarter …) */
    int def_oct = 5; /* default octave */
    int bpm = 120;   /* beats per minute */

    while (*s && *s != ':') {
        if (parse_kv_int(&s, 'd', &def_dur))
            continue;
        if (parse_kv_int(&s, 'o', &def_oct))
            continue;
        if (parse_kv_int(&s, 'b', &bpm))
            continue;
        s++;
    }
    if (!*s)
        return false;
    s++; /* skip ':' before note list */

    /* Clamp the parsed defaults so malformed or hostile header fields can't
     * reach the duration math below.  def_dur is used as an unsigned divisor
     * when a note omits its own duration, so a zero or negative default
     * would divide by zero / underflow to a garbage duration; a non-positive
     * bpm makes the quarter-note duration meaningless.  The per-note paths
     * already guard their explicit duration (see below) and octave, but the
     * defaults were previously unchecked. */
    if (def_dur <= 0)
        def_dur = 4;
    if (def_oct < 4)
        def_oct = 4;
    if (def_oct > 7)
        def_oct = 7;
    if (bpm <= 0)
        return false;

    /* quarter-note duration in ms */
    unsigned int qn_ms = (60000u + (bpm / 2u)) / (unsigned int)bpm;

    /* ---- parse notes ---- */
    while (*s && count < RTTTL_MAX_NOTES) {
        while (*s == ',' || *s == ' ')
            s++;
        if (!*s)
            break;

        int dur_val = def_dur;
        int octave = def_oct;
        bool dotted = false;
        int freq = 0;

        /* optional explicit duration */
        if (*s >= '0' && *s <= '9') {
            dur_val = atoi(s);
            while (*s && *s >= '0' && *s <= '9')
                s++;
            if (dur_val <= 0)
                dur_val = def_dur; /* guard against atoi overflow / zero */
        }

        /* pitch or pause */
        if (*s == 'P' || *s == 'p') {
            freq = 0; /* pause — no frequency */
            s++;
        } else {
            int pc = pitch_class_of_char(*s);
            if (pc < 0) {
                /* Unrecognized character (e.g. a trailing digit from a
                 * duration prefix with no pitch letter after it). Every
                 * other advance in this loop is gated on the character it
                 * just consumed being non-NUL; this catch-all branch must
                 * guard explicitly, or a string ending in such a
                 * character walks s one byte past the NUL terminator on
                 * the next iteration's `while (*s ...)` check. */
                if (*s == '\0')
                    break;
                s++;
                continue;
            }
            s++;

            /* sharp */
            if (*s == '#') {
                pc = (pc + 1) % 12;
                s++;
            }

            /* optional octave */
            if (*s >= '4' && *s <= '7') {
                octave = *s - '0';
                s++;
            }

            if (octave < 4)
                octave = 4;
            if (octave > 7)
                octave = 7;

            /* pc is in [0..11] and octave-4 is in [0..3], guaranteed by
             * pitch_class_of_char() guard above and clamping below. */
            freq = note_freq[pc][octave - 4];
        }

        /* dotted */
        if (*s == '.') {
            dotted = true;
            s++;
        }

        /* Compute note duration in ms:
         * qn_ms * (4 / dur_val)  — at bpm, a quarter note = qn_ms,
         * and a note of value dur_val lasts (4/dur_val) quarter notes.
         */
        unsigned int note_ms = (qn_ms * 4u) / (unsigned int)dur_val;
        if (dotted)
            note_ms = (note_ms * 3u) / 2u;
        if (note_ms < 10)
            note_ms = 10; /* clamp to avoid too-short notes */

        notes[count].freq_hz = (uint16_t)freq;
        notes[count].duration_ms = (uint16_t)note_ms;
        count++;
    }

    *out_count = count;
    return count > 0;
}

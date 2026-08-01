/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include "core/rtttl.h"

TEST_CASE("rtttl_parse: simple single note at default duration/octave/bpm",
          "[rtttl]")
{
    struct rtttl_note notes[RTTTL_MAX_NOTES];
    uint8_t count = 0;

    /* "name:d=4,o=5,b=120:c" -> one quarter note, C5 (523 Hz), at 120 bpm
     * a quarter note is 500 ms. */
    REQUIRE(rtttl_parse("test:d=4,o=5,b=120:c", notes, &count) == true);
    REQUIRE(count == 1);
    REQUIRE(notes[0].freq_hz == 523);
    REQUIRE(notes[0].duration_ms == 500);
}

TEST_CASE("rtttl_parse: explicit per-note duration overrides the default",
          "[rtttl]")
{
    struct rtttl_note notes[RTTTL_MAX_NOTES];
    uint8_t count = 0;

    /* An explicit 8 (eighth note) halves the default quarter-note duration. */
    REQUIRE(rtttl_parse("test:d=4,o=5,b=120:8c", notes, &count) == true);
    REQUIRE(count == 1);
    REQUIRE(notes[0].duration_ms == 250);
}

TEST_CASE("rtttl_parse: explicit per-note octave overrides the default",
          "[rtttl]")
{
    struct rtttl_note notes[RTTTL_MAX_NOTES];
    uint8_t count = 0;

    /* Default octave 5 (523 Hz); explicit octave 6 doubles it (1047 Hz). */
    REQUIRE(rtttl_parse("test:d=4,o=5,b=120:c6", notes, &count) == true);
    REQUIRE(count == 1);
    REQUIRE(notes[0].freq_hz == 1047);
}

TEST_CASE("rtttl_parse: sharp shifts the pitch class up by one semitone",
          "[rtttl]")
{
    struct rtttl_note notes[RTTTL_MAX_NOTES];
    uint8_t count = 0;

    /* C5 (523) sharp -> C#5 (554). */
    REQUIRE(rtttl_parse("test:d=4,o=5,b=120:c#", notes, &count) == true);
    REQUIRE(count == 1);
    REQUIRE(notes[0].freq_hz == 554);
}

TEST_CASE("rtttl_parse: dotted note extends duration by 50 percent", "[rtttl]")
{
    struct rtttl_note notes[RTTTL_MAX_NOTES];
    uint8_t count = 0;

    /* Quarter note at 120bpm = 500ms; dotted = 500 * 3/2 = 750ms. */
    REQUIRE(rtttl_parse("test:d=4,o=5,b=120:c.", notes, &count) == true);
    REQUIRE(count == 1);
    REQUIRE(notes[0].duration_ms == 750);
}

TEST_CASE("rtttl_parse: pause produces a zero-frequency note", "[rtttl]")
{
    struct rtttl_note notes[RTTTL_MAX_NOTES];
    uint8_t count = 0;

    REQUIRE(rtttl_parse("test:d=4,o=5,b=120:p", notes, &count) == true);
    REQUIRE(count == 1);
    REQUIRE(notes[0].freq_hz == 0);
    REQUIRE(notes[0].duration_ms == 500);
}

TEST_CASE("rtttl_parse: multiple notes parsed in order", "[rtttl]")
{
    struct rtttl_note notes[RTTTL_MAX_NOTES];
    uint8_t count = 0;

    REQUIRE(rtttl_parse("test:d=4,o=5,b=120:c,d,e", notes, &count) == true);
    REQUIRE(count == 3);
    REQUIRE(notes[0].freq_hz == 523); /* C5 */
    REQUIRE(notes[1].freq_hz == 587); /* D5 */
    REQUIRE(notes[2].freq_hz == 659); /* E5 */
}

TEST_CASE("rtttl_parse: bpm affects note duration", "[rtttl]")
{
    struct rtttl_note slow[RTTTL_MAX_NOTES];
    struct rtttl_note fast[RTTTL_MAX_NOTES];
    uint8_t slow_count = 0;
    uint8_t fast_count = 0;

    REQUIRE(rtttl_parse("test:d=4,o=5,b=60:c", slow, &slow_count) == true);
    REQUIRE(rtttl_parse("test:d=4,o=5,b=240:c", fast, &fast_count) == true);

    /* Quarter note at 60bpm = 1000ms; at 240bpm = 250ms. */
    REQUIRE(slow[0].duration_ms == 1000);
    REQUIRE(fast[0].duration_ms == 250);
}

TEST_CASE("rtttl_parse: real tone strings from the project's tone table",
          "[rtttl]")
{
    struct rtttl_note notes[RTTTL_MAX_NOTES];
    uint8_t count = 0;

    /* "Nokia" tune, verbatim from notification.cpp's tone_table[]. */
    REQUIRE(
        rtttl_parse("24:d=4,o=5,b=180:8e6,8d6,f#,g#,8c#6,8b,d,e,8b,8a,c#,e,2a",
                    notes, &count)
        == true);
    REQUIRE(count == 13);

    /* "CQ" tune. */
    count = 0;
    REQUIRE(rtttl_parse("24:d=16,o=6,b=120:8c,p,c,p,8c,p,c,4p,8c,p,8c,p,c,p,"
                        "8c,8p",
                        notes, &count)
            == true);
    REQUIRE(count == 16);
}

TEST_CASE("rtttl_parse: malformed input returns false", "[rtttl]")
{
    struct rtttl_note notes[RTTTL_MAX_NOTES];
    uint8_t count = 0;

    /* No ':' separators at all. */
    REQUIRE(rtttl_parse("not a valid rtttl string", notes, &count) == false);
    REQUIRE(count == 0);

    /* Only one ':' (missing the note-list separator). */
    count = 0;
    REQUIRE(rtttl_parse("name:d=4,o=5,b=120", notes, &count) == false);

    /* bpm of zero is invalid (would divide by zero). */
    count = 0;
    REQUIRE(rtttl_parse("name:d=4,o=5,b=0:c", notes, &count) == false);

    /* Negative bpm is likewise rejected rather than promoted to a huge
     * unsigned divisor. */
    count = 0;
    REQUIRE(rtttl_parse("name:d=4,o=5,b=-120:c", notes, &count) == false);

    /* Empty string. */
    count = 0;
    REQUIRE(rtttl_parse("", notes, &count) == false);
}

TEST_CASE("rtttl_parse: zero or negative default duration is clamped, not "
          "divided by",
          "[rtttl]")
{
    struct rtttl_note notes[RTTTL_MAX_NOTES];
    uint8_t count = 0;

    /* d=0 with a duration-less note previously reached (qn_ms*4)/0, an
     * integer divide-by-zero.  The default must be clamped back to a quarter
     * note (500 ms at 120 bpm). */
    REQUIRE(rtttl_parse("test:d=0,o=5,b=120:c", notes, &count) == true);
    REQUIRE(count == 1);
    REQUIRE(notes[0].duration_ms == 500);

    /* d=-4 must not promote to a huge unsigned divisor either. */
    count = 0;
    REQUIRE(rtttl_parse("test:d=-4,o=5,b=120:c", notes, &count) == true);
    REQUIRE(count == 1);
    REQUIRE(notes[0].duration_ms == 500);
}

TEST_CASE("rtttl_parse: out-of-range default octave is clamped for a "
          "duration-less note",
          "[rtttl]")
{
    struct rtttl_note notes[RTTTL_MAX_NOTES];
    uint8_t count = 0;

    /* o=9 default with a note that omits its own octave: the default is
     * clamped into the 4..7 lookup range (7 here) instead of indexing past
     * note_freq[]. */
    REQUIRE(rtttl_parse("test:o=9,d=4,b=120:c", notes, &count) == true);
    REQUIRE(count == 1);
    REQUIRE(notes[0].freq_hz == 2093); /* C7 */
}

TEST_CASE("rtttl_parse: note list with no recognizable notes returns false",
          "[rtttl]")
{
    struct rtttl_note notes[RTTTL_MAX_NOTES];
    uint8_t count = 0;

    /* 'x','y','z' are not valid pitch letters or 'P'; every character is
     * skipped, leaving zero notes. */
    REQUIRE(rtttl_parse("name:d=4,o=5,b=120:x,y,z", notes, &count) == false);
    REQUIRE(count == 0);
}

TEST_CASE("rtttl_parse: parsing stops at RTTTL_MAX_NOTES without overflowing",
          "[rtttl]")
{
    struct rtttl_note notes[RTTTL_MAX_NOTES];
    uint8_t count = 0;

    /* Build a note list with more entries than RTTTL_MAX_NOTES. Sized with
     * generous headroom (+1 for NUL, rounded up further) rather than an
     * exact fit, so this does not silently become a one-byte-short buffer
     * again if RTTTL_MAX_NOTES or the prefix string ever change. */
    char rtttl[32 + (RTTTL_MAX_NOTES + 10) * 2] = "t:d=4,o=5,b=120:";
    for (int i = 0; i < RTTTL_MAX_NOTES + 10; i++) {
        strcat(rtttl, "c,");
    }

    REQUIRE(rtttl_parse(rtttl, notes, &count) == true);
    REQUIRE(count == RTTTL_MAX_NOTES); /* clamped, did not overflow */
}

TEST_CASE("rtttl_parse: octave is clamped to the supported 4..7 range",
          "[rtttl]")
{
    struct rtttl_note notes[RTTTL_MAX_NOTES];
    uint8_t count = 0;

    /* Octave digit '8' or above is not consumed as an octave (parser only
     * recognizes '4'..'7'), so the note falls back to the default octave. */
    REQUIRE(rtttl_parse("test:d=4,o=5,b=120:c9", notes, &count) == true);
    REQUIRE(count >= 1);
    REQUIRE(notes[0].freq_hz == 523); /* default octave 5, '9' not consumed */
}

TEST_CASE("rtttl_parse: very short computed durations are clamped to 10ms",
          "[rtttl]")
{
    struct rtttl_note notes[RTTTL_MAX_NOTES];
    uint8_t count = 0;

    /* A 64th note at a very high bpm would compute to well under 10ms. */
    REQUIRE(rtttl_parse("test:d=4,o=5,b=250:64c", notes, &count) == true);
    REQUIRE(count == 1);
    REQUIRE(notes[0].duration_ms >= 10);
}

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <climits>

extern "C" {
#include "core/graphics.h"
}

/*
 * TomThumb (FONT_SIZE_5PT) has yAdvance = 6.  Tests that need a concrete
 * line-height value use this constant rather than embedding magic numbers.
 */
static constexpr uint16_t TT_Y_ADVANCE = 6;

/* A max_x wide enough that no ASCII string of a few characters will wrap. */
static constexpr uint16_t WIDE = 4096;

/* A max_x so narrow (1 px) that every printable glyph triggers a wrap. */
static constexpr uint16_t NARROW = 1;

TEST_CASE("gfx_measureText single-line strings", "[gfx][text]")
{
    SECTION("empty string occupies one line")
    {
        uint16_t h = gfx_measureText(FONT_SIZE_5PT, "", 0, WIDE, SIZE_MAX);
        REQUIRE(h == TT_Y_ADVANCE);
    }

    SECTION("short string with no wrap occupies one line")
    {
        uint16_t h = gfx_measureText(FONT_SIZE_5PT, "Hello", 0, WIDE, SIZE_MAX);
        REQUIRE(h == TT_Y_ADVANCE);
    }

    SECTION("char_count zero treats string as empty")
    {
        uint16_t h = gfx_measureText(FONT_SIZE_5PT, "Hello", 0, WIDE, 0);
        REQUIRE(h == TT_Y_ADVANCE);
    }
}

TEST_CASE("gfx_measureText multi-line via explicit newlines", "[gfx][text]")
{
    SECTION("one newline produces two lines")
    {
        uint16_t h = gfx_measureText(FONT_SIZE_5PT, "abc\ndef", 0, WIDE,
                                     SIZE_MAX);
        REQUIRE(h == 2 * TT_Y_ADVANCE);
    }

    SECTION("two newlines produce three lines")
    {
        uint16_t h = gfx_measureText(FONT_SIZE_5PT, "a\nb\nc", 0, WIDE,
                                     SIZE_MAX);
        REQUIRE(h == 3 * TT_Y_ADVANCE);
    }

    SECTION("trailing newline adds an extra line")
    {
        uint16_t h = gfx_measureText(FONT_SIZE_5PT, "abc\n", 0, WIDE, SIZE_MAX);
        REQUIRE(h == 2 * TT_Y_ADVANCE);
    }
}

TEST_CASE("gfx_measureText char_count truncation", "[gfx][text]")
{
    SECTION("truncate before newline stays on first line")
    {
        /* "abc\ndef" with char_count 3 — newline never reached */
        uint16_t h = gfx_measureText(FONT_SIZE_5PT, "abc\ndef", 0, WIDE, 3);
        REQUIRE(h == TT_Y_ADVANCE);
    }

    SECTION("truncate including newline advances to second line")
    {
        /* char_count 4 includes the '\\n' */
        uint16_t h = gfx_measureText(FONT_SIZE_5PT, "abc\ndef", 0, WIDE, 4);
        REQUIRE(h == 2 * TT_Y_ADVANCE);
    }

    SECTION("char_count larger than string length measures whole string")
    {
        uint16_t h_full = gfx_measureText(FONT_SIZE_5PT, "hi", 0, WIDE,
                                          SIZE_MAX);
        uint16_t h_over = gfx_measureText(FONT_SIZE_5PT, "hi", 0, WIDE, 999);
        REQUIRE(h_over == h_full);
    }
}

TEST_CASE("gfx_measureText word-wrap at max_x", "[gfx][text]")
{
    SECTION("each glyph wraps with max_x=1, height grows per character")
    {
        /*
         * With max_x=1 every printable glyph triggers the wrap condition
         * (cur_x + xAdvance > 1), adding one yAdvance per character.
         * One character → initial line (yAdvance) + one wrap (yAdvance).
         */
        uint16_t h1 = gfx_measureText(FONT_SIZE_5PT, "A", 0, NARROW, SIZE_MAX);
        uint16_t h2 = gfx_measureText(FONT_SIZE_5PT, "AB", 0, NARROW, SIZE_MAX);
        REQUIRE(h1 == 2 * TT_Y_ADVANCE);
        REQUIRE(h2 == h1 + TT_Y_ADVANCE);
    }

    SECTION("wide max_x prevents wrap for short string")
    {
        uint16_t h_wide = gfx_measureText(FONT_SIZE_5PT, "Hello", 0, WIDE,
                                          SIZE_MAX);
        uint16_t h_narrow = gfx_measureText(FONT_SIZE_5PT, "Hello", 0, NARROW,
                                            SIZE_MAX);
        REQUIRE(h_narrow > h_wide);
    }
}

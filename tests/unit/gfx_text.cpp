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

/* -----------------------------------------------------------------------
 * gfx_printBuffer regression tests.
 *
 * These pin the behaviour fixed in the commit that corrected wrap logic,
 * UB glyph access order, and the sign of text_size.y.
 * ----------------------------------------------------------------------- */

TEST_CASE("gfx_printBuffer text_size.y is positive", "[gfx][text]")
{
    color_t white = { 255, 255, 255, 255 };
    point_t start = { 0, 10 };

    point_t sz = gfx_printBuffer(start, FONT_SIZE_5PT, TEXT_ALIGN_LEFT, white,
                                 "Hello");
    REQUIRE(sz.y > 0);
}

TEST_CASE("gfx_printBuffer text_size.y grows with newlines", "[gfx][text]")
{
    color_t white = { 255, 255, 255, 255 };
    point_t start = { 0, 10 };

    point_t sz1 = gfx_printBuffer(start, FONT_SIZE_5PT, TEXT_ALIGN_LEFT, white,
                                  "Hello");
    point_t sz2 = gfx_printBuffer(start, FONT_SIZE_5PT, TEXT_ALIGN_LEFT, white,
                                  "Hello\nHello");

    REQUIRE(sz2.y > sz1.y);
    REQUIRE((sz2.y - sz1.y) == TT_Y_ADVANCE);
}

TEST_CASE("gfx_printBuffer does not access glyph for newline", "[gfx][text]")
{
    /* If the UB glyph-access-before-newline-check bug were present, this
     * would either crash or corrupt memory when '\\n' (0x0A) is below
     * f.first.  Reaching REQUIRE without fault is the assertion. */
    color_t white = { 255, 255, 255, 255 };
    point_t start = { 0, 10 };

    point_t sz = gfx_printBuffer(start, FONT_SIZE_5PT, TEXT_ALIGN_LEFT, white,
                                 "a\nb\nc");
    REQUIRE(sz.y > 0);
}

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

/* -----------------------------------------------------------------------
 * gfx_printBufferClipped return-value tests.
 *
 * gfx_setPixel is a no-op in the Linux platform test build, so we cannot
 * verify pixel output.  We verify the returned text_size instead, which
 * exercises the layout logic without display hardware.
 * ----------------------------------------------------------------------- */

TEST_CASE("gfx_printBufferClipped text_size.y for single-line text",
          "[gfx][text]")
{
    color_t white = { 255, 255, 255, 255 };

    /*
     * Single-line: start.y does not advance, so text_size.y should equal
     * the raw glyph height of the last character rendered (line_h).  It
     * must be > 0 and <= TT_Y_ADVANCE.
     */
    point_t start = { 0, 20 };
    point_t sz = gfx_printBufferClipped(start, FONT_SIZE_5PT, TEXT_ALIGN_LEFT,
                                        white, "Hi", (uint16_t)WIDE, 0, 127);
    REQUIRE(sz.y > 0);
    REQUIRE(sz.y <= TT_Y_ADVANCE);
}

TEST_CASE("gfx_printBufferClipped text_size.y grows for two lines",
          "[gfx][text]")
{
    color_t white = { 255, 255, 255, 255 };
    point_t start = { 0, 20 };

    point_t sz1 = gfx_printBufferClipped(start, FONT_SIZE_5PT, TEXT_ALIGN_LEFT,
                                         white, "Hi", (uint16_t)WIDE, 0, 127);
    point_t sz2 = gfx_printBufferClipped(start, FONT_SIZE_5PT, TEXT_ALIGN_LEFT,
                                         white, "Hi\nHi", (uint16_t)WIDE, 0,
                                         127);

    /* Two lines must be taller than one. */
    REQUIRE(sz2.y > sz1.y);
    /* The difference should equal exactly one yAdvance. */
    REQUIRE((sz2.y - sz1.y) == TT_Y_ADVANCE);
}

TEST_CASE("gfx_printBufferClipped negative start.y does not corrupt result",
          "[gfx][text]")
{
    color_t white = { 255, 255, 255, 255 };

    /* Scrolled position: top of text block is above the visible area. */
    point_t start_neg = { 0, -10 };
    point_t start_pos = { 0, 0 };

    point_t sz_neg = gfx_printBufferClipped(start_neg, FONT_SIZE_5PT,
                                            TEXT_ALIGN_LEFT, white, "Hi",
                                            (uint16_t)WIDE, 0, 127);
    point_t sz_pos = gfx_printBufferClipped(start_pos, FONT_SIZE_5PT,
                                            TEXT_ALIGN_LEFT, white, "Hi",
                                            (uint16_t)WIDE, 0, 127);

    /* Layout is the same regardless of vertical scroll offset. */
    REQUIRE(sz_neg.y == sz_pos.y);
    REQUIRE(sz_neg.y > 0);
}

TEST_CASE("gfx_printBufferClipped negative clip_top_y does not corrupt result",
          "[gfx][text]")
{
    color_t white = { 255, 255, 255, 255 };

    /* Pixels above row 0 must be suppressed without corrupting layout. */
    point_t start = { 0, 10 };
    point_t sz = gfx_printBufferClipped(start, FONT_SIZE_5PT, TEXT_ALIGN_LEFT,
                                        white, "Hi", (uint16_t)WIDE, -10, 127);
    REQUIRE(sz.y > 0);
    REQUIRE(sz.y <= TT_Y_ADVANCE);
}

TEST_CASE("get_line_size exact-fit glyph is included in line width",
          "[gfx][text]")
{
    color_t white = { 255, 255, 255, 255 };
    point_t start = { 0, 10 };

    /* TomThumb 'A' xAdvance=4: with max_x=4 the glyph exactly fills the line. */
    point_t sz = gfx_printBufferClipped(start, FONT_SIZE_5PT, TEXT_ALIGN_LEFT,
                                        white, "A", 4, 0, 127);
    REQUIRE(sz.x == 4);
}

TEST_CASE("gfx_printBufferClipped text_size.x is max line width, not last line",
          "[gfx][text]")
{
    color_t white = { 255, 255, 255, 255 };
    point_t start = { 0, 10 };

    /*
     * First line "Hello" is wider than second line "Hi".
     * text_size.x must reflect the wider first line, not the shorter last.
     */
    point_t sz_wide = gfx_printBufferClipped(start, FONT_SIZE_5PT,
                                             TEXT_ALIGN_LEFT, white, "Hello",
                                             (uint16_t)WIDE, 0, 127);
    point_t sz_multi =
        gfx_printBufferClipped(start, FONT_SIZE_5PT, TEXT_ALIGN_LEFT, white,
                               "Hello\nHi", (uint16_t)WIDE, 0, 127);
    REQUIRE(sz_multi.x == sz_wide.x);
}

TEST_CASE("gfx_printBufferClipped text_size.x when second line is wider",
          "[gfx][text]")
{
    color_t white = { 255, 255, 255, 255 };
    point_t start = { 0, 10 };

    /* Second line "Hello" is wider than first line "Hi". */
    point_t sz_wide = gfx_printBufferClipped(start, FONT_SIZE_5PT,
                                             TEXT_ALIGN_LEFT, white, "Hello",
                                             (uint16_t)WIDE, 0, 127);
    point_t sz_multi =
        gfx_printBufferClipped(start, FONT_SIZE_5PT, TEXT_ALIGN_LEFT, white,
                               "Hi\nHello", (uint16_t)WIDE, 0, 127);
    REQUIRE(sz_multi.x == sz_wide.x);
}

TEST_CASE("gfx_printBufferClipped glyph at clip_bot_y baseline is not skipped",
          "[gfx][text]")
{
    color_t white = { 255, 255, 255, 255 };

    /* Baseline at clip_bot_y+1: all glyph pixels still fall within window. */
    int16_t base_y = 20;
    int16_t clip_bot = base_y - 1;
    point_t start = { 0, base_y };

    point_t sz = gfx_printBufferClipped(start, FONT_SIZE_5PT, TEXT_ALIGN_LEFT,
                                        white, "A", (uint16_t)WIDE, 0,
                                        clip_bot);
    REQUIRE(sz.y > 0);
}

TEST_CASE("gfx_printBufferClipped TEXT_ALIGN_RIGHT resets per line",
          "[gfx][text]")
{
    color_t white = { 255, 255, 255, 255 };
    point_t start = { 0, 10 };

    /*
     * With TEXT_ALIGN_RIGHT each line's start.x should be computed from
     * that line's own width.  The returned text_size.x must equal the
     * width of the widest line, same as for LEFT alignment.
     */
    point_t sz_right =
        gfx_printBufferClipped(start, FONT_SIZE_5PT, TEXT_ALIGN_RIGHT, white,
                               "Hello\nHi", (uint16_t)WIDE, 0, 127);
    point_t sz_left =
        gfx_printBufferClipped(start, FONT_SIZE_5PT, TEXT_ALIGN_LEFT, white,
                               "Hello\nHi", (uint16_t)WIDE, 0, 127);
    /* Max line width is alignment-independent. */
    REQUIRE(sz_right.x == sz_left.x);
}

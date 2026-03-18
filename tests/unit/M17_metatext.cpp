/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include "protocols/M17/MetaText.hpp"
#include "protocols/M17/Datatypes.hpp"

using namespace M17;

TEST_CASE("Out of order blocks", "[m17][metatext]")
{
    MetaText mt;
    meta_t meta0 = {};
    meta0.raw_data[0] = 0x71;
    memcpy(meta0.raw_data + 1, "First block  ", 13);

    meta_t meta1 = {};
    meta1.raw_data[0] = 0x72;
    memcpy(meta1.raw_data + 1, "Second block ", 13);

    meta_t meta2 = {};
    meta2.raw_data[0] = 0x74;
    memcpy(meta2.raw_data + 1, "Third block  ", 13);

    REQUIRE(mt.addBlock(meta0));
    REQUIRE(mt.addBlock(meta2));
    REQUIRE(mt.addBlock(meta1));

    const char *result = mt.getText();
    const char expected[] = "First block  Second block Third block";
    REQUIRE(strcmp(result, expected) == 0);
}

TEST_CASE("Four block maximum", "[m17][metatext]")
{
    MetaText mt;
    for (int i = 0; i < 4; i++) {
        meta_t meta = {};
        meta.raw_data[0] = 0xF0 | (1 << i);
        char text[14];
        snprintf(text, 14, "Block %d data ", i);
        memcpy(meta.raw_data + 1, text, 13);
        REQUIRE(mt.addBlock(meta));
    }
    const char *result = mt.getText();
    const char expected[] =
        "Block 0 data Block 1 data Block 2 data Block 3 data";
    REQUIRE(strcmp(result, expected) == 0);
}

TEST_CASE("Partial message gaps", "[m17][metatext]")
{
    MetaText mt;
    meta_t meta0 = {};
    meta0.raw_data[0] = 0x71;
    memcpy(meta0.raw_data + 1, "Start text   ", 13);
    meta_t meta2 = {};
    meta2.raw_data[0] = 0x74;
    memcpy(meta2.raw_data + 1, "End text     ", 13);
    mt.addBlock(meta0);
    mt.addBlock(meta2);
    const char *result = mt.getText();
    REQUIRE(result == nullptr);
}

TEST_CASE("Text trimming", "[m17][metatext]")
{
    MetaText mt;
    meta_t meta = {};
    meta.raw_data[0] = 0x11;
    memcpy(meta.raw_data + 1, "Trim test    ", 13);
    mt.addBlock(meta);
    const char *result = mt.getText();
    const char expected[] = "Trim test";
    REQUIRE(strcmp(result, expected) == 0);
}

TEST_CASE("Reset functionality", "[m17][metatext]")
{
    MetaText mt;
    meta_t meta = {};
    meta.raw_data[0] = 0x11;
    memcpy(meta.raw_data + 1, "Test data    ", 13);
    mt.addBlock(meta);
    mt.reset();
    const char *result = mt.getText();
    REQUIRE(result == nullptr);
}

TEST_CASE("MetaText roundtrip", "[m17][metatext]")
{
    const char *input =
        "MetaText roundtrip test: block 0, block 1, block 2, block 3.";
    const char *expected =
        "MetaText roundtrip test: block 0, block 1, block 2,";
    MetaText mt1;
    mt1.setText(const_cast<char *>(input));

    meta_t meta1, meta2, meta3, meta4;
    mt1.getBlock(meta1, 0);
    mt1.getBlock(meta2, 1);
    mt1.getBlock(meta3, 2);
    mt1.getBlock(meta4, 3);

    MetaText mt2;
    mt2.addBlock(meta1);
    mt2.addBlock(meta2);
    mt2.addBlock(meta3);
    mt2.addBlock(meta4);

    const char *result = mt2.getText();
    REQUIRE(strcmp(result, expected) == 0);
}

TEST_CASE("getNextBlock with single block input", "[m17][metatext]")
{
    const char *input = "openrtx";
    MetaText mt1;
    mt1.reset();
    mt1.setText(const_cast<char *>(input));

    meta_t meta1 = {};
    meta_t meta2 = {};
    meta_t meta3 = {};
    meta_t meta4 = {};
    mt1.getBlock(meta1, 0);

    mt1.getNextBlock(meta2);
    mt1.getNextBlock(meta3);
    mt1.getNextBlock(meta4);

    REQUIRE(
        strncmp((char *)(meta1.raw_data + 1), (char *)(meta2.raw_data + 1), 13)
        == 0);
    REQUIRE(
        strncmp((char *)(meta1.raw_data + 1), (char *)(meta3.raw_data + 1), 13)
        == 0);
    REQUIRE(
        strncmp((char *)(meta1.raw_data + 1), (char *)(meta4.raw_data + 1), 13)
        == 0);
}
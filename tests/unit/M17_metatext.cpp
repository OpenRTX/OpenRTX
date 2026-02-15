/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include "protocols/M17/MetaText.hpp"
#include "protocols/M17/M17Datatypes.hpp"

using namespace M17;

int test_out_of_order_blocks()
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

    if (!mt.addBlock(meta0))
        return -1;
    if (!mt.addBlock(meta2))
        return -1;
    if (!mt.addBlock(meta1))
        return -1;

    const char *result = mt.getText();
    // Trailing spaces are trimmed as per M17 spec
    const char expected[] = "First block  Second block Third block";
    if (strcmp(result, expected) != 0) {
        printf("Error: Expected '%s', got '%s'\n", expected, result);
        return -1;
    }
    return 0;
}

int test_four_block_maximum()
{
    MetaText mt;
    for (int i = 0; i < 4; i++) {
        meta_t meta = {};
        meta.raw_data[0] = 0xF0 | (1 << i);
        char text[14];
        snprintf(text, 14, "Block %d data ", i);
        memcpy(meta.raw_data + 1, text, 13);
        if (!mt.addBlock(meta)) {
            printf("Error: Failed to add block %d\n", i);
            return -1;
        }
    }
    const char *result = mt.getText();
    const char expected[] =
        "Block 0 data Block 1 data Block 2 data Block 3 data";
    if (strcmp(result, expected) != 0) {
        printf("Error: Expected '%s', got '%s'\n", expected, result);
        return -1;
    }
    return 0;
}

int test_partial_message_gaps()
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
    const char expected[] = "Start text";
    if (strcmp(result, expected) != 0) {
        printf("Error: Expected '%s', got '%s'\n", expected, result);
        return -1;
    }
    return 0;
}

int test_text_trimming()
{
    MetaText mt;
    meta_t meta = {};
    meta.raw_data[0] = 0x11;
    memcpy(meta.raw_data + 1, "Trim test    ", 13);
    mt.addBlock(meta);
    const char *result = mt.getText();
    const char expected[] = "Trim test";
    if (strcmp(result, expected) != 0) {
        printf("Error: Expected '%s', got '%s'\n", expected, result);
        return -1;
    }
    return 0;
}

int test_reset_functionality()
{
    MetaText mt;
    meta_t meta = {};
    meta.raw_data[0] = 0x11;
    memcpy(meta.raw_data + 1, "Test data    ", 13);
    mt.addBlock(meta);
    mt.reset();
    const char *result = mt.getText();
    if (strlen(result) != 0) {
        printf("Error: Reset did not clear state, got '%s'\n", result);
        return -1;
    }
    return 0;
}

/**
 * This test uses the MetaText class to fragment and reassemble a meta text, in the way that two OpenRTX transceivers may
 */
int test_metatext_roundtrip()
{
    // Compose a long text
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

    if (strcmp(result, expected) != 0) {
        printf("Error: Roundtrip expected '%s', got '%s'\n", expected, result);
        return -1;
    }
    return 0;
}

/**
 * This test uses the MetaText class's getNextBlock function and tests it when there would only be one block
 */

int test_get_next_block()
{
    // Compose a long text
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

    // Compare using strncmp since meta.raw_data + 1 is a fixed 13-byte buffer
    // that may not be null-terminated
    if (strncmp((char *)(meta1.raw_data + 1), (char *)(meta2.raw_data + 1), 13)
        != 0) {
        printf("Error: testNextBlock expected '%.13s', got '%.13s'\n",
               meta1.raw_data + 1, meta2.raw_data + 1);
        return -1;
    }
    // Verify meta3 and meta4 are also the same (idempotent behavior)
    if (strncmp((char *)(meta1.raw_data + 1), (char *)(meta3.raw_data + 1), 13)
        != 0) {
        printf("Error: testNextBlock meta3 expected '%.13s', got '%.13s'\n",
               meta1.raw_data + 1, meta3.raw_data + 1);
        return -1;
    }
    if (strncmp((char *)(meta1.raw_data + 1), (char *)(meta4.raw_data + 1), 13)
        != 0) {
        printf("Error: testNextBlock meta4 expected '%.13s', got '%.13s'\n",
               meta1.raw_data + 1, meta4.raw_data + 1);
        return -1;
    }
    return 0;
}

int main()
{
    printf("Running M17 Meta Text Unit Tests...\n");

    if (test_out_of_order_blocks()) {
        printf("FAIL: Out of order blocks test\n");
        return -1;
    }
    printf("PASS: Out of order blocks test\n");

    if (test_four_block_maximum()) {
        printf("FAIL: Four block maximum test\n");
        return -1;
    }
    printf("PASS: Four block maximum test\n");

    if (test_partial_message_gaps()) {
        printf("FAIL: Partial message gaps test\n");
        return -1;
    }
    printf("PASS: Partial message gaps test\n");

    if (test_text_trimming()) {
        printf("FAIL: Text trimming test\n");
        return -1;
    }
    printf("PASS: Text trimming test\n");

    if (test_reset_functionality()) {
        printf("FAIL: Reset functionality test\n");
        return -1;
    }
    printf("PASS: Reset functionality test\n");

    if (test_metatext_roundtrip()) {
        printf("FAIL: MetaText roundtrip test\n");
        return -1;
    }
    printf("PASS: MetaText roundtrip test\n");

    if (test_get_next_block()) {
        printf("FAIL: MetaText getnextblock test\n");
        return -1;
    }
    printf("PASS: MetaText getnextblock test\n");

    printf("All M17 Meta Text tests passed!\n");
    return 0;
}
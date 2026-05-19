/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <new>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include "core/utils.h"
#include "protocols/M17/MetaText.hpp"

using namespace M17;

MetaText::MetaText()
{
    reset();
}

MetaText::~MetaText()
{
}

void MetaText::reset()
{
    memset(text, 0, sizeof(text));
    blockMask = 0;
    expectedBlockMask = 0;
    resetBlockPaging();
}

void MetaText::getBlock(meta_t &meta, uint8_t index)
{
    memset(&meta, 0, sizeof(meta));

    size_t textLen = strlen(text);
    uint8_t totalBlocks = DIV_ROUND_UP(textLen, META_TEXT_BLOCK_LEN);

    // Set control byte: lower nibble = block index, upper nibble = total blocks mask
    uint8_t lowerNibble = (1 << index);
    uint8_t upperNibble = (0x0f >> (4 - totalBlocks)) << 4;
    meta.raw_data[0] = upperNibble | lowerNibble;

    // Fill text portion with spaces (raw_data[1] through raw_data[13])
    memset(&meta.raw_data[1], ' ', META_TEXT_BLOCK_LEN);

    size_t offset = index * META_TEXT_BLOCK_LEN;
    if (offset < textLen) {
        size_t charsToCopy = std::min(textLen - offset, (size_t) META_TEXT_BLOCK_LEN);
        memcpy(&meta.raw_data[1], text + offset, charsToCopy);
    }
}

void MetaText::setText(char *srcText)
{
    memset(text, 0, META_TEXT_MAX_LEN);

    if (!srcText)
        return;

    strncpy(text, srcText, META_TEXT_MAX_LEN);
    resetBlockPaging();
}

bool MetaText::addBlock(const meta_t &meta)
{
    // Retrieve the bitmask of the current block
    uint8_t mask = meta.raw_data[0] & 0x0f;
    uint8_t index = mask ? __builtin_ctz(mask) : 0;
    size_t offset = index * META_TEXT_BLOCK_LEN;

    // Store expected block mask from upper nibble of control byte
    expectedBlockMask = (meta.raw_data[0] & 0xf0) >> 4;

    if (index >= META_TEXT_MAX_BLOCKS)
        return false;

    // Set bit for this block and copy data directly to buffer
    blockMask |= mask;
    memcpy(text + offset, &meta.raw_data[1], META_TEXT_BLOCK_LEN);

    return true;
}

const char *MetaText::getText()
{
    // According to M17 spec: OR the control bytes together, and once
    // the most significant and least significant four bits are the same,
    // a complete message has been received.
    if (blockMask == 0 || blockMask != expectedBlockMask)
        return nullptr;

    // All expected blocks received, compute total length
    size_t len = __builtin_popcount(blockMask) * META_TEXT_BLOCK_LEN;

    // Trim trailing spaces
    while (len > 0 && text[len - 1] == ' ')
        len--;

    text[len] = '\0';

    return text;
}

void MetaText::getNextBlock(meta_t &meta)
{
    uint8_t totalBlocks = DIV_ROUND_UP(strlen(text), META_TEXT_BLOCK_LEN);

    getBlock(meta, nextBlockIndex);
    nextBlockIndex = (nextBlockIndex + 1) % totalBlocks;
}

void MetaText::resetBlockPaging()
{
    nextBlockIndex = 0;
}

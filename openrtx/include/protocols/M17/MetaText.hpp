/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef METATEXT_H
#define METATEXT_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
#include <cstring>
#include "M17Datatypes.hpp"

/**
 * M17 Meta Text reassembly utilities.
 * 
 * Meta text can span up to 4 stream frames. This module provides both
 * single-frame parsing (layer 2) and multi-frame accumulation (layer 3)
 * for simplicity, as the reassembly logic is straightforward.
 */

namespace M17
{

static constexpr uint8_t META_TEXT_MAX_BLOCKS = 4;
static constexpr uint8_t META_TEXT_BLOCK_LEN = 13;
static constexpr uint8_t META_TEXT_MAX_LEN = META_TEXT_MAX_BLOCKS
                                           * META_TEXT_BLOCK_LEN;

/**
 * Class for accumulating multi-block M17 text metadata.
 */
class MetaText
{
private:
    char text[META_TEXT_MAX_LEN + 1];
    uint8_t blockMask; // Bitmap of blocks added (bit i = block i present)

    /**
     * Set the block index (0-based) at metadata byte 0.
     */
    uint8_t setBlockIndex(meta_t &meta, uint8_t index);

    /**
     * Set the number of total blocks
     */
    uint8_t setTotalBlocks(meta_t &meta, uint8_t totalBlocks);

    uint8_t nextBlockIndex = 0;

public:
    MetaText();

    ~MetaText();

    /**
     * Reset the MetaText state.
     */
    void reset();

    meta_t getBlock(meta_t &meta, uint8_t index);

    void setText(char *meta);

    /**
     * Get the block index (0-based) from metadata byte 0.
     */
    uint8_t getBlockIndex(const meta_t &meta);

    /**
     * Get total block count from metadata byte 0.
     */
    uint8_t getTotalBlocks(const meta_t &meta);

    /**
     * Add a metadata block to the state.
     * @return true if block was added successfully.
     */
    bool addBlock(const meta_t &meta);

    /**
     * Get assembled text string.
     * @return pointer to null-terminated text (may be empty).
     */
    const char *getText();

    void getNextBlock(meta_t &meta);

    void resetBlockPaging();
};

} // namespace M17

#endif // METATEXT_H

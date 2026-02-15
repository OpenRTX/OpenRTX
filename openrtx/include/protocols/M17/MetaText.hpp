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

namespace M17
{

/**
 * @brief M17 Meta Text disassembly and reassembly
 * 
 * This class implements both frame parsing and accumulation for M17 protocol metadata text,
 * supporting both reception and transmission of MetaText blocks.
 * 
 * The M17 protocol splits text messages into 13-char blocks, with up to 4 blocks total
 * supported. Each block contains a control byte followed by the text data.
 * 
 * For reception, the class accumulates incoming blocks using a bitmap to track which
 * blocks have been received, allowing out-of-order reception and duplicate detection.
 * 
 * For transmission, the class maintains a round-robin iterator that cycles through all
 * blocks in the sequence. This allows continuous retransmission of the complete message.
 * The iterator should be reset to start from block 0 when beginning a new transmission cycle.
 */
class MetaText
{
public:
    /**
     * @brief Default constructor.
     * 
     * Initializes the MetaText object with empty text buffer and reset state.
     */
    MetaText();

    /**
     * @brief Destructor.
     */
    ~MetaText();

    /**
     * @brief Reset the MetaText state.
     * 
     * Clears all accumulated text data, block mask, and resets block paging to initial state.
     */
    void reset();

    /**
     * @brief Return a single meta block at a given index.
     * 
     * Generates a properly formatted M17 metadata block containing the control byte
     * and text data for the specified block index.
     * 
     * @param meta Reference to meta_t structure to populate with block data
     * @param index The 0-based index of the block being fetched (0-3)
     * @return The populated meta object
     */
    meta_t getBlock(meta_t &meta, uint8_t index);

    /**
     * @brief Set the text that will be included in transmission.
     * 
     * @param srcText The char array of text to transmit. Only the first 52 characters will
     *                be used. Can be nullptr to clear the text.
     */
    void setText(char *meta);

    /**
     * @brief Add a received metadata block to the accumulator.
     * 
     * Processes an incoming M17 metadata block and adds it to the internal text buffer.
     * Multiple blocks are assembled to reconstruct the complete message.
     * 
     * @param meta The metadata block to add
     * @return true if block was added successfully, false if block index is invalid
     */
    bool addBlock(const meta_t &meta);

    /**
     * @brief Get the assembled text string from received blocks.
     * 
     * Returns the complete text message assembled from received metadata blocks.
     * Text is only considered complete when all sequential blocks have been received.
     * 
     * @return Pointer to null-terminated text string (may be empty if no complete message)
     */
    const char *getText();

    /**
     * Return the next block of text that is available; this is useful
     * for TX.
     * 
     * This method implements a round-robin iterator over all blocks in the current
     * text message. Each call returns the next sequential block and advances the
     * internal iterator. When the last block is reached, the iterator wraps around
     * to block 0, enabling continuous cyclic transmission of the complete message.
     * 
     * @param meta Reference to meta_t structure where the next block will be stored
     */
    void getNextBlock(meta_t &meta);

    /**
     * @brief Reset the transmission block paging mechanism.
     * 
     * Resets the internal block counter to start transmission from the first block.
     * Call this when starting a new transmission sequence.
     */
    void resetBlockPaging();

private:
    /**
     * @brief Set the block index in the metadata control byte.
     * 
     * Sets the lower nibble of raw_data[0] to indicate which block this is.
     * Uses bit position encoding: 0001=block0, 0010=block1, 0100=block2, 1000=block3.
     * 
     * @param meta Reference to meta_t structure to modify
     * @param index 0-based block index (0-3)
     */
    void setBlockIndex(meta_t &meta, uint8_t index);

    /**
     * @brief Set the total number of blocks in the metadata control byte.
     * 
     * Sets the upper nibble of raw_data[0] to indicate total blocks in message.
     * Uses mask encoding: 0001=1block, 0011=2blocks, 0111=3blocks, 1111=4blocks.
     * 
     * @param meta Reference to meta_t structure to modify
     * @param totalBlocks Total number of blocks in the message (1-4)
     */
    void setTotalBlocks(meta_t &meta, uint8_t totalBlocks);

    /**
     * @brief Extract the block index from metadata control byte.
     * 
     * Decodes the lower nibble of raw_data[0] to determine which block this is.
     * 
     * @param meta The metadata structure to examine
     * @return 0-based block index (0-3)
     */
    uint8_t getBlockIndex(const meta_t &meta);

    /**
     * @brief Extract the total block count from metadata control byte.
     * 
     * Decodes the upper nibble of raw_data[0] to determine total blocks in message.
     * 
     * @param meta The metadata structure to examine
     * @return Total number of blocks in the message (1-4)
     */
    uint8_t getTotalBlocks(const meta_t &meta);

    /**
     * @brief Round-robin iterator for transmission
     * Tracks next block index to send (0-3)
     */
    uint8_t nextBlockIndex = 0;

    /** @brief Maximum number of text blocks supported by M17 protocol */
    static constexpr uint8_t META_TEXT_MAX_BLOCKS = 4;

    /** @brief Number of text characters per block */
    static constexpr uint8_t META_TEXT_BLOCK_LEN = 13;

    /** @brief Maximum total text length (4 blocks x 13 chars) */
    static constexpr uint8_t META_TEXT_MAX_LEN = META_TEXT_MAX_BLOCKS
                                               * META_TEXT_BLOCK_LEN;

    /** @brief Internal text buffer with space for null terminator */
    char text[META_TEXT_MAX_LEN + 1];

    /** 
     * @brief Bitmap tracking which blocks have been received (bit i = block i present)
     */
    uint8_t blockMask;
};

} // namespace M17

#endif // METATEXT_H

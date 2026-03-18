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
#include "Datatypes.hpp"

namespace M17
{

/**
 * @brief M17 Meta Text disassembly and reassembly
 * 
 * This class implements both frame parsing and accumulation for M17 protocol
 * metadata text, supporting both reception and transmission of MetaText blocks.
 * 
 * The M17 protocol splits text messages into 13-char blocks, with up to 4 blocks
 * total supported. Each block contains a control byte followed by the text data.
 * 
 * For reception, the class accumulates incoming blocks using a bitmap to track
 * which blocks have been received, allowing out-of-order reception and duplicate
 * detection.
 * 
 * For transmission, the class maintains a round-robin iterator that cycles
 * through all blocks in the sequence. This allows continuous retransmission of
 * the complete message. The iterator should be reset to start from block 0 when
 * beginning a new transmission cycle.
 */
class MetaText
{
public:
    /**
     * @brief Default constructor.
     * 
     * Initialize the MetaText object with empty text buffer and reset state.
     */
    MetaText();

    /**
     * @brief Destructor.
     */
    ~MetaText();

    /**
     * @brief Reset the MetaText state.
     * 
     * Clear all accumulated text data, block mask, and resets block paging to
     * initial state.
     */
    void reset();

    /**
     * @brief Return a single meta block at a given index.
     * 
     * Generate a properly formatted M17 metadata block containing the control
     * byte and text data for the specified block index.
     * 
     * @param meta Reference to meta_t structure to populate with block data
     * @param index The 0-based index of the block being fetched (0-3)
     */
    void getBlock(meta_t &meta, uint8_t index);

    /**
     * @brief Set the text that will be included in transmission.
     * 
     * @param srcText The char array of text to transmit. Only the first 52
     * characters will be used. Can be nullptr to clear the text.
     */
    void setText(char *meta);

    /**
     * @brief Add a received metadata block to the accumulator.
     * 
     * Process an incoming M17 metadata block and adds it to the internal text
     * buffer. Multiple blocks are assembled to reconstruct the complete message.
     * 
     * @param meta The metadata block to add
     * @return true if block was added successfully, false if block index is invalid
     */
    bool addBlock(const meta_t &meta);

    /**
     * @brief Get the assembled text string from received blocks.
     * 
     * Return the complete text message assembled from received metadata blocks.
     * Text is only considered complete when all sequential blocks have been
     * received.
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
     * Reset the internal block counter to start transmission from the first block.
     * Call this when starting a new transmission sequence.
     */
    void resetBlockPaging();

private:
    static constexpr uint8_t META_TEXT_MAX_BLOCKS = 4;
    static constexpr uint8_t META_TEXT_BLOCK_LEN = 13;
    static constexpr uint8_t META_TEXT_MAX_LEN = META_TEXT_MAX_BLOCKS
                                               * META_TEXT_BLOCK_LEN;
    char text[META_TEXT_MAX_LEN + 1]; ///< Internal text
    uint8_t nextBlockIndex = 0;       ///< Round-robin iterator for transmission
    uint8_t blockMask;                ///< Bitmap of received blocks
    uint8_t expectedBlockMask;        ///< Block mask for complete message
};

} // namespace M17

#endif // METATEXT_H

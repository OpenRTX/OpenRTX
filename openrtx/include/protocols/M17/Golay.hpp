/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GOLAY_H
#define GOLAY_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
#include <array>

namespace M17
{

namespace Golay24
{

/**
 * Function computing the Golay(24,12) checksum of a given 12-bit data block.
 *
 * @param value: input data.
 * @return Golay(24,12) checksum.
 */
uint16_t calcChecksum(const uint16_t& value);

/**
 * Detect and correct errors in a Golay(24,12) codeword.
 *
 * @param codeword: input codeword.
 * @return bitmask corresponding to detected bit errors in the codeword, or
 * 0xFFFFFFFF if bit errors are unrecoverable.
 */
uint32_t detectErrors(const uint32_t& codeword);

}   // namespace Golay24


/**
 * Compute the Golay(24,12) codeword of a given block of data using the
 * generator polynomial 0xC75.
 * Result is composed as follows:
 *
 * +--------+----------+--------+
 * | parity | checksum |  data  |
 * +--------+----------+--------+
 * | 1 bit  |   11 bit | 12 bit |
 * +--------+----------+--------+
 *
 * \param data: input data, upper four bits are discarded.
 * \return resulting 24 bit codeword.
 */
static inline uint32_t golay24_encode(const uint16_t& data)
{
    return (data << 12) | Golay24::calcChecksum(data);
}


/**
 * Decode a Golay(24,12) codeword, correcting eventual bit errors. In case the
 * bit errors are not correctable, the function returns 0xFFFF, a value outside
 * the range allowed for the 12-bit input data required by Golay coding.
 *
 * \param codeword: input Golay(24,12) codeword.
 * \return original data block or 0xFFFF in case of unrecoverable errors.
 */
static inline uint16_t golay24_decode(const uint32_t& codeword)
{
    uint32_t errors = Golay24::detectErrors(codeword);
    if(errors == 0xFFFFFFFF) return 0xFFFF;
    return ((codeword ^ errors) >> 12) & 0x0FFF;
}

/**
 * Decode a Golay(24,12) codeword from soft bits (Chase-II). The hard decoder
 * runs with every combination of the four least reliable bits flipped, and
 * the candidate closest to the soft bits wins. When the runner-up is almost
 * as close, the codeword is ambiguous and rejected.
 *
 * \param soft: the 24 soft bits of the codeword, most significant first,
 * 0x0000 a confident 0 and 0xFFFF a confident 1.
 * \return original data block or 0xFFFF if no codeword is close enough.
 */
uint16_t golay24_softDecode(const std::array< uint16_t, 24 >& soft);

}      // namespace M17

#endif // GOLAY_H

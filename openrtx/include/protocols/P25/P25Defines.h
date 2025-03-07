/*
 *   Copyright (C) 2016,2017 by Jonathan Naylor G4KLX
 *   Copyright (C) 2018 by Bryan Biedenkapp <gatekeep@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if !defined(P25DEFINES_H)
#define  P25DEFINES_H
#include <stdint.h>

const unsigned int P25_RADIO_SYMBOL_LENGTH = 5U;      // At 24 kHz sample rate

const unsigned int P25_HDR_FRAME_LENGTH_BYTES      = 99U;
const unsigned int P25_HDR_FRAME_LENGTH_BITS       = P25_HDR_FRAME_LENGTH_BYTES * 8U;
const unsigned int P25_HDR_FRAME_LENGTH_SYMBOLS    = P25_HDR_FRAME_LENGTH_BYTES * 4U;
const unsigned int P25_HDR_FRAME_LENGTH_SAMPLES    = P25_HDR_FRAME_LENGTH_SYMBOLS * P25_RADIO_SYMBOL_LENGTH;

const unsigned int P25_LDU_FRAME_LENGTH_BYTES      = 216U;
const unsigned int P25_LDU_FRAME_LENGTH_BITS       = P25_LDU_FRAME_LENGTH_BYTES * 8U;
const unsigned int P25_LDU_FRAME_LENGTH_SYMBOLS    = P25_LDU_FRAME_LENGTH_BYTES * 4U;
const unsigned int P25_LDU_FRAME_LENGTH_SAMPLES    = P25_LDU_FRAME_LENGTH_SYMBOLS * P25_RADIO_SYMBOL_LENGTH;

const unsigned int P25_TERMLC_FRAME_LENGTH_BYTES   = 54U;
const unsigned int P25_TERMLC_FRAME_LENGTH_BITS    = P25_TERMLC_FRAME_LENGTH_BYTES * 8U;
const unsigned int P25_TERMLC_FRAME_LENGTH_SYMBOLS = P25_TERMLC_FRAME_LENGTH_BYTES * 4U;
const unsigned int P25_TERMLC_FRAME_LENGTH_SAMPLES = P25_TERMLC_FRAME_LENGTH_SYMBOLS * P25_RADIO_SYMBOL_LENGTH;

const unsigned int P25_TERM_FRAME_LENGTH_BYTES     = 18U;
const unsigned int P25_TERM_FRAME_LENGTH_BITS      = P25_TERM_FRAME_LENGTH_BYTES * 8U;
const unsigned int P25_TERM_FRAME_LENGTH_SYMBOLS   = P25_TERM_FRAME_LENGTH_BYTES * 4U;
const unsigned int P25_TERM_FRAME_LENGTH_SAMPLES   = P25_TERM_FRAME_LENGTH_SYMBOLS * P25_RADIO_SYMBOL_LENGTH;

const unsigned int P25_TSDU_FRAME_LENGTH_BYTES     = 45U;
const unsigned int P25_TSDU_FRAME_LENGTH_BITS      = P25_TSDU_FRAME_LENGTH_BYTES * 8U; 
const unsigned int P25_TSDU_FRAME_LENGTH_SYMBOLS   = P25_TSDU_FRAME_LENGTH_BYTES * 4U; 
const unsigned int P25_TSDU_FRAME_LENGTH_SAMPLES   = P25_TSDU_FRAME_LENGTH_SYMBOLS * P25_RADIO_SYMBOL_LENGTH;

const unsigned int P25_PDU_HDR_FRAME_LENGTH_BYTES = 45U;
const unsigned int P25_PDU_HDR_FRAME_LENGTH_BITS = P25_PDU_HDR_FRAME_LENGTH_BYTES * 8U;
const unsigned int P25_PDU_HDR_FRAME_LENGTH_SYMBOLS = P25_PDU_HDR_FRAME_LENGTH_BYTES * 4U;
const unsigned int P25_PDU_HDR_FRAME_LENGTH_SAMPLES = P25_PDU_HDR_FRAME_LENGTH_SYMBOLS * P25_RADIO_SYMBOL_LENGTH;

const unsigned int P25_SYNC_LENGTH_BYTES   = 6U;
const unsigned int P25_SYNC_LENGTH_BITS    = P25_SYNC_LENGTH_BYTES * 8U;
const unsigned int P25_SYNC_LENGTH_SYMBOLS = P25_SYNC_LENGTH_BYTES * 4U;
const unsigned int P25_SYNC_LENGTH_SAMPLES = P25_SYNC_LENGTH_SYMBOLS * P25_RADIO_SYMBOL_LENGTH;

const unsigned int P25_NID_LENGTH_BYTES    = 8U;
const unsigned int P25_NID_LENGTH_BITS     = P25_NID_LENGTH_BYTES * 8U;
const unsigned int P25_NID_LENGTH_SYMBOLS  = P25_NID_LENGTH_BYTES * 4U; 
const unsigned int P25_NID_LENGTH_SAMPLES  = P25_NID_LENGTH_SYMBOLS * P25_RADIO_SYMBOL_LENGTH;

const uint8_t P25_SYNC_BYTES[] = {0x55U, 0x75U, 0xF5U, 0xFFU, 0x77U, 0xFFU};
const uint8_t P25_SYNC_BYTES_LENGTH  = 6U;

const uint64_t P25_SYNC_BITS      = 0x00005575F5FF77FFU;
const uint64_t P25_SYNC_BITS_MASK = 0x0000FFFFFFFFFFFFU;

// 5     5      7     5      F     5      F     F      7     7      F     F
// 01 01 01 01  01 11 01 01  11 11 01 01  11 11 11 11  01 11 01 11  11 11 11 11
// +3 +3 +3 +3  +3 -3 +3 +3  -3 -3 +3 +3  -3 -3 -3 -3  +3 -3 +3 -3  -3 -3 -3 -3

const int8_t P25_SYNC_SYMBOLS_VALUES[] = {+3, +3, +3, +3, +3, -3, +3, +3, -3, -3, +3, +3, -3, -3, -3, -3, +3, -3, +3, -3, -3, -3, -3, -3};

const uint32_t P25_SYNC_SYMBOLS      = 0x00FB30A0U;
const uint32_t P25_SYNC_SYMBOLS_MASK = 0x00FFFFFFU;

const uint8_t P25_DUID_HDU = 0x00U;             // Header Data Unit
const uint8_t P25_DUID_TDU = 0x03U;             // Simple Terminator Data Unit
const uint8_t P25_DUID_LDU1 = 0x05U;            // Logical Link Data Unit 1
const uint8_t P25_DUID_TSDU = 0x07U;            // Trunking System Data Unit
const uint8_t P25_DUID_LDU2 = 0x0AU;            // Logical Link Data Unit 2
const uint8_t P25_DUID_PDU = 0x0CU;             // Packet Data Unit 
const uint8_t P25_DUID_TDULC = 0x0FU;           // Terminator Data Unit with Link Control

const unsigned int  P25_MI_LENGTH_BYTES = 9U;

const unsigned char P25_LCF_GROUP   = 0x00U;
const unsigned char P25_LCF_PRIVATE = 0x03U;

const unsigned int  P25_SS0_START    = 70U;
const unsigned int  P25_SS1_START    = 71U;
const unsigned int  P25_SS_INCREMENT = 72U;

const unsigned char P25_DUID_HEADER  = 0x00U;
const unsigned char P25_DUID_TERM    = 0x03U;
const unsigned char P25_DUID_TERM_LC = 0x0FU;

const unsigned char P25_NULL_IMBE[] = {0x04U, 0x0CU, 0xFDU, 0x7BU, 0xFBU, 0x7DU, 0xF2U, 0x7BU, 0x3DU, 0x9EU, 0x45U};

#endif


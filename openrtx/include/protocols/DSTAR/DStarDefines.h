/*
 *   Copyright (C) 2009-2015 by Jonathan Naylor G4KLX
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

#if !defined(DSTARDEFINES_H)
#define  DSTARDEFINES_H

const unsigned int DSTAR_RADIO_BIT_LENGTH = 5U;      // At 24 kHz sample rate
const unsigned int DSTAR_RADIO_SYMBOL_LENGTH = 5U;      // At 24 kHz sample rate
const unsigned int DSTAR_END_SYNC_LENGTH_BYTES = 6U;
const uint8_t  DSTAR_END_SYNC_BYTES[] = {0x55, 0x55, 0x55, 0x55, 0xC8, 0x7A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const unsigned int DSTAR_HEADER_LENGTH_BYTES = 41U;
const unsigned int DSTAR_HEADER_LENGTH_BITS  = DSTAR_HEADER_LENGTH_BYTES * 8U;

const unsigned int DSTAR_FEC_SECTION_LENGTH_BYTES = 83U;
const unsigned int DSTAR_FEC_SECTION_LENGTH_BITS  = 660U;

const unsigned int DSTAR_DATA_LENGTH_BYTES = 12U;
const unsigned int DSTAR_DATA_LENGTH_BITS  = DSTAR_DATA_LENGTH_BYTES * 8U;

//const uint8_t      DSTAR_EOT_BYTES[] = {0x55, 0x55, 0x55, 0x55, 0xC8, 0x7A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//const unsigned int DSTAR_EOT_LENGTH_BYTES = 6U;
const unsigned int DSTAR_EOT_LENGTH_BITS  = DSTAR_RADIO_SYMBOL_LENGTH * 8U;

const uint8_t      DSTAR_DATA_SYNC_LENGTH_BYTES = 3U;
const uint8_t      DSTAR_DATA_SYNC_LENGTH_BITS  = DSTAR_DATA_SYNC_LENGTH_BYTES * 8U;

const uint8_t      DSTAR_DATA_SYNC_BYTES[] = {0x9E, 0x8D, 0x32, 0x88, 0x26, 0x1A, 0x3F, 0x61, 0xE8, 0x55, 0x2D, 0x16};

const uint8_t      DSTAR_SLOW_DATA_TYPE_TEXT   = 0x40U;
const uint8_t      DSTAR_SLOW_DATA_TYPE_HEADER = 0x50U;

const uint8_t      DSTAR_SCRAMBLER_BYTES[] = {0x70U, 0x4FU, 0x93U};

#endif


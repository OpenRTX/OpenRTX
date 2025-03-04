/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
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

#include <P25/P25Utils.h>
#include <P25/P25Defines.h>

#include <cstdio>
#include <cassert>

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

void CP25Utils::decode(const unsigned char* in, unsigned char* out, unsigned int start, unsigned int stop)
{
	assert(in != NULL);
	assert(out != NULL);

	// Move the SSx positions to the range needed
	unsigned int ss0Pos = P25_SS0_START;
	unsigned int ss1Pos = P25_SS1_START;

	while (ss0Pos < start) {
		ss0Pos += P25_SS_INCREMENT;
		ss1Pos += P25_SS_INCREMENT;
	}

	unsigned int n = 0U;
	for (unsigned int i = start; i < stop; i++) {
		if (i == ss0Pos) {
			ss0Pos += P25_SS_INCREMENT;
		} else if (i == ss1Pos) {
			ss1Pos += P25_SS_INCREMENT;
		} else {
			bool b = READ_BIT(in, i);
			WRITE_BIT(out, n, b);
			n++;
		}
	}
}

void CP25Utils::encode(const unsigned char* in, unsigned char* out, unsigned int start, unsigned int stop)
{
	assert(in != NULL);
	assert(out != NULL);

	// Move the SSx positions to the range needed
	unsigned int ss0Pos = P25_SS0_START;
	unsigned int ss1Pos = P25_SS1_START;

	while (ss0Pos < start) {
		ss0Pos += P25_SS_INCREMENT;
		ss1Pos += P25_SS_INCREMENT;
	}

	unsigned int n = 0U;
	for (unsigned int i = start; i < stop; i++) {
		if (i == ss0Pos) {
			ss0Pos += P25_SS_INCREMENT;
		} else if (i == ss1Pos) {
			ss1Pos += P25_SS_INCREMENT;
		} else {
			bool b = READ_BIT(in, n);
			WRITE_BIT(out, i, b);
			n++;
		}
	}
}

unsigned int CP25Utils::compare(const unsigned char* data1, const unsigned char* data2, unsigned int length)
{
	assert(data1 != NULL);
	assert(data2 != NULL);

	unsigned int errs = 0U;
	for (unsigned int i = 0U; i < length; i++) {
		unsigned char v = data1[i] ^ data2[i];
		while (v != 0U) {
			v &= v - 1U;
			errs++;
		}
	}

	return errs;
}

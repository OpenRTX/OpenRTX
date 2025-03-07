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

#include <P25/P25Audio.h>
#include <P25/P25Utils.h>
#include <P25/Golay24128.h>
#include <P25/Hamming.h>

#include <cstdio>
#include <cassert>

const unsigned int IMBE_INTERLEAVE[] = {
	0,  7, 12, 19, 24, 31, 36, 43, 48, 55, 60, 67, 72, 79, 84, 91,  96, 103, 108, 115, 120, 127, 132, 139,
	1,  6, 13, 18, 25, 30, 37, 42, 49, 54, 61, 66, 73, 78, 85, 90,  97, 102, 109, 114, 121, 126, 133, 138,
	2,  9, 14, 21, 26, 33, 38, 45, 50, 57, 62, 69, 74, 81, 86, 93,  98, 105, 110, 117, 122, 129, 134, 141,
	3,  8, 15, 20, 27, 32, 39, 44, 51, 56, 63, 68, 75, 80, 87, 92,  99, 104, 111, 116, 123, 128, 135, 140,
	4, 11, 16, 23, 28, 35, 40, 47, 52, 59, 64, 71, 76, 83, 88, 95, 100, 107, 112, 119, 124, 131, 136, 143,
	5, 10, 17, 22, 29, 34, 41, 46, 53, 58, 65, 70, 77, 82, 89, 94, 101, 106, 113, 118, 125, 130, 137, 142};

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CP25Audio::CP25Audio() :
m_fec()
{
}

CP25Audio::~CP25Audio()
{
}

unsigned int CP25Audio::process(unsigned char* data)
{
	assert(data != NULL);

	unsigned int errs = 0U;

	unsigned char imbe[18U];

	CP25Utils::decode(data, imbe, 114U, 262U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 114U, 262U);

	CP25Utils::decode(data, imbe, 262U, 410U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 262U, 410U);

	CP25Utils::decode(data, imbe, 452U, 600U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 452U, 600U);

	CP25Utils::decode(data, imbe, 640U, 788U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 640U, 788U);

	CP25Utils::decode(data, imbe, 830U, 978U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 830U, 978U);

	CP25Utils::decode(data, imbe, 1020U, 1168U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 1020U, 1168U);

	CP25Utils::decode(data, imbe, 1208U, 1356U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 1208U, 1356U);

	CP25Utils::decode(data, imbe, 1398U, 1546U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 1398U, 1546U);

	CP25Utils::decode(data, imbe, 1578U, 1726U);
	errs += m_fec.regenerateIMBE(imbe);
	CP25Utils::encode(imbe, data, 1578U, 1726U);

	return errs;
}

void CP25Audio::decode(const unsigned char* data, unsigned char* imbe, unsigned int n)
{
	assert(data != NULL);
	assert(imbe != NULL);

	unsigned char temp[18U];

	switch (n) {
	case 0U:
		CP25Utils::decode(data, temp, 114U, 262U);
		break;
	case 1U:
		CP25Utils::decode(data, temp, 262U, 410U);
		break;
	case 2U:
		CP25Utils::decode(data, temp, 452U, 600U);
		break;
	case 3U:
		CP25Utils::decode(data, temp, 640U, 788U);
		break;
	case 4U:
		CP25Utils::decode(data, temp, 830U, 978U);
		break;
	case 5U:
		CP25Utils::decode(data, temp, 1020U, 1168U);
		break;
	case 6U:
		CP25Utils::decode(data, temp, 1208U, 1356U);
		break;
	case 7U:
		CP25Utils::decode(data, temp, 1398U, 1546U);
		break;
	case 8U:
		CP25Utils::decode(data, temp, 1578U, 1726U);
		break;
	default:
		return;
	}

	bool bit[144U];

	// De-interleave
	for (unsigned int i = 0U; i < 144U; i++) {
		unsigned int n = IMBE_INTERLEAVE[i];
		bit[i] = READ_BIT(temp, n);
	}

	// now ..

	// 12 voice bits     0
	// 11 golay bits     12
	//
	// 12 voice bits     23
	// 11 golay bits     35
	//
	// 12 voice bits     46
	// 11 golay bits     58
	//
	// 12 voice bits     69
	// 11 golay bits     81
	//
	// 11 voice bits     92
	//  4 hamming bits   103
	//
	// 11 voice bits     107
	//  4 hamming bits   118
	//
	// 11 voice bits     122
	//  4 hamming bits   133
	//
	//  7 voice bits     137

	// c0
	unsigned int c0data = 0U;
	for (unsigned int i = 0U; i < 12U; i++)
		c0data = (c0data << 1) | (bit[i] ? 0x01U : 0x00U);

	bool prn[114U];

	// Create the whitening vector and save it for future use
	unsigned int p = 16U * c0data;
	for (unsigned int i = 0U; i < 114U; i++) {
		p = (173U * p + 13849U) % 65536U;
		prn[i] = p >= 32768U;
	}

	// De-whiten some bits
	for (unsigned int i = 0U; i < 114U; i++)
		bit[i + 23U] ^= prn[i];

	unsigned int offset = 0U;
	for (unsigned int i = 0U; i < 12U; i++, offset++)
		WRITE_BIT(imbe, offset, bit[i + 0U]);
	for (unsigned int i = 0U; i < 12U; i++, offset++)
		WRITE_BIT(imbe, offset, bit[i + 23U]);
	for (unsigned int i = 0U; i < 12U; i++, offset++)
		WRITE_BIT(imbe, offset, bit[i + 46U]);
	for (unsigned int i = 0U; i < 12U; i++, offset++)
		WRITE_BIT(imbe, offset, bit[i + 69U]);
	for (unsigned int i = 0U; i < 11U; i++, offset++)
		WRITE_BIT(imbe, offset, bit[i + 92U]);
	for (unsigned int i = 0U; i < 11U; i++, offset++)
		WRITE_BIT(imbe, offset, bit[i + 107U]);
	for (unsigned int i = 0U; i < 11U; i++, offset++)
		WRITE_BIT(imbe, offset, bit[i + 122U]);
	for (unsigned int i = 0U; i < 7U; i++, offset++)
		WRITE_BIT(imbe, offset, bit[i + 137U]);
}

void CP25Audio::encode(unsigned char* data, const unsigned char* imbe, unsigned int n)
{
	assert(data != NULL);
	assert(imbe != NULL);

	bool bTemp[144U];
	bool* bit = bTemp;

	// c0
	unsigned int c0 = 0U;
	for (unsigned int i = 0U; i < 12U; i++) {
		bool b = READ_BIT(imbe, i);
		c0 = (c0 << 1) | (b ? 0x01U : 0x00U);
	}
	unsigned int g2 = CGolay24128::encode23127(c0);
	for (int i = 23; i >= 0; i--) {
		bit[i] = (g2 & 0x01U) == 0x01U;
		g2 >>= 1;
	}
	bit += 23U;

	// c1
	unsigned int c1 = 0U;
	for (unsigned int i = 12U; i < 24U; i++) {
		bool b = READ_BIT(imbe, i);
		c1 = (c1 << 1) | (b ? 0x01U : 0x00U);
	}
	g2 = CGolay24128::encode23127(c1);
	for (int i = 23; i >= 0; i--) {
		bit[i] = (g2 & 0x01U) == 0x01U;
		g2 >>= 1;
	}
	bit += 23U;

	// c2
	unsigned int c2 = 0;
	for (unsigned int i = 24U; i < 36U; i++) {
		bool b = READ_BIT(imbe, i);
		c2 = (c2 << 1) | (b ? 0x01U : 0x00U);
	}
	g2 = CGolay24128::encode23127(c2);
	for (int i = 23; i >= 0; i--) {
		bit[i] = (g2 & 0x01U) == 0x01U;
		g2 >>= 1;
	}
	bit += 23U;

	// c3
	unsigned int c3 = 0U;
	for (unsigned int i = 36U; i < 48U; i++) {
		bool b = READ_BIT(imbe, i);
		c3 = (c3 << 1) | (b ? 0x01U : 0x00U);
	}
	g2 = CGolay24128::encode23127(c3);
	for (int i = 23; i >= 0; i--) {
		bit[i] = (g2 & 0x01U) == 0x01U;
		g2 >>= 1;
	}
	bit += 23U;

	// c4
	for (unsigned int i = 0U; i < 11U; i++)
		bit[i] = READ_BIT(imbe, i + 48U);
	CHamming::encode15113_1(bit);
	bit += 15U;

	// c5
	for (unsigned int i = 0U; i < 11U; i++)
		bit[i] = READ_BIT(imbe, i + 59U);
	CHamming::encode15113_1(bit);
	bit += 15U;

	// c6
	for (unsigned int i = 0U; i < 11U; i++)
		bit[i] = READ_BIT(imbe, i + 70U);
	CHamming::encode15113_1(bit);
	bit += 15U;

	// c7
	for (unsigned int i = 0U; i < 7U; i++)
		bit[i] = READ_BIT(imbe, i + 81U);

	bool prn[114U];

	// Create the whitening vector and save it for future use
	unsigned int p = 16U * c0;
	for (unsigned int i = 0U; i < 114U; i++) {
		p = (173U * p + 13849U) % 65536U;
		prn[i] = p >= 32768U;
	}

	// Whiten some bits
	for (unsigned int i = 0U; i < 114U; i++)
		bTemp[i + 23U] ^= prn[i];

	unsigned char temp[18U];

	// Interleave
	for (unsigned int i = 0U; i < 144U; i++) {
		unsigned int n = IMBE_INTERLEAVE[i];
		WRITE_BIT(temp, n, bTemp[i]);
	}

	switch (n) {
	case 0U:
		CP25Utils::encode(temp, data, 114U, 262U);
		break;
	case 1U:
		CP25Utils::encode(temp, data, 262U, 410U);
		break;
	case 2U:
		CP25Utils::encode(temp, data, 452U, 600U);
		break;
	case 3U:
		CP25Utils::encode(temp, data, 640U, 788U);
		break;
	case 4U:
		CP25Utils::encode(temp, data, 830U, 978U);
		break;
	case 5U:
		CP25Utils::encode(temp, data, 1020U, 1168U);
		break;
	case 6U:
		CP25Utils::encode(temp, data, 1208U, 1356U);
		break;
	case 7U:
		CP25Utils::encode(temp, data, 1398U, 1546U);
		break;
	case 8U:
		CP25Utils::encode(temp, data, 1578U, 1726U);
		break;
	default:
		return;
	}
}

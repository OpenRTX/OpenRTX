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

#include <P25/P25NID.h>
#include <P25/P25Defines.h>
#include <P25/P25Utils.h>
#include <P25/BCH.h>

#include <cstdio>
#include <cassert>

const unsigned int MAX_NID_ERRS = 5U;

CP25NID::CP25NID(unsigned int nac) :
m_duid(0U),
m_hdr(NULL),
m_ldu1(NULL),
m_ldu2(NULL),
m_termlc(NULL),
m_term(NULL)
{
	CBCH bch;

	m_hdr = new unsigned char[P25_NID_LENGTH_BYTES];
	m_hdr[0U]  = (nac >> 4) & 0xFFU;
	m_hdr[1U]  = (nac << 4) & 0xF0U;
	m_hdr[1U] |= P25_DUID_HEADER;
	bch.encode(m_hdr);
	m_hdr[7U] &= 0xFEU;		// Clear the parity bit

	m_ldu1 = new unsigned char[P25_NID_LENGTH_BYTES];
	m_ldu1[0U]  = (nac >> 4) & 0xFFU;
	m_ldu1[1U]  = (nac << 4) & 0xF0U;
	m_ldu1[1U] |= P25_DUID_LDU1;
	bch.encode(m_ldu1);
	m_ldu1[7U] |= 0x01U;	// Set the parity bit

	m_ldu2 = new unsigned char[P25_NID_LENGTH_BYTES];
	m_ldu2[0U]  = (nac >> 4) & 0xFFU;
	m_ldu2[1U]  = (nac << 4) & 0xF0U;
	m_ldu2[1U] |= P25_DUID_LDU2;
	bch.encode(m_ldu2);
	m_ldu2[7U] |= 0x01U;	// Set the parity bit

	m_termlc = new unsigned char[P25_NID_LENGTH_BYTES];
	m_termlc[0U]  = (nac >> 4) & 0xFFU;
	m_termlc[1U]  = (nac << 4) & 0xF0U;
	m_termlc[1U] |= P25_DUID_TERM_LC;
	bch.encode(m_termlc);
	m_termlc[7U] &= 0xFEU;		// Clear the parity bit

	m_term = new unsigned char[P25_NID_LENGTH_BYTES];
	m_term[0U]  = (nac >> 4) & 0xFFU;
	m_term[1U]  = (nac << 4) & 0xF0U;
	m_term[1U] |= P25_DUID_TERM;
	bch.encode(m_term);
	m_term[7U] &= 0xFEU;		// Clear the parity bit
}

CP25NID::~CP25NID()
{
	delete[] m_hdr;
	delete[] m_ldu1;
	delete[] m_ldu2;
	delete[] m_termlc;
	delete[] m_term;
}

bool CP25NID::decode(const unsigned char* data)
{
	assert(data != NULL);

	unsigned char nid[P25_NID_LENGTH_BYTES];
	CP25Utils::decode(data, nid, 48U, 114U);

	unsigned int errs = CP25Utils::compare(nid, m_ldu1, P25_NID_LENGTH_BYTES);
	if (errs < MAX_NID_ERRS) {
		m_duid = P25_DUID_LDU1;
		return true;
	}

	errs = CP25Utils::compare(nid, m_ldu2, P25_NID_LENGTH_BYTES);
	if (errs < MAX_NID_ERRS) {
		m_duid = P25_DUID_LDU2;
		return true;
	}

	errs = CP25Utils::compare(nid, m_term, P25_NID_LENGTH_BYTES);
	if (errs < MAX_NID_ERRS) {
		m_duid = P25_DUID_TERM;
		return true;
	}

	errs = CP25Utils::compare(nid, m_termlc, P25_NID_LENGTH_BYTES);
	if (errs < MAX_NID_ERRS) {
		m_duid = P25_DUID_TERM_LC;
		return true;
	}

	errs = CP25Utils::compare(nid, m_hdr, P25_NID_LENGTH_BYTES);
	if (errs < MAX_NID_ERRS) {
		m_duid = P25_DUID_HEADER;
		return true;
	}

	return false;
}

void CP25NID::encode(unsigned char* data, unsigned char duid) const
{
	assert(data != NULL);

	switch (duid) {
	case P25_DUID_HEADER:
		CP25Utils::encode(m_hdr, data, 48U, 114U);
		break;
	case P25_DUID_LDU1:
		CP25Utils::encode(m_ldu1, data, 48U, 114U);
		break;
	case P25_DUID_LDU2:
		CP25Utils::encode(m_ldu2, data, 48U, 114U);
		break;
	case P25_DUID_TERM:
		CP25Utils::encode(m_term, data, 48U, 114U);
		break;
	case P25_DUID_TERM_LC:
		CP25Utils::encode(m_termlc, data, 48U, 114U);
		break;
	default:
		break;
	}
}

unsigned char CP25NID::getDUID() const
{
	return m_duid;
}

/*
 *   Copyright (C) 2016,2017,2020 by Jonathan Naylor G4KLX
 *   Copyright (C) 2017 by Andy Uribe CA6JAU
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
 *
 *   (2025) Modified by KD0OSS for P25 on Module17/OpenRTX
 */
//#ifdef CONFIG_P25

#include <P25/P25TX.h>

#include <P25/P25Defines.h>
//#include <drivers/USART3_MOD17.h> // used for debugging
#include "string.h"
#include "stdio.h"

// Generated using rcosdesign(0.2, 8, 5, 'normal') in MATLAB
// numTaps = 40, L = 5
static q15_t RC_0_2_FILTER[] = {-897, -1636, -1840, -1278, 0, 1613, 2936, 3310, 2315, 0, -3011, -5627, -6580, -4839,
                                0, 7482, 16311, 24651, 30607, 32767, 30607, 24651, 16311, 7482, 0, -4839, -6580, -5627,
                               -3011, 0, 2315, 3310, 2936, 1613, 0, -1278, -1840, -1636, -897, 0}; // numTaps = 40, L = 5
const uint16_t RC_0_2_FILTER_PHASE_LEN = 8U; // phaseLength = numTaps/L

static q15_t LOWPASS_FILTER[] = {124, -188, -682, 1262, 556, -621, -1912, -911, 2058, 3855, 1234, -4592, -7692, -2799,
                                8556, 18133, 18133, 8556, -2799, -7692, -4592, 1234, 3855, 2058, -911, -1912, -621,
                                556, 1262, -682, -188, 124};
const uint16_t LOWPASS_FILTER_LEN = 32U;

const q15_t P25_LEVELA =  1260;
const q15_t P25_LEVELB =   420;
const q15_t P25_LEVELC =  -420;
const q15_t P25_LEVELD = -1260;

const uint8_t P25_START_SYNC = 0x77U;
const uint16_t TX_BUFFER_LEN = 4000U;

extern bool p25_tx;
extern bool p25_duplex;
extern CRingBuffer<uint16_t> p25_txBuffer;

static const uint16_t DC_OFFSET = 0;

CP25TX::CP25TX() :
		m_buffer(TX_BUFFER_LEN),
		m_modFilter(),
		m_lpFilter(),
		m_modState(),
		m_lpState(),
		m_poBuffer(),
		m_poLen(0U),
		m_poPtr(0U),
		m_txDelay(240U),      // 200ms
		m_txHang(6000U),      // 5s
		m_txCount(0U)
{
	::memset(m_modState, 0x00U, 16U * sizeof(q15_t));
	::memset(m_lpState,  0x00U, 60U * sizeof(q15_t));

	m_modFilter.L           = P25_RADIO_SYMBOL_LENGTH;
	m_modFilter.phaseLength = RC_0_2_FILTER_PHASE_LEN;
	m_modFilter.pCoeffs     = RC_0_2_FILTER;
	m_modFilter.pState      = m_modState;

	m_lpFilter.numTaps = LOWPASS_FILTER_LEN;
	m_lpFilter.pState  = m_lpState;
	m_lpFilter.pCoeffs = LOWPASS_FILTER;
}

void CP25TX::process()
{
	if (m_poLen == 0U && m_buffer.getData() > 0U) {
		uint8_t length;
		m_buffer.get(length);
		for (uint8_t i = 0U; i < length; i++) {
			uint8_t c = 0U;
			m_buffer.get(c);
			m_poBuffer[m_poLen++] = c;
		}

		m_poPtr = 0U;
	}

	if (m_poLen > 0U) {
		uint16_t space = p25_txBuffer.getSpace();

		while (space > (4U * P25_RADIO_SYMBOL_LENGTH)) {
			uint8_t c = m_poBuffer[m_poPtr++];
			writeByte(c);

			space -= 4U * P25_RADIO_SYMBOL_LENGTH;

			if (m_poPtr >= m_poLen) {
				m_poPtr = 0U;
				m_poLen = 0U;
				return;
			}
		}
	} else if (m_txCount > 0U) {
		// Transmit silence until the hang timer has expired.
		uint16_t space = p25_txBuffer.getSpace();

		while (space > (4U * P25_RADIO_SYMBOL_LENGTH)) {
			writeSilence();

			space -= 4U * P25_RADIO_SYMBOL_LENGTH;
			m_txCount--;

			if (m_txCount == 0U)
				return;
		}
	}
}

void CP25TX::writeSyncFrames()
{
	m_buffer.reset();
	p25_txBuffer.reset();
	p25_tx = true;
	m_buffer.put(m_txDelay);
	for (uint16_t i = 0U; i < m_txDelay; i++)
		m_buffer.put(P25_START_SYNC);
}

uint8_t CP25TX::writeData(const uint8_t* data, uint16_t length)
{
	if (length < P25_TERM_FRAME_LENGTH_BYTES)
		return 4U;

	uint16_t space = m_buffer.getSpace();
	if (space < length)
		return 5U;

//	usart3_mod17_writeBlock((void*)"HDR\n", 4);
//	usart3_mod17_writeBlock((void*)data, length);
	m_buffer.put(length);
	for (uint8_t i = 0U; i < length; i++)
		m_buffer.put(data[i]);

	return 0U;
}

void CP25TX::writeByte(uint8_t c)
{
	uint16_t high = 0;
	q15_t inBuffer[4U];
	q15_t intBuffer[P25_RADIO_SYMBOL_LENGTH * 4U];
	q15_t outBuffer[P25_RADIO_SYMBOL_LENGTH * 4U];

	const uint8_t MASK = 0xC0U;

	for (uint8_t i = 0U; i < 4U; i++, c <<= 2) {
		switch (c & MASK) {
		case 0xC0U:
			inBuffer[i] = P25_LEVELA;
			break;
		case 0x80U:
			inBuffer[i] = P25_LEVELB;
			break;
		case 0x00U:
			inBuffer[i] = P25_LEVELC;
			break;
		default:
			inBuffer[i] = P25_LEVELD;
			break;
		}
	}

	::arm_fir_interpolate_q15(&m_modFilter, inBuffer, intBuffer, 4U);

	::arm_fir_fast_q15(&m_lpFilter, intBuffer, outBuffer, P25_RADIO_SYMBOL_LENGTH * 4U);

	for (uint16_t i = 0U; i < 20; i++) {
		q31_t res1 = outBuffer[i] * (m_txLevel * 128);
		q15_t res2 = q15_t(__SSAT((res1 >> 15), 16));
		uint16_t res3 = uint16_t((res2 * 14) + DC_OFFSET);

		p25_txBuffer.put(res3);
//		if (res3 > high) high = res3;
//		char buf[20];
//		sprintf(buf, "%u  %u\n", res3, high);
//		    usart3_mod17_writeBlock((void*)buf, strlen(buf));
	}
	//  io.write(STATE_P25, outBuffer, P25_RADIO_SYMBOL_LENGTH * 4U);
}

void CP25TX::writeSilence()
{
	q15_t inBuffer[4U] = {0x00U, 0x00U, 0x00U, 0x00U};
	q15_t intBuffer[P25_RADIO_SYMBOL_LENGTH * 4U];
	q15_t outBuffer[P25_RADIO_SYMBOL_LENGTH * 4U];

	::arm_fir_interpolate_q15(&m_modFilter, inBuffer, intBuffer, 4U);

	::arm_fir_fast_q15(&m_lpFilter, intBuffer, outBuffer, P25_RADIO_SYMBOL_LENGTH * 4U);

	for (uint16_t i = 0U; i < 20; i++) {
		q31_t res1 = outBuffer[i] * (m_txLevel * 128);
		q15_t res2 = q15_t(__SSAT((res1 >> 15), 16));
		uint16_t res3 = uint16_t((res2 * 14) + DC_OFFSET);

		p25_txBuffer.put(res3);
		//  usart3_mod17_writeBlock((void*)res3, 2);
	}
	//  io.write(STATE_P25, outBuffer, P25_RADIO_SYMBOL_LENGTH * 4U);
}

void CP25TX::setTXDelay(uint8_t delay)
{
	m_txDelay = 600U + uint16_t(delay) * 12U;        // 500ms + tx delay

	if (m_txDelay > 1200U)
		m_txDelay = 1200U;
}

uint8_t CP25TX::getSpace() const
{
	return m_buffer.getSpace() / P25_LDU_FRAME_LENGTH_BYTES;
}

void CP25TX::setTxLevel(uint8_t level)
{
	m_txLevel = level;
}

void CP25TX::setParams(uint8_t txHang)
{
	m_txHang = txHang * 1200U;
}

//#endif // if P25

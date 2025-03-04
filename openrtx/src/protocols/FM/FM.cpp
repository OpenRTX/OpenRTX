/*
 *   Copyright (C) 2020,2021 by Jonathan Naylor G4KLX
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
 *   (2025) Modified by KD0OSS for FM mode on Module17/OpenRTX
 */

//#if defined(MODE_FM)

#include "FM.h"
//#include <drivers/USART3_MOD17.h> // for debugging

const uint16_t FM_TX_BLOCK_SIZE = 100U;
const uint16_t FM_SERIAL_BLOCK_SIZE = 80U;//this is the number of sample pairs to send over serial. One sample pair is 3bytes.
//three times this value shall never exceed 252
const uint16_t FM_SERIAL_BLOCK_SIZE_BYTES = FM_SERIAL_BLOCK_SIZE * 3U;

extern CRingBuffer<int16_t> rxBuffer;
extern CRingBuffer<q15_t>   m_outputRFRB;

// table of CTCSS frequencies
extern const float ctcss_index[] = {0.0,67.0,69.3,71.9,74.4,77.0,79.7,82.5,85.4,
		                            88.5,91.5,94.8,97.4,100.0,103.5,107.2,110.9,
				                    114.8,118.8,123.0,127.3,131.8,136.5,141.3,
				                    146.2,151.4,156.7,162.2,167.9,173.8,179.9,
				                    186.2,192.8,203.5,210.7,218.1,225.7,233.6,241.8,250.3};


CFM::CFM() :
m_ctcssRX(),
m_squelch(),
m_filterStage1(  724,   1448,   724, 32768, -37895, 21352),//3rd order Cheby Filter 300 to 2700Hz, 0.2dB passband ripple, sampling rate 24kHz
m_filterStage2(32768,      0,-32768, 32768, -50339, 19052),
m_filterStage3(32768, -65536, 32768, 32768, -64075, 31460),
m_accessMode(1U),
m_cosInvert(false),
m_noiseSquelch(true),
m_rxLevel(1),
m_openSq(false),
m_inputRFRB(2401U),   // 100ms of audio + 1 sample
m_rfSignal(false)
{
}

void CFM::DEBUG1(const char *text)
{
//	usart3_mod17_writeBlock((void*)text, strlen(text));
//	usart3_mod17_writeBlock((void*)"\n", 1);
}

void CFM::samples(bool cos, q15_t* samples, uint8_t length)
{
	uint8_t i = 0U;
	for (; i < length; i++) {
		// ARMv7-M has hardware integer division
		q15_t currentSample = q15_t((q31_t(samples[i]) << 8) / m_rxLevel);

		if (m_noiseSquelch)
			cos = m_squelch.process(currentSample);

		switch (m_accessMode) {
		case 0U:
			if (!cos)
			{
				m_openSq = false;
			    continue;
			}
			m_openSq = true;
			break;

		case 1U: {
			bool ctcss = m_ctcssRX.process(currentSample);
			// Delay the audio by 100ms to better match the CTCSS detector output
			m_inputRFRB.put(currentSample);
			m_inputRFRB.get(currentSample);

			if (!ctcss) {
				// No CTCSS detected, just carry on
				m_rfSignal = false;
				continue;
			} else if ((ctcss)) {
				m_rfSignal = true;
				// We had CTCSS
			}
		}
		break;

		case 2U: {
			bool ctcss = m_ctcssRX.process(currentSample);
			if (!ctcss) {
				// No CTCSS detected, just carry on
				m_rfSignal = false;
				m_openSq = false;
				continue;
			} else if ((ctcss && cos)) {
				// We had CTCSS
				m_openSq = true;
				m_rfSignal = true;
			}
		}
		break;

		default: {
			bool ctcss = m_ctcssRX.process(currentSample);
			if (!ctcss) {
				// No CTCSS detected, just carry on
				m_openSq = false;
				continue;
			} else if ((ctcss && cos)) {
				// We had CTCSS
				m_openSq = true;
			}
		}
		break;
		}

		currentSample = m_filterStage3.filter(m_filterStage2.filter(m_filterStage1.filter(currentSample)));

		if (m_outputRFRB.getSpace() >= 1)
			m_outputRFRB.put(currentSample);
	}
}

void CFM::reset()
{
	m_ctcssRX.reset();

	m_outputRFRB.reset();

	m_squelch.reset();

	m_rfSignal = false;
	m_openSq = false;
}

uint8_t CFM::setMisc(uint8_t ctcssFrequency, uint8_t ctcssHighThreshold, uint8_t ctcssLowThreshold, uint8_t accessMode, bool cosInvert, bool noiseSquelch, uint8_t squelchHighThreshold, uint8_t squelchLowThreshold,uint8_t maxDev, uint8_t rxLevel)
{
	m_accessMode   = accessMode;
	m_cosInvert    = cosInvert;
	m_noiseSquelch = noiseSquelch;

	m_rxLevel = rxLevel;

	m_squelch.setParams(squelchHighThreshold, squelchLowThreshold);

	uint8_t ret = m_ctcssRX.setParams(ctcssFrequency, ctcssHighThreshold, ctcssLowThreshold);
	if (ret != 0U)
		return ret;

	return 0U;
}
//#endif

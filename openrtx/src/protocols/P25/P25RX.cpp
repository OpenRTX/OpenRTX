/*
 *   Copyright (C) 2009-2017,2020 by Jonathan Naylor G4KLX
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
 *
 *   (2025) Modified by KD0OSS for P25 on Module17/OpenRTX
 */
#ifdef CONFIG_P25
#include <P25/P25RX.h>
#include <DSTAR/Utils.h>
//#include <drivers/USART3_MOD17.h> // for debugging
#include <stdio.h>
#include <DSTAR/RingBuffer.h>

const q15_t SCALING_FACTOR = 18750;      // Q15(0.57)

const uint8_t CORRELATION_COUNTDOWN = 10U;//5U;

const uint8_t MAX_SYNC_BIT_START_ERRS = 2U;
const uint8_t MAX_SYNC_BIT_RUN_ERRS   = 4U;

const uint8_t MAX_SYNC_SYMBOLS_ERRS = 2U;

const uint8_t BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])

const uint8_t NOAVEPTR = 99U;

const uint16_t NOENDPTR = 9999U;

const unsigned int MAX_SYNC_FRAMES = 4U + 1U;

extern CRingBuffer<uint8_t> p25LDUBuffer;

CP25RX::CP25RX() :
m_state(P25RXS_NONE),
m_bitBuffer(),
m_buffer(),
m_bitPtr(0U),
m_dataPtr(0U),
m_hdrStartPtr(NOENDPTR),
m_lduStartPtr(NOENDPTR),
m_lduEndPtr(NOENDPTR),
m_minSyncPtr(NOENDPTR),
m_maxSyncPtr(NOENDPTR),
m_hdrSyncPtr(NOENDPTR),
m_lduSyncPtr(NOENDPTR),
m_maxCorr(0),
m_lostCount(0U),
m_countdown(0U),
m_centre(),
m_centreVal(0),
m_threshold(),
m_thresholdVal(0),
m_averagePtr(NOAVEPTR),
m_rssiAccum(0U),
m_rssiCount(0U),
m_duid(0U),
m_synced(false)
{
}

void CP25RX::reset()
{
	m_state         = P25RXS_NONE;
	m_dataPtr       = 0U;
	m_bitPtr        = 0U;
	m_maxCorr       = 0;
	m_averagePtr    = NOAVEPTR;
	m_hdrStartPtr   = NOENDPTR;
	m_lduStartPtr   = NOENDPTR;
	m_lduEndPtr     = NOENDPTR;
	m_hdrSyncPtr    = NOENDPTR;
	m_lduSyncPtr    = NOENDPTR;
	m_minSyncPtr    = NOENDPTR;
	m_maxSyncPtr    = NOENDPTR;
	m_centreVal     = 0;
	m_thresholdVal  = 0;
	m_lostCount     = 0U;
	m_countdown     = 0U;
	m_rssiAccum     = 0U;
	m_rssiCount     = 0U;
	m_duid          = 0U;
	m_synced        = false;
}

void CP25RX::samples(const q15_t* samples, uint16_t* rssi, uint8_t length)
{
	for (uint8_t i = 0U; i < length; i++) {
		q15_t sample = samples[i];

		m_rssiAccum += rssi[i];
		m_rssiCount++;

		m_bitBuffer[m_bitPtr] <<= 1;
		if (sample < 0)
			m_bitBuffer[m_bitPtr] |= 0x01U;

		m_buffer[m_dataPtr] = sample;

		switch (m_state) {
		case P25RXS_HDR:
			processHdr(sample);
			break;
		case P25RXS_LDU:
			processLdu(sample);
			break;
		default:
			processNone(sample);
			break;
		}

		m_dataPtr++;
		if (m_dataPtr >= P25_LDU_FRAME_LENGTH_SAMPLES) {
			m_dataPtr = 0U;
			m_duid = 0U;
		}

		m_bitPtr++;
		if (m_bitPtr >= P25_RADIO_SYMBOL_LENGTH)
			m_bitPtr = 0U;
	}
}

void CP25RX::processNone(q15_t sample)
{
	bool ret = correlateSync();
	if (ret) {
		// On the first sync, start the countdown to the state change
		if (m_countdown == 0U) {
			m_rssiAccum = 0U;
			m_rssiCount = 0U;

			//     io.setDecode(true);
			//     io.setADCDetection(true);

			m_averagePtr = NOAVEPTR;

			m_countdown = CORRELATION_COUNTDOWN;
		}
	}

	if (m_countdown > 0U)
		m_countdown--;

	if (m_countdown == 1U) {
		// These are the sync positions for the following LDU after a HDR
		m_minSyncPtr = m_hdrSyncPtr + P25_HDR_FRAME_LENGTH_SAMPLES - 1U;
		if (m_minSyncPtr >= P25_LDU_FRAME_LENGTH_SAMPLES)
			m_minSyncPtr -= P25_LDU_FRAME_LENGTH_SAMPLES;

		m_maxSyncPtr = m_hdrSyncPtr + P25_HDR_FRAME_LENGTH_SAMPLES + 1U;
		if (m_maxSyncPtr >= P25_LDU_FRAME_LENGTH_SAMPLES)
			m_maxSyncPtr -= P25_LDU_FRAME_LENGTH_SAMPLES;

		m_state     = P25RXS_HDR;
		m_countdown = 0U;
	}
}

void CP25RX::processHdr(q15_t sample)
{
	if (m_minSyncPtr < m_maxSyncPtr) {
		if (m_dataPtr >= m_minSyncPtr && m_dataPtr <= m_maxSyncPtr)
			correlateSync();
	} else {
		if (m_dataPtr >= m_minSyncPtr || m_dataPtr <= m_maxSyncPtr)
			correlateSync();
	}

	if (m_dataPtr == m_maxSyncPtr) {
		uint16_t nidStartPtr = m_hdrStartPtr + P25_SYNC_LENGTH_SAMPLES;
		if (nidStartPtr >= P25_LDU_FRAME_LENGTH_SAMPLES)
			nidStartPtr -= P25_LDU_FRAME_LENGTH_SAMPLES;

		uint8_t nid[2U];
		samplesToBits(nidStartPtr, (2U * 4U), nid, 0U, m_centreVal, m_thresholdVal);
		// DEBUG3("P25RX: nid (b0 - b1)", nid[0U], nid[1U]);
		//char buf[25];
		//sprintf(buf, "P25RX: nid (%0X - %0X)\n", nid[0], nid[1]);
		//usart3_mod17_writeBlock((void*)buf, strlen(buf));

		m_duid = nid[1U] & 0x0F;

		switch (m_duid) {
		case P25_DUID_HDU: {
			calculateLevels(m_hdrStartPtr, P25_HDR_FRAME_LENGTH_SYMBOLS);

			//     DEBUG4("P25RX: sync found in Hdr pos/centre/threshold", m_hdrSyncPtr, m_centreVal, m_thresholdVal);
			m_synced = true;
			uint8_t frame[P25_HDR_FRAME_LENGTH_BYTES + 1U];
			samplesToBits(m_hdrStartPtr, P25_HDR_FRAME_LENGTH_SYMBOLS, frame, 8U, m_centreVal, m_thresholdVal);

			frame[0U] = 0x11U;
			//            serial.writeP25Hdr(frame, P25_HDR_FRAME_LENGTH_BYTES + 1U);
			//            usart3_mod17_writeBlock((void*)"HDU ", 4);
			//            usart3_mod17_writeBlock((void*)nid, 2);
			//            usart3_mod17_writeBlock((void*)"\n", 1);
			//            usart3_mod17_writeBlock((void*)frame, P25_HDR_FRAME_LENGTH_BYTES + 1U);
			//            char data[10];
			//            for (int i=0;i<14;i++)
			//    	    {
			//    	        sprintf(data, "%02X ", frame[i+1]);
			//    	        usart3_mod17_writeBlock((void*)data, strlen(data));
			//            }
			//        	usart3_mod17_writeBlock((void*)"\n", 1);
			//            for (int i=0;i<P25_HDR_FRAME_LENGTH_BYTES + 1U;i++)
			//                p25LDUBuffer.put(frame[i]);
		}
		break;
		case P25_DUID_PDU: {
			calculateLevels(m_hdrSyncPtr, P25_PDU_HDR_FRAME_LENGTH_SYMBOLS);

			//		DEBUG4("P25RX: sync found in PDU pos/centre/threshold", m_hdrSyncPtr, m_centreVal, m_thresholdVal);
			m_synced = true;

			uint8_t frame[P25_PDU_HDR_FRAME_LENGTH_BYTES + 1U];
			samplesToBits(m_hdrSyncPtr, P25_PDU_HDR_FRAME_LENGTH_SYMBOLS, frame, 8U, m_centreVal, m_thresholdVal);

			frame[0U] = 0x01U;
		//	usart3_mod17_writeBlock((void*)"PDU\n", 4);
		//	usart3_mod17_writeBlock((void*)nid, 2);
		//	usart3_mod17_writeBlock((void*)frame, P25_PDU_HDR_FRAME_LENGTH_BYTES + 1U);
			//			serial.writeP25Hdr(frame, P25_PDU_HDR_FRAME_LENGTH_BYTES + 1U);
		}
		break;
		case P25_DUID_TSDU: {
			calculateLevels(m_hdrStartPtr, P25_TSDU_FRAME_LENGTH_SYMBOLS);

			//      DEBUG4("P25RX: sync found in TSDU pos/centre/threshold", m_hdrSyncPtr, m_centreVal, m_thresholdVal);
			m_synced = true;

			uint8_t frame[P25_TSDU_FRAME_LENGTH_BYTES + 1U];
			samplesToBits(m_hdrStartPtr, P25_TSDU_FRAME_LENGTH_SYMBOLS, frame, 8U, m_centreVal, m_thresholdVal);

			frame[0U] = 0x01U;
		//	usart3_mod17_writeBlock((void*)"TSDU\n", 5);
		//	usart3_mod17_writeBlock((void*)nid, 2);
		//	usart3_mod17_writeBlock((void*)frame, P25_TSDU_FRAME_LENGTH_BYTES + 1U);
			//            serial.writeP25Hdr(frame, P25_TSDU_FRAME_LENGTH_BYTES + 1U);
		}
		break;
		case P25_DUID_TDU: {
			calculateLevels(m_hdrStartPtr, P25_TERM_FRAME_LENGTH_SYMBOLS);

			//      DEBUG4("P25RX: sync found in TDU pos/centre/threshold", m_hdrSyncPtr, m_centreVal, m_thresholdVal);
			m_synced = true;

			uint8_t frame[P25_TERM_FRAME_LENGTH_BYTES + 1U];
			samplesToBits(m_hdrStartPtr, P25_TERM_FRAME_LENGTH_SYMBOLS, frame, 8U, m_centreVal, m_thresholdVal);

			frame[0U] = 0x01U;
	//		usart3_mod17_writeBlock((void*)"TDU\n", 4);
	//		usart3_mod17_writeBlock((void*)nid, 2);
	//		usart3_mod17_writeBlock((void*)frame, P25_TERM_FRAME_LENGTH_BYTES + 1U);
			//            serial.writeP25Hdr(frame, P25_TERM_FRAME_LENGTH_BYTES + 1U);
		}
		break;
		case P25_DUID_TDULC: {
			calculateLevels(m_hdrStartPtr, P25_TERMLC_FRAME_LENGTH_SYMBOLS);

			//     DEBUG4("P25RX: sync found in TDULC pos/centre/threshold", m_hdrSyncPtr, m_centreVal, m_thresholdVal);
			m_synced = true;

			uint8_t frame[P25_TERMLC_FRAME_LENGTH_BYTES + 1U];
			samplesToBits(m_hdrStartPtr, P25_TERMLC_FRAME_LENGTH_SYMBOLS, frame, 8U, m_centreVal, m_thresholdVal);

			frame[0U] = 0x01U;
	//		usart3_mod17_writeBlock((void*)"TDULC\n", 6);
	//		usart3_mod17_writeBlock((void*)nid, 2);
	//		usart3_mod17_writeBlock((void*)frame, P25_TERMLC_FRAME_LENGTH_BYTES + 1U);
			//           serial.writeP25Hdr(frame, P25_TERMLC_FRAME_LENGTH_BYTES + 1U);
		}
		break;
		default:
			break;
		}

		m_minSyncPtr = m_lduSyncPtr + P25_LDU_FRAME_LENGTH_SAMPLES - 1U;
		if (m_minSyncPtr >= P25_LDU_FRAME_LENGTH_SAMPLES)
			m_minSyncPtr -= P25_LDU_FRAME_LENGTH_SAMPLES;

		m_maxSyncPtr = m_lduSyncPtr + 1U;
		if (m_maxSyncPtr >= P25_LDU_FRAME_LENGTH_SAMPLES)
			m_maxSyncPtr -= P25_LDU_FRAME_LENGTH_SAMPLES;

		m_state   = P25RXS_LDU;
		m_maxCorr = 0;
	}
}

void CP25RX::processLdu(q15_t sample)
{
	if (m_minSyncPtr < m_maxSyncPtr) {
		if (m_dataPtr >= m_minSyncPtr && m_dataPtr <= m_maxSyncPtr)
			correlateSync();
	} else {
		if (m_dataPtr >= m_minSyncPtr || m_dataPtr <= m_maxSyncPtr)
			correlateSync();
	}

	if (m_dataPtr == m_lduEndPtr) {
		// Only update the centre and threshold if they are from a good sync
		if (m_lostCount == MAX_SYNC_FRAMES) {
			m_minSyncPtr = m_lduSyncPtr + P25_LDU_FRAME_LENGTH_SAMPLES - 1U;
			if (m_minSyncPtr >= P25_LDU_FRAME_LENGTH_SAMPLES)
				m_minSyncPtr -= P25_LDU_FRAME_LENGTH_SAMPLES;

			m_maxSyncPtr = m_lduSyncPtr + 1U;
			if (m_maxSyncPtr >= P25_LDU_FRAME_LENGTH_SAMPLES)
				m_maxSyncPtr -= P25_LDU_FRAME_LENGTH_SAMPLES;
		}

		calculateLevels(m_lduStartPtr, P25_LDU_FRAME_LENGTH_SYMBOLS);

		//  DEBUG4("P25RX: sync found in Ldu pos/centre/threshold", m_lduSyncPtr, m_centreVal, m_thresholdVal);
		m_synced = true;

		uint8_t frame[P25_LDU_FRAME_LENGTH_BYTES + 3U];
		samplesToBits(m_lduStartPtr, P25_LDU_FRAME_LENGTH_SYMBOLS, frame, 8U, m_centreVal, m_thresholdVal);

		// We've not seen a data sync for too long, signal RXLOST and change to RX_NONE
		m_lostCount--;
		if (m_lostCount == 0U) {
			//   DEBUG1("P25RX: sync timed out, lost lock");
			m_synced = false;

			//     io.setDecode(false);
			//     io.setADCDetection(false);

			//     serial.writeP25Lost();
	//		usart3_mod17_writeBlock((void*)"P25LOST\n", 8);

			m_state      = P25RXS_NONE;
			m_lduEndPtr  = NOENDPTR;
			m_averagePtr = NOAVEPTR;
			m_countdown  = 0U;
			m_maxCorr    = 0;
			m_duid       = 0U;
		} else {
			frame[0U] = m_lostCount == (MAX_SYNC_FRAMES - 1U) ? 0x01U : 0x00U;
			writeRSSILdu(frame);
			m_maxCorr = 0;
		}
	}
}

bool CP25RX::correlateSync()
{
	if (countBits32((m_bitBuffer[m_bitPtr] & P25_SYNC_SYMBOLS_MASK) ^ P25_SYNC_SYMBOLS) <= MAX_SYNC_SYMBOLS_ERRS) {
		uint16_t ptr = m_dataPtr + P25_LDU_FRAME_LENGTH_SAMPLES - P25_SYNC_LENGTH_SAMPLES + P25_RADIO_SYMBOL_LENGTH;
		if (ptr >= P25_LDU_FRAME_LENGTH_SAMPLES)
			ptr -= P25_LDU_FRAME_LENGTH_SAMPLES;

		q31_t corr = 0;
		q15_t min =  16000;
		q15_t max = -16000;

		for (uint8_t i = 0U; i < P25_SYNC_LENGTH_SYMBOLS; i++) {
			q15_t val = m_buffer[ptr];

			if (val > max)
				max = val;
			if (val < min)
				min = val;

			switch (P25_SYNC_SYMBOLS_VALUES[i]) {
			case +3:
				corr -= (val + val + val);
				break;
			case +1:
				corr -= val;
				break;
			case -1:
				corr += val;
				break;
			default:  // -3
			corr += (val + val + val);
			break;
			}

			ptr += P25_RADIO_SYMBOL_LENGTH;
			if (ptr >= P25_LDU_FRAME_LENGTH_SAMPLES)
				ptr -= P25_LDU_FRAME_LENGTH_SAMPLES;
		}

		if (corr > m_maxCorr) {
			if (m_averagePtr == NOAVEPTR) {
				m_centreVal = (max + min) >> 1;

				q31_t v1 = (max - m_centreVal) * SCALING_FACTOR;
				m_thresholdVal = q15_t(v1 >> 15);
			}

			uint16_t startPtr = m_dataPtr + P25_LDU_FRAME_LENGTH_SAMPLES - P25_SYNC_LENGTH_SAMPLES + P25_RADIO_SYMBOL_LENGTH;
			if (startPtr >= P25_LDU_FRAME_LENGTH_SAMPLES)
				startPtr -= P25_LDU_FRAME_LENGTH_SAMPLES;

			uint8_t sync[P25_SYNC_BYTES_LENGTH];
			samplesToBits(startPtr, P25_SYNC_LENGTH_SYMBOLS, sync, 0U, m_centreVal, m_thresholdVal);

			uint8_t maxErrs;
			if (m_state == P25RXS_NONE)
				maxErrs = MAX_SYNC_BIT_START_ERRS;
			else
				maxErrs = MAX_SYNC_BIT_RUN_ERRS;

			uint8_t errs = 0U;
			for (uint8_t i = 0U; i < P25_SYNC_BYTES_LENGTH; i++)
				errs += countBits8(sync[i] ^ P25_SYNC_BYTES[i]);

			if (errs <= maxErrs) {
				m_maxCorr     = corr;
				m_lostCount   = MAX_SYNC_FRAMES;

				m_lduSyncPtr  = m_dataPtr;

				// These are the positions of the start and end of an LDU
				m_lduStartPtr = startPtr;

				m_lduEndPtr = m_dataPtr + P25_LDU_FRAME_LENGTH_SAMPLES - P25_SYNC_LENGTH_SAMPLES - 1U;
				if (m_lduEndPtr >= P25_LDU_FRAME_LENGTH_SAMPLES)
					m_lduEndPtr -= P25_LDU_FRAME_LENGTH_SAMPLES;

				if (m_state == P25RXS_NONE) {
					m_hdrSyncPtr = m_dataPtr;

					// This is the position of the start of a HDR
					m_hdrStartPtr = startPtr;

					// These are the range of positions for a sync for an LDU following a HDR
					m_minSyncPtr = m_dataPtr + P25_HDR_FRAME_LENGTH_SAMPLES - 1U;
					if (m_minSyncPtr >= P25_LDU_FRAME_LENGTH_SAMPLES)
						m_minSyncPtr -= P25_LDU_FRAME_LENGTH_SAMPLES;

					m_maxSyncPtr = m_dataPtr + P25_HDR_FRAME_LENGTH_SAMPLES + 1U;
					if (m_maxSyncPtr >= P25_LDU_FRAME_LENGTH_SAMPLES)
						m_maxSyncPtr -= P25_LDU_FRAME_LENGTH_SAMPLES;
				}

				return true;
			}
		}
	}

	return false;
}

void CP25RX::calculateLevels(uint16_t start, uint16_t count)
{
	q15_t maxPos = -16000;
	q15_t minPos =  16000;
	q15_t maxNeg =  16000;
	q15_t minNeg = -16000;

	for (uint16_t i = 0U; i < count; i++) {
		q15_t sample = m_buffer[start];

		if (sample > 0) {
			if (sample > maxPos)
				maxPos = sample;
			if (sample < minPos)
				minPos = sample;
		} else {
			if (sample < maxNeg)
				maxNeg = sample;
			if (sample > minNeg)
				minNeg = sample;
		}

		start += P25_RADIO_SYMBOL_LENGTH;
		if (start >= P25_LDU_FRAME_LENGTH_SAMPLES)
			start -= P25_LDU_FRAME_LENGTH_SAMPLES;
	}

	q15_t posThresh = (maxPos + minPos) >> 1;
	q15_t negThresh = (maxNeg + minNeg) >> 1;

	q15_t centre = (posThresh + negThresh) >> 1;

	q15_t threshold = posThresh - centre;

	// DEBUG5("P25RX: pos/neg/centre/threshold", posThresh, negThresh, centre, threshold);

	if (m_averagePtr == NOAVEPTR) {
		for (uint8_t i = 0U; i < 16U; i++) {
			m_centre[i]    = centre;
			m_threshold[i] = threshold;
		}

		m_averagePtr = 0U;
	} else {
		m_centre[m_averagePtr]    = centre;
		m_threshold[m_averagePtr] = threshold;

		m_averagePtr++;
		if (m_averagePtr >= 16U)
			m_averagePtr = 0U;
	}

	m_centreVal    = 0;
	m_thresholdVal = 0;

	for (uint8_t i = 0U; i < 16U; i++) {
		m_centreVal    += m_centre[i];
		m_thresholdVal += m_threshold[i];
	}

	m_centreVal    >>= 4;
	m_thresholdVal >>= 4;
}

void CP25RX::samplesToBits(uint16_t start, uint16_t count, uint8_t* buffer, uint16_t offset, q15_t centre, q15_t threshold)
{
	for (uint16_t i = 0U; i < count; i++) {
		q15_t sample = m_buffer[start] - centre;

		if (sample < -threshold) {
			WRITE_BIT1(buffer, offset, false);
			offset++;
			WRITE_BIT1(buffer, offset, true);
			offset++;
		} else if (sample < 0) {
			WRITE_BIT1(buffer, offset, false);
			offset++;
			WRITE_BIT1(buffer, offset, false);
			offset++;
		} else if (sample < threshold) {
			WRITE_BIT1(buffer, offset, true);
			offset++;
			WRITE_BIT1(buffer, offset, false);
			offset++;
		} else {
			WRITE_BIT1(buffer, offset, true);
			offset++;
			WRITE_BIT1(buffer, offset, true);
			offset++;
		}

		start += P25_RADIO_SYMBOL_LENGTH;
		if (start >= P25_LDU_FRAME_LENGTH_SAMPLES)
			start -= P25_LDU_FRAME_LENGTH_SAMPLES;
	}
}

void CP25RX::writeRSSILdu(uint8_t* ldu)
{
#if defined(SEND_RSSI_DATA)
	if (m_rssiCount > 0U) {
		uint16_t rssi = m_rssiAccum / m_rssiCount;

		ldu[217U] = (rssi >> 8) & 0xFFU;
		ldu[218U] = (rssi >> 0) & 0xFFU;

//		usart3_mod17_writeBlock((void*)"LDU1\n", 5);
//		serial.writeP25Ldu(ldu, P25_LDU_FRAME_LENGTH_BYTES + 3U);
//		usart3_mod17_writeBlock((void*)ldu, P25_LDU_FRAME_LENGTH_BYTES + 1U);
	} else {
//		usart3_mod17_writeBlock((void*)"LDU2\n", 5);
//		serial.writeP25Ldu(ldu, P25_LDU_FRAME_LENGTH_BYTES + 1U);
//		usart3_mod17_writeBlock((void*)ldu, P25_LDU_FRAME_LENGTH_BYTES + 1U);
	}
#else
	//  serial.writeP25Ldu(ldu, P25_LDU_FRAME_LENGTH_BYTES + 1U);
	//  usart3_mod17_writeBlock((void*)"LDU3\n", 5);
	//  usart3_mod17_writeBlock((void*)ldu, P25_LDU_FRAME_LENGTH_BYTES);
	if (p25LDUBuffer.getSpace() < P25_LDU_FRAME_LENGTH_BYTES + 1U)
	{
		//usart3_mod17_writeBlock((void*)"Full\n", 5);
		return;
	}
	for (unsigned int i=0;i<P25_LDU_FRAME_LENGTH_BYTES + 1U;i++)
		p25LDUBuffer.put(ldu[i]);
#endif

	m_rssiAccum = 0U;
	m_rssiCount = 0U;
}

#endif // if P25

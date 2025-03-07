/*
 *   Copyright (C) 2009-2016 by Jonathan Naylor G4KLX
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
 *   (2025) Modified by KD0OSS for use in Module17/OpenRTX
 */

#include <DSTAR/DStar_IO.hpp>
#include <DSTAR/DStar.h>
#include <DSTAR/RingBuffer.h>
#include <interfaces/platform.h>
#include <audio_stream.h>
#include <string.h>
#include <cstdint>
#include <string>
#include <dsp.h>
#include <crc.h>
#include <calibInfo_Mod17.h>
//#include <drivers/USART3_MOD17.h> // used for debugging

extern mod17Calib_t mod17CalData;

// Generated using gaussfir(0.5, 4, 5) in MATLAB
static q15_t   GMSK_FILTER[] = {8, 104, 760, 3158, 7421, 9866, 7421, 3158, 760, 104, 8, 0};
const uint16_t GMSK_FILTER_LEN = 12U;

const uint16_t DC_OFFSET = 2048U;
const uint16_t RX_BLOCK_SIZE = 2U;

const uint16_t TX_RINGBUFFER_SIZE = 10000U;
const uint16_t RX_RINGBUFFER_SIZE = 600U;

using namespace std;

bool        m_tx = false;
bool        pttInvert = false;

CRingBuffer<uint16_t> m_txBuffer(TX_RINGBUFFER_SIZE);
CRingBuffer<uint16_t> m_rxBuffer(RX_RINGBUFFER_SIZE);

DStar_IO::DStar_IO() :
		m_started(false),
		m_GMSKFilter(),
		m_GMSKState(),
		m_rxLevel(128),
		m_ledCount(0U),
		m_ledValue(true),
		m_pttInvert(false),
		m_detect(false),
		m_count(0U),
		m_watchdog(0U),
		m_lockout(false)
{
	::memset(m_GMSKState,  0x00U, 40U * sizeof(q15_t));

	m_GMSKFilter.numTaps = GMSK_FILTER_LEN;
	m_GMSKFilter.pState  = m_GMSKState;
	m_GMSKFilter.pCoeffs = GMSK_FILTER;

	baseband_rxbuffer = std::make_unique< int16_t[] >(2 * 96);
	dstarRX.reset();
	pttInvert = m_pttInvert;
	m_rxLevel = mod17CalData.dstar_rx_level;
	dstarTX.setTXLevel(mod17CalData.dstar_tx_level);
}

void DStar_IO::reset()
{
	dstarRX.reset();
	pttInvert = m_pttInvert;
	m_rxLevel = mod17CalData.dstar_rx_level;
	dstarTX.setTXLevel(mod17CalData.dstar_tx_level);
}

bool DStar_IO::isLocked()
{
	return locked;
}

void DStar_IO::terminate()
{
	// Ensure proper termination of baseband sampling
	audioPath_release(basebandPath);
	audioStream_terminate(basebandId);

}

void DStar_IO::startBasebandSampling()
{
	basebandPath = audioPath_request(SOURCE_RTX, SINK_MCU, PRIO_RX);
	basebandId = audioStream_start(basebandPath, baseband_rxbuffer.get(),
			2 * 96, RX_SAMPLE_RATE,
			STREAM_INPUT | BUF_CIRC_DOUBLE);

	//    reset();
}

void DStar_IO::stopBasebandSampling()
{
	audioStream_terminate(basebandId);
	audioPath_release(basebandPath);
	locked = false;
}

void DStar_IO::init_tx()
{
	/*
	 * Allocate a chunk of memory to contain two complete buffers for baseband
	 * audio.
	 */

	baseband_txbuffer = std::make_unique< int16_t[] >(2 * 40);
	idleBuffer      = baseband_txbuffer.get();
	txRunning       = false;
}

void DStar_IO::terminate_tx()
{
	// Terminate an ongoing stream, if present
	if(txRunning)
	{
		audioStream_terminate(outStream);
		txRunning = false;
	}

	// Always ensure that outgoing audio path is closed
	audioPath_release(outPath);

	// Deallocate memory.
	baseband_txbuffer.reset();
}

bool DStar_IO::start_tx()
{
	if(txRunning)
		return true;

	outPath = audioPath_request(SOURCE_MCU, SINK_RTX, PRIO_TX);
	if(outPath < 0)
		return false;

	outStream = audioStream_start(outPath, baseband_txbuffer.get(),
			2*40, TX_SAMPLE_RATE,
			STREAM_OUTPUT | BUF_CIRC_DOUBLE);

	if(outStream < 0)
		return false;

	idleBuffer = outputStream_getIdleBuffer(outStream);

	txRunning = true;

	return true;
}

void DStar_IO::stop_tx()
{
	m_tx = false;
	if(txRunning == false)
		return;

	audioStream_stop(outStream);
	txRunning  = false;
	idleBuffer = baseband_txbuffer.get();
	audioPath_release(outPath);
}

void DStar_IO::setMode()
{
	m_dstarEnable = true;
	m_modemState = STATE_IDLE;
}

bool DStar_IO::update(const bool invertPhase)
{
	// Audio path closed, nothing to do
	if(audioPath_getStatus(basebandPath) != PATH_OPEN)
		return false;

	// Read samples from the ADC
	dataBlock_t baseband = inputStream_getData(basebandId);
	if(baseband.data != NULL)
	{
		// Process samples
		for(size_t i = 0; i < baseband.len; i++)
		{
			uint16_t elem = static_cast< uint16_t >(baseband.data[i]);
			if(invertPhase) elem = 0 - elem;
			m_rxBuffer.put(elem);
			process();
		}

	}
	locked = dstarRX.m_synced;

	return dstarRX.m_synced;
}

void DStar_IO::update_tx(const bool invertPhase)
{
	uint16_t count = m_txBuffer.getData();

	if(audioPath_getStatus(outPath) != PATH_OPEN) return;

	memset(idleBuffer, 0x00, 40 * sizeof(stream_sample_t));

	while (count >= 40)
	{
		for (size_t i=0;i<40;i++)
		{
			uint16_t tmp;
			m_txBuffer.get(tmp);
			int16_t elem = static_cast< int16_t >(tmp);
			if(invertPhase) elem = 0 - elem;    // Invert signal phase
			idleBuffer[i] = elem;
			count--;
		}
		if (count >= 40)
		{
			outputStream_sync(outStream, true);
			idleBuffer = outputStream_getIdleBuffer(outStream);
			memset(idleBuffer, 0x00, 40 * sizeof(stream_sample_t));
		}
	}
	// Transmission is ongoing, syncronise with stream end before proceeding
	outputStream_sync(outStream, true);
	idleBuffer = outputStream_getIdleBuffer(outStream);
}

void DStar_IO::start()
{
	if (m_started)
		return;

	m_count   = 0U;
	m_started = true;

	dstarRX.reset();

	setMode();
}

void DStar_IO::slowSpeedDataEncode(char *cMessage, unsigned char *ucBytes, unsigned char ucMode)
{
	static int           iIndex;
	static unsigned char ucFrameCount;

	if (ucMode == 10)
	{
		ucFrameCount = 0;
		return;
	}

	int iSyncfrm = ucFrameCount % 21;

	if (ucFrameCount > 251)
		ucFrameCount = 0;
	else
		ucFrameCount++;

	//   printf("m: %s\n", cMessage);

	if (iSyncfrm == 0)
	{
		ucBytes[0] = 0x25 ^ 0x70;
		ucBytes[1] = 0x62 ^ 0x4f;
		ucBytes[2] = 0x85 ^ 0x93;
		iIndex = 0;
		return;
	}

	if (ucMode == 1)
	{
		if (iIndex <= 17)
		{
			int iSection = iIndex % 5;
			if (iSection == 0)
			{
				ucBytes[0] = (unsigned char)((0x40 + iIndex/5) ^ 0x70);
				ucBytes[1] = (unsigned char)cMessage[iIndex++] ^ 0x4f;
				ucBytes[2] = (unsigned char)cMessage[iIndex++] ^ 0x93;
			}
			else
			{
				ucBytes[0] = (unsigned char)cMessage[iIndex++] ^ 0x70;
				ucBytes[1] = (unsigned char)cMessage[iIndex++] ^ 0x4f;
				ucBytes[2] = (unsigned char)cMessage[iIndex++] ^ 0x93;
			}
		}
		else
		{
			ucBytes[0] = 0x66 ^ 0x70;
			ucBytes[1] = 0x66 ^ 0x4f;
			ucBytes[2] = 0x66 ^ 0x93;
		}
	}
} // end slowSpeedDataEncode

void DStar_IO::setPTTInt(bool ptt)
{
	platform_PTT(ptt);
}

void DStar_IO::process()
{
	m_ledCount++;
	if (m_started) {
		// Two seconds timeout
		if (m_watchdog >= 48000U) {
			if (m_modemState == STATE_DSTAR || m_modemState == STATE_DMR || m_modemState == STATE_YSF) {
				m_modemState = STATE_IDLE;
				setMode();
			}

			m_watchdog = 0U;
		}

		if (m_ledCount >= 24000U) {
			m_ledCount = 0U;
			m_ledValue = !m_ledValue;
		}
	} else {
		if (m_ledCount >= 24000U) {
			m_ledCount = 0U;
			m_ledValue = !m_ledValue;
		}
		return;
	}

	// Switch off the transmitter if needed
	if (m_txBuffer.getData() == 0U && m_tx) {
		m_tx = false;
		setPTTInt(m_pttInvert ? true : false);
	}

	if (m_rxBuffer.getData() >= RX_BLOCK_SIZE) {
		q15_t    samples[RX_BLOCK_SIZE + 1U];
		uint8_t  control[RX_BLOCK_SIZE + 1U];

		uint8_t blockSize = RX_BLOCK_SIZE;

		for (uint16_t i = 0U; i < RX_BLOCK_SIZE; i++) {
			uint16_t sample;
			m_rxBuffer.get(sample);

			q15_t res1 = q15_t(sample) - DC_OFFSET;
			q31_t res2 = res1 * (m_rxLevel * 128);
			samples[i] = q15_t(__SSAT((res2 >> 15), 16));
		}

		if (m_lockout)
			return;

		if (m_modemState == STATE_IDLE) {
			if (m_dstarEnable) {
				q15_t GMSKVals[RX_BLOCK_SIZE + 1U];
				::arm_fir_fast_q15(&m_GMSKFilter, samples, GMSKVals, blockSize);

				dstarRX.samples(GMSKVals, blockSize);
			}
		} else if (m_modemState == STATE_DSTAR) {
			if (m_dstarEnable) {
				q15_t GMSKVals[RX_BLOCK_SIZE + 1U];
				::arm_fir_fast_q15(&m_GMSKFilter, samples, GMSKVals, blockSize);

				dstarRX.samples(GMSKVals, blockSize);
			}
		}
	}
}

void DStar_IO::setTxHeader(const uint8_t *header, uint8_t len)
{
	addCCITT161((unsigned char*)header, len);
	dstarTX.writeHeader(header, len);
}

void DStar_IO::setTxData(uint8_t *data, uint8_t len)
{
	dstarTX.writeData(data, len);
}

void DStar_IO::writeEOT()
{
	dstarTX.writeEOT();
}

void DStar_IO::process_tx()
{
	dstarTX.process();
}

void DStar_IO::setTXDelay(uint8_t delay)
{
	dstarTX.setTXDelay(delay);
}

void DStar_IO::getMyCall(char *myCall)
{
	char call[9] = "";

	dstarRX.getMyCall(call);
	strcpy(myCall, call);
}

void DStar_IO::getUrCall(char *urCall)
{
	char call[9] = "";

	dstarRX.getUrCall(call);
	strcpy(urCall, call);
}

void DStar_IO::getRpt1Call(char *rpt1Call)
{
	char call[9] = "";

	dstarRX.getRpt1Call(call);
	strcpy(rpt1Call, call);
}

void DStar_IO::getRpt2Call(char *rpt2Call)
{
	char call[9] = "";

	dstarRX.getRpt2Call(call);
	strcpy(rpt2Call, call);
}

void DStar_IO::getSuffix(char *suffix)
{
	char suf[5] = "";

	dstarRX.getSuffix(suf);
	strcpy(suffix, suf);
}

void DStar_IO::getText(char *text)
{
	char txt[21] = "";

	dstarRX.getText(txt);
	strcpy(text, txt);
}

void DStar_IO::getData(uint8_t *data)
{
	uint8_t d[9];

	dstarRX.getData(d);
	::memcpy(data, d, 9);
}

/***************************************************************************
 *   Copyright (C) 2022 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 *                                                                         *
 *   Modified by KD0OSS for P25 on Module17/OpenRTX                        *
 ***************************************************************************/
#ifdef CONFIG_P25
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <interfaces/audio.h>
#include <interfaces/radio.h>
#include <OpMode_P25.hpp>
#include <cstdint>
#include <imbe_audio_codec.h>
#include <errno.h>
#include <rtx.h>
#include <state.h>
#include <settings.h>
#include <drivers/USART3_MOD17.h> // for debugging
#include <DSTAR/RingBuffer.h>
#include <drivers/usb_vcom.h>

#ifdef PLATFORM_MOD17
#include <calibInfo_Mod17.h>

extern mod17Calib_t mod17CalData;
#endif

CRingBuffer<uint8_t>  p25LDUBuffer(2180);
CRingBuffer<uint8_t>  p25PacketBuffer(1100);

extern CRingBuffer<uint16_t> p25_txBuffer;

extern bool host_found;
extern bool p25_tx;
extern bool txing;

extern bool host_found;

OpMode_P25::OpMode_P25():
startRx(false),
startTx(false),
dataValid(false),
invertTxPhase(false),
invertRxPhase(false)
{
	m_srcid = 3119077;
	m_dstid = 11;
}

OpMode_P25::~OpMode_P25()
{
    disable();
}

void OpMode_P25::reset()
{
	p25_io.reset();
}

void OpMode_P25::enable()
{
    dataValid    = false;
    startRx      = true;
    startTx      = false;
	p25_tx       = false;

    imbe_init();
    p25_io.start();
    p25_io.init_tx();
}

void OpMode_P25::disable()
{
    dataValid    = false;
    startRx      = false;
    startTx      = false;

    p25_io.terminate_tx();
    p25_io.terminate();
	p25_io.stopBasebandSampling();
    imbe_stop(rxAudioPath);
    platform_ledOff(GREEN);
    platform_ledOff(RED);
    audioPath_release(rxAudioPath);
    audioPath_release(txAudioPath);
    imbe_terminate();
    radio_disableRtx();
}

void OpMode_P25::update(rtxStatus_t *const status, const bool newCfg)
{
    (void) newCfg;

    if(!host_found) // Check if P25 host has connected
    {
      	uint8_t buf[2];
       	uint8_t count = vcom_readBlock((uint8_t*)buf, 2);
       	if (count == 2)
       	{
       		if (buf[0] == 0x61 && buf[1] == 0x03)
       			host_found = true;
       	}
  //     	sleepFor(0, 100);
    }

#if defined(PLATFORM_MOD17)
//
// Get phase inversion settings from calibration.
//
    invertTxPhase = !(mod17CalData.bb_tx_invert == 1) ? true : false;
    invertRxPhase = !(mod17CalData.bb_rx_invert == 1) ? true : false;
#endif

    // Main FSM logic
    switch(status->opStatus)
    {
        case OFF:
            offState(status);
            break;

        case RX:
            rxState(status);
            break;

        case TX:
            txState(status);
            break;

        default:
            break;
    }

    // Led control logic
    switch(status->opStatus)
    {
        case RX:

            if(dataValid)
                platform_ledOn(GREEN);
            else
                platform_ledOff(GREEN);
            break;

        case TX:
            platform_ledOff(GREEN);
            platform_ledOn(RED);
            break;

        default:
            platform_ledOff(GREEN);
            platform_ledOff(RED);
            break;
    }
}

void OpMode_P25::offState(rtxStatus_t *const status)
{
    radio_disableRtx();

    if(startRx)
    {
        status->opStatus = RX;
        return;
    }

    if(platform_getPttStatus() && (status->txDisable == 0))
    {
        startTx = true;
        status->opStatus = TX;
        return;
    }

    // Sleep for 30ms if there is nothing else to do in order to prevent the
    // rtx thread looping endlessly and locking up all the other tasks
    sleepFor(0, 30);
}

void OpMode_P25::rxState(rtxStatus_t *const status)
{
	bool locked = false;

    if(startRx)
    {
        p25_io.startBasebandSampling();

        radio_enableRx();

        startRx = false;
    }

    bool newData = p25_io.update(invertRxPhase);

    if(newData)
    {
    	dataValid = true;
    	status->lsfOk = true;
    	locked = true;

    	if(imbe_running() == false)
    	{
    		uint8_t pthSts = audioPath_getStatus(rxAudioPath);
    		if(pthSts == PATH_CLOSED)
    		{
    			rxAudioPath = audioPath_request(SOURCE_MCU, SINK_SPK, PRIO_RX);
    			pthSts = audioPath_getStatus(rxAudioPath);
    		}
    		imbe_startDecode(rxAudioPath);
    	}

    	if(p25PacketBuffer.getSpace() > 99)
    		p25_io.decode();

    	status->P25_SrcId = p25_io.getSrcId();
    	status->P25_DstId = p25_io.getDstId();
/*
        uint8_t buf[17];
        buf[0] = 0x61;
        buf[1] = 0x00;
        buf[2] = 0x0d;
        buf[3] = 0x03; // Type = imbe packets for imbeDecoder
        buf[4] = 0x01;
        buf[5] = 0x48;

        while(p25PacketBuffer.getData() >= 11)
        {
        	for (int i=0;i<11;i++)
        	{
        		p25PacketBuffer.get(buf[6+i]);
        	}
        	if(host_found)
        	{
        		vcom_writeBlock((uint8_t*)buf, 17);
        	}
        } */
    }

    if(platform_getPttStatus() && host_found)
    {
    	locked = false;
    	p25_io.stopBasebandSampling();
    	status->opStatus = OFF;
    	imbe_stop(rxAudioPath);
    	audioPath_release(rxAudioPath);
    }

    // Force invalidation of LSF data as soon as lock is lost (for whatever cause)
    if(locked == false)
    {
    	while (p25LDUBuffer.getData() >= P25_LDU_FRAME_LENGTH_BYTES + 1U)
    		p25_io.decode();
    	p25LDUBuffer.reset();
    	dataValid = false;
    	status->lsfOk = false;
    }
}

void OpMode_P25::txState(rtxStatus_t *const status)
{
	if(!host_found) // receive only
	{
		sleepFor(0, 30);
		return;
	}

	if(startTx)
	{
        txAudioPath = audioPath_request(SOURCE_MIC, SINK_MCU, PRIO_TX);
		imbe_startEncode(txAudioPath);
	    txing = true;
		p25_tx = false;
		startTx = false;
		radio_enableTx();
		p25_io.start_tx(invertTxPhase);
		p25_io.setSrcId(state.settings.p25_srcId);
		p25_io.setDstId(state.settings.p25_dstId);
		p25_io.writeSyncFrames();
		p25_io.process_tx();
		p25_io.createTxHeader();
		p25_io.process_tx();
	}

	if(vcom_bytesReady() >= 17)
	{
		// look for start of IMBE packet
		uint8_t buf[16];
		vcom_readBlock(buf, 1);
		if(buf[0] == 0x61)
		{
			vcom_readBlock(buf, 16);
            for(size_t i = 5; i < 16; i++)
				p25PacketBuffer.put(buf[i]);

            if(p25PacketBuffer.getData() >= 198) // wait for 360ms of encoded audio
			{
				p25_io.createTxLDU1();
				p25_io.process_tx();
				p25_io.createTxLDU2();
				p25_io.process_tx();
			}

		}
	}
	else
		sleepFor(0, 1);

	if(platform_getPttStatus() == false)
	{
		imbe_stop(txAudioPath);
		audioPath_release(txAudioPath);
		p25_io.createTxTerminator();
		p25_io.process_tx();
		while(p25_txBuffer.getData() >= 20)
			sleepFor(0, 20);
    	txing = false;
		p25_io.stop_tx();
		p25_tx = false;
		startRx = true;
		status->opStatus = OFF;
	}
}
#endif // if P25

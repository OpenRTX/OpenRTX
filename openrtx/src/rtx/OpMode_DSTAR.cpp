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
 *   Modified by KD0OSS for DSTAR on Module17/OpenRTX                      *
 ***************************************************************************/

#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <interfaces/audio.h>
#include <interfaces/radio.h>
#include <OpMode_DSTAR.hpp>
#include <cstdint>
#include <errno.h>
#include <state.h>
#include <settings.h>
//#include <drivers/USART3_MOD17.h> // for debugging
#include <drivers/usb_vcom.h>
#include <DSTAR/RingBuffer.h>
#include <dstar_audio_codec.h>
#include <rtx.h>

#ifdef PLATFORM_MOD17
#include <calibInfo_Mod17.h>
//#include <interfaces/platform.h>

extern mod17Calib_t mod17CalData;
#endif

CRingBuffer<uint8_t>  ambeBuffer(900);
CRingBuffer<uint8_t>  packetBuffer(900);

extern bool host_found;

OpMode_DSTAR::OpMode_DSTAR():
startRx(false),
startTx(false),
dataValid(false),
invertTxPhase(false),
invertRxPhase(false)
{
}

OpMode_DSTAR::~OpMode_DSTAR()
{
    disable();
}

void OpMode_DSTAR::enable()
{
    dataValid    = false;
    startRx      = true;
    startTx      = false;

    ambe_init();
    dstar_io.start();
    dstar_io.init_tx();
}

void OpMode_DSTAR::disable()
{
    dataValid    = false;
    startRx      = false;
    startTx      = false;

    dstar_io.terminate_tx();
    dstar_io.terminate();
    ambe_stop(rxAudioPath);
    platform_ledOff(GREEN);
    platform_ledOff(RED);
    audioPath_release(rxAudioPath);
    audioPath_release(txAudioPath);
    ambe_terminate();
    radio_disableRtx();
}

void OpMode_DSTAR::update(rtxStatus_t *const status, const bool newCfg)
{
    (void) newCfg;

    while (!host_found) // Check if host has connected
    {
      	uint8_t buf[2];
       	uint8_t count = vcom_readBlock((uint8_t*)buf, 2);
       	if (count == 2)
       	{
       		if (buf[0] == 0x61 && buf[1] == 0x03)
       			host_found = true;
       	}
       	sleepFor(0, 100);
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

void OpMode_DSTAR::offState(rtxStatus_t *const status)
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

void OpMode_DSTAR::rxState(rtxStatus_t *const status)
{
    if(startRx)
    {
        dstar_io.startBasebandSampling();

        radio_enableRx();

        startRx = false;
    }

    bool newData = dstar_io.update(invertRxPhase);
    bool locked  = dstar_io.isLocked();

    if(locked && newData)
    {
        dataValid = true;

        if(ambe_running() == false)
        {
            uint8_t pthSts = audioPath_getStatus(rxAudioPath);
            if(pthSts == PATH_CLOSED)
            {
                rxAudioPath = audioPath_request(SOURCE_MCU, SINK_SPK, PRIO_RX);
                pthSts = audioPath_getStatus(rxAudioPath);
            }
            ambe_startDecode(rxAudioPath);
        }

        if (ambeBuffer.getData())
        {
            uint8_t buf[15];
            buf[0] = 0x61;
            buf[1] = 0x00;
            buf[2] = 0x0b;
            buf[3] = 0x01;
            buf[4] = 0x01;
            buf[5] = 0x48;

        	for (int i=0;i<9;i++)
        	{
        		ambeBuffer.get(buf[6+i]);
        	}
        	if(host_found)
        	{
        		vcom_writeBlock((uint8_t*)buf, 15);
        	}
        }
		dstar_io.getMyCall(status->DSTAR_src);
		dstar_io.getUrCall(status->DSTAR_dst);
		dstar_io.getSuffix(status->DSTAR_sufx);
		dstar_io.getRpt1Call(status->DSTAR_rpt1);
		dstar_io.getRpt2Call(status->DSTAR_rpt2);
		dstar_io.getText(status->DSTAR_message);
		status->lsfOk = true;
    }

    if(platform_getPttStatus())
    {
    	locked = false;
        dstar_io.stopBasebandSampling();
        status->opStatus = OFF;
        ambe_stop(rxAudioPath);
        audioPath_release(rxAudioPath);
    }

    // Force invalidation of LSF data as soon as lock is lost (for whatever cause)
    if(locked == false)
    {
        status->lsfOk = false;
        dataValid     = false;
        status->DSTAR_link[0] = '\0';
        status->DSTAR_refl[0] = '\0';
    }
}

void OpMode_DSTAR::txState(rtxStatus_t *const status)
{
    if(startTx)
    {
        startTx = false;
        txAudioPath = audioPath_request(SOURCE_MIC, SINK_MCU, PRIO_TX);
        ambe_startEncode(txAudioPath);
        radio_enableTx();
        dstar_io.start_tx();
        uint8_t header[41];
        header[0] = 0x40;
        header[1] = 0x00;
        header[2] = 0x00;
        char hdr[38]; //  example: "KD0OSS GKD0OSS BCQCQCQ  KD0OSS  MD17";
        memcpy(hdr, state.settings.dstar_rpt2call, 8);
        memcpy(hdr+8, state.settings.dstar_rpt1call, 8);
        memcpy(hdr+16, state.settings.dstar_urcall, 8);
        memcpy(hdr+24, state.settings.dstar_mycall, 8);
        memcpy(hdr+32, state.settings.dstar_suffix, 4);
        // Replace nulls and underscores introduced during header setup with spaces.
        for (size_t i=0;i<36;i++)
        {
        	if(hdr[i] == '_') hdr[i] = ' ';
        	if(hdr[i] == 0) hdr[i] = ' ';
        }
        hdr[36] = 0;
        hdr[37] = 0;
        memcpy(header+3, hdr, 41-3);
        dstar_io.setTxHeader(header, 41);
        dstar_io.process_tx(); // create start of transmission sync data
        dstar_io.process_tx(); // create header data
        dstar_io.update_tx(invertTxPhase);
        dstar_io.slowSpeedDataEncode(NULL, NULL, 10); // initialize sync data
    }

	if(vcom_bytesReady() >= 15) // wait for packet data from host
	{
		uint8_t ambe[12];
		uint8_t buf[11];
		vcom_readBlock(buf, 1);
		if(buf[0] == 0x61) // check for correct header byte
		{
			vcom_readBlock(buf, 14);
			for(size_t i = 5; i < 14; i++)
			{
				ambe[i-5] = buf[i];
			}
			dstar_io.slowSpeedDataEncode(state.settings.dstar_message, ambe+9, 1);
			dstar_io.setTxData(ambe, 12);
			dstar_io.process_tx();
			dstar_io.update_tx(invertTxPhase);
		}
	}

    if(platform_getPttStatus() == false)
    {
        ambe_stop(txAudioPath);
        audioPath_release(txAudioPath);
        if (vcom_bytesReady() < 15) // keep transmitting as long voice data in queue
        {
        	dstar_io.writeEOT();
        	dstar_io.process_tx();
        	dstar_io.update_tx(invertTxPhase);
        	dstar_io.stop_tx();
        	startRx = true;
        	status->opStatus = OFF;
        }
    }
}

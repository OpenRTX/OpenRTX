/***************************************************************************
 *   Copyright (C) 2021 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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
 *   (2025) Modified by KD0OSS for new modes on Module17                   *
 ***************************************************************************/

#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <interfaces/audio.h>
#include <interfaces/radio.h>
#include <M17/M17Callsign.hpp>
#include <core/state.h>
#include <OpMode_M17.hpp>
#include <audio_codec.h>
#include <vector>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <gps.h>
//#include <drivers/USART3_MOD17.h> // for debugging
#include <rtx.h>
//#include <drivers/usb_vcom.h>

#ifdef PLATFORM_MOD17
#include <calibInfo_Mod17.h>

extern mod17Calib_t mod17CalData;
#endif

using namespace std;
using namespace M17;

OpMode_M17::OpMode_M17() : startRx(false), startTx(false), locked(false),
                           dataValid(false), extendedCall(false),
                           invertTxPhase(false), invertRxPhase(false)
{

}

OpMode_M17::~OpMode_M17()
{
    disable();
}

void OpMode_M17::enable()
{
    codec_init();
    modulator.init();
    demodulator.init();
    smsSender.clear();
    smsMessage.clear();
    locked                 = false;
    dataValid              = false;
    extendedCall           = false;
    startRx                = true;
    startTx                = false;
    smsEnabled             = true;
    gpsEnabled             = true;
    totalSMSLength         = 0;
    state.totalSMSMessages = 0;
    state.havePacketData   = false;
}

void OpMode_M17::disable()
{
    startRx = false;
    startTx = false;
    platform_ledOff(GREEN);
    platform_ledOff(RED);
    audioPath_release(rxAudioPath);
    audioPath_release(txAudioPath);
    codec_terminate();
    radio_disableRtx();
    modulator.terminate();
    demodulator.terminate();
    for(size_t i=0;i<state.totalSMSMessages;i++)
    {
        free(smsSender[i]);
        free(smsMessage[i]);
    }
    smsSender.clear();
    smsMessage.clear();
}

bool OpMode_M17::getSMSMessage(uint8_t mesg_num, char *sender, char *message)
{
    if(state.totalSMSMessages == 0 || mesg_num > (state.totalSMSMessages - 1))
        return false;
    strcpy(sender, smsSender[mesg_num]);
    strcpy(message, smsMessage[mesg_num]);
    return true;
}

void OpMode_M17::delSMSMessage(uint8_t mesg_num)
{
    free(smsSender[mesg_num]);
    free(smsMessage[mesg_num]);
    smsSender.erase(smsSender.begin()+mesg_num);
    smsMessage.erase(smsMessage.begin()+mesg_num);
}

void OpMode_M17::update(rtxStatus_t *const status, const bool newCfg)
{
    (void) newCfg;
    #if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
    //
    // Invert TX phase for all MDx models.
    // Invert RX phase for MD-3x0 VHF and MD-UV3x0 radios.
    //
    const hwInfo_t* hwinfo = platform_getHwInfo();
    invertTxPhase = true;
    if(hwinfo->vhf_band == 1)
        invertRxPhase = true;
    else
        invertRxPhase = false;
    #elif defined(PLATFORM_MOD17)
    //
    // Get phase inversion settings from calibration.
    //
    invertTxPhase = (mod17CalData.bb_tx_invert == 1) ? true : false;
    invertRxPhase = (mod17CalData.bb_rx_invert == 1) ? true : false;
    #elif defined(PLATFORM_CS7000) || defined(PLATFORM_CS7000P)
    invertTxPhase = true;
    #elif defined(PLATFORM_DM1701)
    invertTxPhase = true;
    invertRxPhase = true;
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
            // check if we have SMS packet to send
            if(state.havePacketData)
                txPacketState(status);
            else
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

void OpMode_M17::offState(rtxStatus_t *const status)
{
    radio_disableRtx();

    codec_stop(txAudioPath);
    audioPath_release(txAudioPath);

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

    if(state.havePacketData)
    {
        startTx = true;
        status->opStatus = TX;
        status->txDisable = 0;
        return;
    }

    // Sleep for 30ms if there is nothing else to do in order to prevent the
    // rtx thread looping endlessly and locking up all the other tasks
    sleepFor(0, 30);
}

void OpMode_M17::rxState(rtxStatus_t *const status)
{
	uint8_t pthSts;

	if(startRx)
    {
        demodulator.startBasebandSampling();

        radio_enableRx();

        startRx = false;
    }

    bool newData = demodulator.update(invertRxPhase);
    bool lock    = demodulator.isLocked();

            // Reset frame decoder when transitioning from unlocked to locked state.
    if((lock == true) && (locked == false))
    {
        decoder.reset();
        locked = lock;
    }

    if(locked)
    {
        // Process new data
        if(newData)
        {
            auto& frame   = demodulator.getFrame();
            auto  type    = decoder.decodeFrame(frame);
            auto  lsf     = decoder.getLsf();
            status->lsfOk = lsf.valid();

            if(status->lsfOk)
            {
                dataValid = true;
                frameCnt++;

                        // Retrieve stream source and destination data
                std::string dst = lsf.getDestination();
                std::string src = lsf.getSource();

                        // Retrieve extended callsign data
                streamType_t streamType = lsf.getType();

                if((streamType.fields.encType    == M17_ENCRYPTION_NONE) &&
                    (streamType.fields.encSubType == M17_META_EXTD_CALLSIGN))
                {
                    meta_t& meta = lsf.metadata();
                    std::string exCall1 = decode_callsign(meta.extended_call_sign.call1);
                    std::string exCall2 = decode_callsign(meta.extended_call_sign.call2);

                            //
                            // The source callsign only contains the last link when
                            // receiving extended callsign data: in order to always store
                            // the true source of a transmission, we need to store the first
                            // extended callsign in M17_src.
                            //
                    strncpy(status->M17_src,  exCall1.c_str(), 10);
                    strncpy(status->M17_refl, exCall2.c_str(), 10);

                    extendedCall = true;
                    if(frameCnt == 6)
                        frameCnt = 0;

                            // no metatext present
                    memset(status->M17_Meta_Text, 0, 53);
                }
                // Check if metatext is present
                else if((streamType.fields.encType    == M17_ENCRYPTION_NONE) &&
                         (streamType.fields.encSubType == M17_META_TEXT) &&
                         lsf.valid() && frameCnt == 6)
                {
                    frameCnt = 0;
                    meta_t& meta = lsf.metadata();
                    uint8_t blk_len = (meta.raw_data[0] & 0xf0) >> 4;
                    uint8_t blk_id = (meta.raw_data[0] & 0x0f);
                    if(blk_id == 1)
                    {  // On first block reset everything
                        memset(status->M17_Meta_Text, 0, 53);
                        memset(textBuffer, 0, 53);
                        textOffset = 0;
                        blk_id_tot = 0;
                        textStarted = true;
                    }
                    // check if first valid metatext block is found
                    if(textStarted)
                    {
                        // Check for valid block id
                        if(blk_id <= 0x0f)
                        {
                            blk_id_tot += blk_id;
                            memcpy(textBuffer+textOffset, meta.raw_data+1, 13);
                            textOffset += 13;
                            // Check for completed text message
                            if((blk_len == blk_id_tot) || textOffset == 52)
                            {
                                memcpy(status->M17_Meta_Text, textBuffer, textOffset);
                                textOffset = 0;
                                blk_id_tot = 0;
                                textStarted = false;
                            }
                        }
                    }
                }

                        // Set source and destination fields.
                        // If we have received an extended callsign the src will be the RF link address
                        // The M17_src will already be stored from the extended callsign
                strncpy(status->M17_dst, dst.c_str(), 10);

                if(extendedCall)
                    strncpy(status->M17_link, src.c_str(), 10);
                else
                    strncpy(status->M17_src, src.c_str(), 10);

                        // Check CAN on RX, if enabled.
                        // If check is disabled, force match to true.
                bool canMatch =  (streamType.fields.CAN == status->can)
                                || (status->canRxEn == false);

                        // Check if the destination callsign of the incoming transmission
                        // matches with ours
                bool callMatch = compareCallsigns(std::string(status->source_address), dst);

                // Open audio path only if CAN and callsign match
                if((type == M17FrameType::STREAM) && (canMatch == true) && (callMatch == true))
                {
                    pthSts = audioPath_getStatus(rxAudioPath);
                	if(pthSts == PATH_CLOSED)
                	{
                	    rxAudioPath = audioPath_request(SOURCE_MCU, SINK_SPK, PRIO_RX);
                	    pthSts = audioPath_getStatus(rxAudioPath);
                	}

                	// Extract audio data and sent it to codec
                	if((type == M17FrameType::STREAM) && (pthSts == PATH_OPEN))
                	{
                		// (re)start codec2 module if not already up
                		if(codec_running() == false)
                			codec_startDecode(rxAudioPath);

                		M17StreamFrame sf = decoder.getStreamFrame();
                		codec_pushFrame(sf.payload().data(),     false);
                		codec_pushFrame(sf.payload().data() + 8, false);
                	}
                } // check if packet SMS message and SMS receive enabled
                else if(type == M17FrameType::PACKET && smsEnabled && (canMatch == true) &&
                		((callMatch == true) || !state.settings.m17_sms_match_call))
                {
                	// grab decoded packet data
                    M17PacketFrame pf = decoder.getPacketFrame();

                    if(!smsStarted && pf.payload()[0] == 0x05)
                    {  // check if we need to delete oldest message to make room
                    	if(state.totalSMSMessages == 0)
                    		lastCRC = 0;
                    	if(state.totalSMSMessages == 10)
                    	{
                    		free(smsSender[0]);
                    		free(smsMessage[0]);
                    		smsSender.erase(smsSender.begin());
                    		smsMessage.erase(smsMessage.begin());
                    		state.totalSMSMessages--;
                    	}
                		smsLastFrame = 0;
                		// start new message by saving senders call
                		char *tmp = (char*)malloc(strlen(status->M17_src)+1);
                		if(tmp != NULL)
                		{
                			memset(tmp, 0, strlen(status->M17_src)+1);
                			memcpy(tmp, status->M17_src, strlen(status->M17_src));
                			smsSender.push_back(tmp);
                			smsStarted = true;
                			totalSMSLength = 0;
                			memset(smsBuffer, 0, 821);
                		}
                    }

                	// store next frame of message
                	if(smsStarted)
                    {
                        uint8_t rx_fn   = (pf.payload()[25] >> 2) & 0x1F;
                    	uint8_t rx_last =  pf.payload()[25] >> 7;

                    	if(rx_fn <= 31 && rx_fn == smsLastFrame && !rx_last)
                    	{
                   		    memcpy(&smsBuffer[totalSMSLength], pf.payload().data(), 25);
                    		smsLastFrame++;
                    		totalSMSLength += 25;
                    	}
                    	else if(rx_last)
                    	{
                   		    memcpy(&smsBuffer[totalSMSLength], pf.payload().data(), rx_fn < 25 ? rx_fn : 25);
                    		totalSMSLength += rx_fn < 25 ? rx_fn : 25;

                    		// check crc matches
                			uint16_t packet_crc = lsf.m17Crc(smsBuffer, totalSMSLength - 2);
                			uint16_t crc;
                			memcpy((uint8_t*)&crc, &smsBuffer[totalSMSLength - 2], 2);
                			// store completed message into message queue
                			char *tmp = (char*)malloc(totalSMSLength-3);
                			if(tmp != NULL && crc == packet_crc && crc != lastCRC)
                			{
                				memset(tmp, 0, totalSMSLength-3);
                				memcpy(tmp, &smsBuffer[1], totalSMSLength-3);
                				smsMessage.push_back(tmp);
                        		state.totalSMSMessages++;
                        		lastCRC = crc;
                			}
                			else
                			{  // if message memory allocation fails, crc does not match
                			   // or duplicate message delete sender call
                				if(tmp != NULL)
                					free(tmp);
                				free(smsSender[state.totalSMSMessages]);
                				smsSender.pop_back();
                			}
                	        smsStarted = false;
                		}
                    } // if SMS started
                } // if type PACKET
            } // if LSF OK
        }
    }

    locked = lock;

    if(platform_getPttStatus() || state.havePacketData)
    {
        demodulator.stopBasebandSampling();
        locked = false;
        status->opStatus = OFF;
    }

            // Force invalidation of LSF data as soon as lock is lost (for whatever cause)
    if(locked == false)
    {
        status->lsfOk = false;
        dataValid     = false;
        extendedCall  = false;
        textStarted   = false;
        smsStarted    = false;
        memset(textBuffer, 0, 52);
        status->M17_link[0] = '\0';
        status->M17_refl[0] = '\0';
 //       if(codec_running() == true)
        	codec_stop(rxAudioPath);
  //      if(pthSts == PATH_OPEN)
            audioPath_release(rxAudioPath);
    }
}

void OpMode_M17::txState(rtxStatus_t *const status)
{
    static streamType_t type;
    frame_t m17Frame;
    static bool textStarted;
    static bool gpsStarted;

    if(startTx)
    {
        startTx = false;
        lsfFragCount = 6;
        gpsTimer = -1;
        textStarted = false;
        gpsStarted = false;

        // reset metatext so nothing left over from previous contact
        memset(status->M17_Meta_Text, 0, 53);

        std::string src(status->source_address);
        std::string dst(status->destination_address);

        lsf.clear();
        lsf.setSource(src);
        if(!dst.empty()) lsf.setDestination(dst);

        type.fields.dataMode   = M17_DATAMODE_STREAM;     // Stream
        type.fields.dataType   = M17_DATATYPE_VOICE;      // Voice data
        type.fields.CAN        = status->can;             // Channel access number
        if(strlen(state.settings.M17_meta_text) > 0)      // have text to send
        {
            type.fields.encType    = M17_ENCRYPTION_NONE; // No encryption
            type.fields.encSubType = M17_META_TEXT;       // Meta text

    		uint8_t buf[14];
    		memset(buf, 32, 14); // set to all spaces
    		// this should return number of meta blocks needed
    		uint8_t msglen = ceilf((float)strlen(state.settings.M17_meta_text) / 13.0f);
    		// set control byte upper nibble for number of text blocks
    		// 0001 = 1 blk. 0011 = 2 blks, 0111 = 3 blks, 1111 = 4 blks
    		buf[0] = (0x0f >> (4 - msglen)) << 4;

    		// check if less than 13 characters remain
    		uint8_t len = (uint8_t)(strlen(state.settings.M17_meta_text) - (last_text_blk * 13));
    		// if over 13 then limit to 13
    		if(len > 13)
    			len = 13;
    		memcpy(buf+1, state.settings.M17_meta_text+(last_text_blk * 13), len);

    		// set control byte lower nibble to block id
    		// 0001 = blk1, 0010 = blk2, 0100 = blk3, 1000 = blk4
    		buf[0] += (1 << last_text_blk++);
    		lsf.setMetaText(buf);
    		encoder.encodeLsf(lsf, m17Frame);

    		// if all blocks sent then reset
    		if(last_text_blk == msglen)
    		{
    			last_text_blk = 0;
    		}
        }
        else
        	last_text_blk = 0xff;   // no text to send

        lsf.setType(type);
        lsf.updateCrc();

        encoder.reset();
        encoder.encodeLsf(lsf, m17Frame);

        txAudioPath = audioPath_request(SOURCE_MIC, SINK_MCU, PRIO_TX);
        codec_startEncode(txAudioPath);
        radio_enableTx();

        modulator.invertPhase(invertTxPhase);
        modulator.start();
        modulator.sendPreamble();
        modulator.sendFrame(m17Frame);
    }

    if(lsfFragCount == 6)
    {
	   lsfFragCount = 0;
       textStarted = false;
       gpsStarted = false;

       // reset metadata if no text message
       if(last_text_blk == 0xff)
       {
           uint8_t buf[14];
	       memset(buf, 0, 14);
     	   lsf.setMetaText(buf);
    	   type.fields.encSubType = M17_META_TEXT;
    	   lsf.setType(type);
           lsf.updateCrc();
    	   encoder.encodeLsf(lsf, m17Frame);
       }

       // Wait at least 5 seconds between GPS transmissions
       if((gpsTimer == -1 || gpsTimer >= 150) && (last_text_blk == 0 || last_text_blk == 0xff))
       {
           gpsStarted = true;
           textStarted = true;
       }
    }
    else
    	lsfFragCount++;

    // 0xff indicates no text to send
    if(last_text_blk != 0xff && !gpsStarted && !textStarted) // send meta text
    {
        textStarted = true;

    	uint8_t buf[14];
    	memset(buf, 32, 14); // set to all spaces
    	// this should return number of meta blocks needed
    	uint8_t msglen = ceilf((float)strlen(state.settings.M17_meta_text) / 13.0f);
    	// set control byte upper nibble for number of text blocks
    	// 0001 = 1 blk. 0011 = 2 blks, 0111 = 3 blks, 1111 = 4 blks
    	buf[0] = (0x0f >> (4 - msglen)) << 4;

    	// check if less than 13 characters remain
    	uint8_t len = (uint8_t)(strlen(state.settings.M17_meta_text) - (last_text_blk * 13));
    	// if over 13 then limit to 13
    	if(len > 13)
    	    len = 13;
    	memcpy(buf+1, state.settings.M17_meta_text+(last_text_blk * 13), len);

    	// set control byte lower nibble to block id
    	// 0001 = blk1, 0010 = blk2, 0100 = blk3, 1000 = blk4
    	buf[0] += (1 << last_text_blk++);
    	lsf.setMetaText(buf);
    	type.fields.encSubType = M17_META_TEXT;
    	lsf.setType(type);
        lsf.updateCrc();
    	encoder.encodeLsf(lsf, m17Frame);

    	// if all blocks sent then reset
    	if(last_text_blk == msglen)
    	{
    		last_text_blk = 0;
    	}
    }

    if(gpsStarted)
    {
        gpsStarted = false;
        gpsTimer = 0;

        gps_t gps_data;
        pthread_mutex_lock(&state_mutex);
        gps_data = state.gps_data;
        pthread_mutex_unlock(&state_mutex);

        if(gps_data.fix_type > 0) //Valid GPS fix
        {
            platform_ledOn(YELLOW); // Blink LED yellow when sending GNSS
        	uint8_t gnss[14] = {0};

        	gnss[0] = (M17_GNSS_SOURCE_OPENRTX<<4) | M17_GNSS_STATION_HANDHELD; //OpenRTX source, portable station

            gnss[1] &= ~((uint8_t)0xF0); //zero out gnss data validity field

            gnss[1] &= ~((uint8_t)0x7<<1); //Radius = 0
            gnss[1] &= ~((uint8_t)0<<4); //Radius invalid

            gnss[1] |= ((uint16_t)gps_data.tmg_true>>8)&1; //Bearing
            gnss[2] = ((uint16_t)gps_data.tmg_true)&0xFF;

            int32_t lat_tmp, lon_tmp;
            rtx_to_q(&lat_tmp, &lon_tmp, gps_data.latitude, gps_data.longitude);
            for(uint8_t i=0; i<3; i++)
            {
                gnss[3+i] = *((uint8_t*)&lat_tmp+2-i);
                gnss[6+i] = *((uint8_t*)&lon_tmp+2-i);
            }
            gnss[1] |= (1<<7); //Lat/lon valid

            uint16_t alt = (uint16_t)1000 + gps_data.altitude*2;
			gnss[9] = alt>>8;
            gnss[10] = alt&0xFF;
			gnss[1] |= (1<<6); //Altitude valid

            uint16_t speed = (uint16_t)gps_data.speed*2;
			gnss[11] = speed>>4;
            gnss[12] = (speed&0xFF)<<4;
			gnss[1] |= (1<<5); //Speed and Bearing valid

            gnss[12] &= ~((uint8_t)0x0F);
            gnss[13] = 0;

        	lsf.setMetaText((uint8_t*)&gnss);

        	type.fields.encSubType = M17_META_GNSS;
        	lsf.setType(type);
        	lsf.updateCrc();
        	encoder.encodeLsf(lsf, m17Frame);
            platform_ledOn(RED);
        }
    }

    if(gpsEnabled)
    	gpsTimer++;

    payload_t dataFrame;
    bool      lastFrame = false;

    // Wait until there are 16 bytes of compressed speech, then send them
    codec_popFrame(dataFrame.data(),     true);
    codec_popFrame(dataFrame.data() + 8, true);

    if(platform_getPttStatus() == false)
    {
        lastFrame = true;
        startRx   = true;
        if(strlen(state.settings.M17_meta_text) > 0) //do we have text to send
            last_text_blk = 0;
        else
        	last_text_blk = 0xff;
        status->opStatus = OFF;
    }

    encoder.encodeStreamFrame(dataFrame, m17Frame, lastFrame);
    modulator.sendFrame(m17Frame);

    if(lastFrame)
    {
    	lastCRC = 0;
        encoder.encodeEotFrame(m17Frame);
        modulator.sendFrame(m17Frame);
        modulator.stop();
        gpsTimer = -1;
    }
}

void OpMode_M17::txPacketState(rtxStatus_t *const status)
{
	frame_t      m17Frame;
    pktPayload_t packetFrame;
    uint8_t      full_packet_data[33*25] = {0};

    if(!startRx && locked)
    {
        demodulator.stopBasebandSampling();
        locked = false;
        status->opStatus = OFF;
    }

    // do not transmit if sms message empty
    if(strlen(state.sms_message) == 0)
    {
    	// do not enter normal tx mode until de-key from SMS send
        if(platform_getPttStatus() == false)
        	state.havePacketData = false;
    	startRx = true;
        status->opStatus = OFF;
    	return;
    }
	startTx = false;

	std::string src(status->source_address);
	std::string dst(status->destination_address);

	lsf.clear();
	lsf.setSource(src);
	if(!dst.empty()) lsf.setDestination(dst);

//	strcpy(state.sms_message, "Hello this is OpenRTX.");
	memset(full_packet_data, 0, 33*25);
	full_packet_data[0] = 0x05;
	memcpy((char*)&full_packet_data[1], state.sms_message, strlen(state.sms_message));
	numPacketbytes                     = strlen(state.sms_message) + 2; //0x05 and 0x00
	uint16_t packet_crc                = lsf.m17Crc(full_packet_data, numPacketbytes);
	full_packet_data[numPacketbytes]   = packet_crc & 0xFF;
	full_packet_data[numPacketbytes+1] = packet_crc >> 8;
	numPacketbytes += 2; //count 2-byte CRC too

	streamType_t type;
	type.fields.dataMode   = M17_DATAMODE_PACKET;     // Packet
	type.fields.dataType   = 0;
	type.fields.CAN        = status->can;             // Channel access number

	lsf.setType(type);
	lsf.updateCrc();

	encoder.reset();
    encoder.encodeLsf(lsf, m17Frame);

	radio_enableTx();

	modulator.invertPhase(invertTxPhase);
	modulator.start();
	modulator.sendPreamble();
	modulator.sendFrame(m17Frame);

	uint8_t cnt = 0;
	while(numPacketbytes > 25)
	{
		memcpy(packetFrame.data(), &full_packet_data[cnt*25], 25);
		packetFrame[25] = cnt << 2;
		encoder.encodePacketFrame(packetFrame, m17Frame);
		modulator.sendFrame(m17Frame);
		cnt++;
		numPacketbytes -= 25;
	}

	memset(packetFrame.data(), 0, 26);
	memcpy(packetFrame.data(), &full_packet_data[cnt*25], numPacketbytes);
	packetFrame[25] = 0x80 | (numPacketbytes << 2);
	encoder.encodePacketFrame(packetFrame, m17Frame);
	modulator.sendFrame(m17Frame);

	encoder.encodeEotFrame(m17Frame);
	modulator.sendFrame(m17Frame);
	modulator.stop();

	startRx = true;
//	if(platform_getPttStatus() == false)
		state.havePacketData = false;
	memset(state.sms_message, 0, 821);
    lastCRC = 0;
    status->txDisable = 1;
    status->opStatus = OFF;
}

bool OpMode_M17::compareCallsigns(const std::string& localCs,
                                  const std::string& incomingCs)
{
    if((incomingCs == "ALL") || (incomingCs == "INFO") || (incomingCs == "ECHO"))
        return true;

    std::string truncatedLocal(localCs);
    std::string truncatedIncoming(incomingCs);

    int slashPos = localCs.find_first_of('/');
    if(slashPos <= 2)
        truncatedLocal = localCs.substr(slashPos + 1);

    slashPos = incomingCs.find_first_of('/');
    if(slashPos <= 2)
        truncatedIncoming = incomingCs.substr(slashPos + 1);

    if(truncatedLocal == truncatedIncoming)
        return true;
    else
    {
    	// Remove any appended characters from callsign
        int spacePos = truncatedLocal.find_first_of(' ');
        if(spacePos >= 4)
            truncatedLocal = truncatedLocal.substr(0, spacePos);

        spacePos = truncatedIncoming.find_first_of(' ');
        if(spacePos >= 4)
            truncatedIncoming = truncatedIncoming.substr(0, spacePos);

        if(truncatedLocal == truncatedIncoming)
            return true;
    }

    return false;
}

void OpMode_M17::rtx_to_q(int32_t* qlat, int32_t* qlon, int32_t lat, int32_t lon)
{
	if(qlat!=NULL && qlon!=NULL)
	{
		*qlat = lat / 10 - lat / 147 + lat / 105646;  // 90e6/(2^23-1) - 1/(1/10 - 1/147 + 1/105646)  = ~0
		*qlon = lon / 21 - lon / 985 - lon / 2237284; //180e6/(2^23-1) - 1/(1/21 - 1/985 - 1/2237284) = ~0
	}
}

void OpMode_M17::q_to_rtx(int32_t* lat, int32_t* lon, int32_t qlat, int32_t qlon)
{
	if(lat!=NULL && lon!=NULL)
	{
		*lat = qlat * 11 - qlat / 4 - qlat / 47 + qlat / 8777; // 90e6/(2^23-1) - (11 - 1/4 - 1/47 + 1/8777) = ~0
		*lon = qlon * 21 + qlon / 2 - qlon / 23 + qlon / 867;  //180e6/(2^23-1) - (21 + 1/2 - 1/23 + 1/867)  = ~0
	}
}

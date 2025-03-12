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
 *   (2025) Modified by KD0OSS for P25 on Module17/OpenRTX                 *
 ***************************************************************************/

#include <audio_stream.h>
#include <imbe_audio_codec.h>
#include <interfaces/delays.h>
#include <pthread.h>
#include <stdint.h>
#include <threads.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <dsp.h>
//#include <drivers/USART3_MOD17.h> // used for debugging
#include <cstdint>
#include <DSTAR/RingBuffer.h>
#include <imbe_vocoder_api.h>
#include <drivers/usb_vcom.h>


static pathId           audioPath;

static uint8_t          initCnt = 0;
static bool             running;

static bool             reqStop;
static pthread_t        codecThread;
static pthread_attr_t   codecAttr;
static pthread_mutex_t  init_mutex  = PTHREAD_MUTEX_INITIALIZER;
static int16_t          decodeTable[256];

#ifdef PLATFORM_MOD17
static const uint8_t micGainPre  = 4;
static const uint8_t micGainPost = 3;
#else
#ifndef PLATFORM_LINUX
static const uint8_t micGainPre  = 8;
static const uint8_t micGainPost = 4;
#endif
#endif

extern CRingBuffer<uint8_t> p25PacketBuffer;
imbe_vocoder vocoder;

static void alawDecode(void);

static void *encodeFunc(void *arg);
static void *decodeFunc(void *arg);
static bool startThread(const pathId path, void *(*func) (void *));
static void stopThread();


void imbe_init()
{
    pthread_mutex_lock(&init_mutex);
    initCnt += 1;
    pthread_mutex_unlock(&init_mutex);
    alawDecode();

    if(initCnt > 0)
        return;

    running = false;
}

void imbe_terminate()
{
    pthread_mutex_lock(&init_mutex);
    initCnt -= 1;
    pthread_mutex_unlock(&init_mutex);

    if(initCnt > 0)
        return;

    if(running)
        stopThread();
}

bool imbe_startEncode(const pathId path)
{
    return startThread(path, encodeFunc);
}

bool imbe_startDecode(const pathId path)
{
    return startThread(path, decodeFunc);
}

void imbe_stop(const pathId path)
{
    if(running == false)
        return;

    if(audioPath != path)
        return;

    stopThread();
}

bool imbe_running()
{
    return running;
}

void encodeIMBE(int16_t *pcm, uint8_t *imbe)
{
//	vocoder.encode_4400(pcm, imbe);
}

void decodeIMBE(int16_t *pcm, uint8_t *imbe)
{
	vocoder.decode_4400(pcm, imbe);
}

static unsigned char alawEncode(int16_t sample)
{
    uint16_t samp = (sample & 0xFFFF);
    int sign=(samp & 0x8000) >> 8;
    if (sign != 0)
    {
        samp=(int16_t)-samp;
        sign=0x80;
    }

    if (samp > 32635) samp = 32635;

    int exp=7;
    int expMask;
    for (expMask=0x4000;(samp & expMask)==0 && exp>0; exp--, expMask >>= 1);
    int mantis = (samp >> ((exp == 0) ? 4 : (exp + 3))) & 0x0f;
    unsigned char alaw = (unsigned char)(sign | exp << 4 | mantis);
    return (unsigned char)(alaw^0xD5);
} // end alawEncode

static void alawDecode(void)
{
    for (int i = 0; i < 256; i++)
    {
        int input = (i & 0xFF) ^ 85;
        int mantissa = (input & 15) << 4;
        int segment = (input & 112) >> 4;
        int value = mantissa + 8;
        if (segment >= 1) value += 256;
        if (segment > 1) value <<= (segment - 1);
        if ((input & 128) == 0) value = -value;
        decodeTable[i] = (int16_t)value;
    }
} // end alawDecode

static void *encodeFunc(void *arg)
{
    streamId        iStream;
    pathId          iPath = *((pathId*) arg);
    stream_sample_t audioBuf[320];

    iStream = audioStream_start(iPath, audioBuf, 320, 8000,
                                STREAM_INPUT | BUF_CIRC_DOUBLE);
    if(iStream < 0)
    {
        pthread_detach(pthread_self());
        running = false;
        return NULL;
    }

    uint8_t *cRxBuffer = (uint8_t*)malloc(166 * sizeof(uint8_t));

    int byte_count = 0;

    p25PacketBuffer.reset();

    while(reqStop == false)
    {
        // Invalid path, quit
        if(audioPath_getStatus(iPath) != PATH_OPEN)
            break;

        dataBlock_t audio = inputStream_getData(iStream);
        if(audio.data == NULL)
            break;

#ifndef PLATFORM_LINUX
        // Pre-amplification stage
        for(size_t i = 0; i < audio.len; i++) audio.data[i] *= micGainPre;

        if(cRxBuffer != NULL)
        {
        	// Six header bytes
            cRxBuffer[byte_count++] = 0x61;
            cRxBuffer[byte_count++] = 0x00;
            cRxBuffer[byte_count++] = 0xA2;
            cRxBuffer[byte_count++] = 0x04; // Type = audio for imbeEncoder
            cRxBuffer[byte_count++] = 0x00;
            cRxBuffer[byte_count++] = 0xA0;

            // Post-amplification stage
            for(size_t i = 0; i < audio.len; i++)
            {
            	cRxBuffer[byte_count++] = alawEncode(audio.data[i] * micGainPost);
            }
            vcom_writeBlock(cRxBuffer, 166);
            byte_count = 0;
        }
/*
        if(cRxBuffer != NULL && p25PacketBuffer.getSpace() >= 11)
        {
            // Post-amplification stage
            for(size_t i = 0; i < audio.len; i++)
            {
            	cRxBuffer[byte_count++] = audio.data[i] * micGainPost;
            	if (byte_count == 160)
            	{
            		encodeIMBE(cRxBuffer, imbe);
            		for (size_t j=0; j < 11; j++)
            			p25PacketBuffer.put(imbe[j]);
            		byte_count = 0;
            	}
            }
            byte_count = 0;
        }
*/
#endif
    }

    if(cRxBuffer != NULL) free(cRxBuffer);

    audioStream_terminate(iStream);

    // In case thread terminates due to invalid path or stream error, detach it
    // to ensure that its memory gets freed by the OS.
    if(reqStop == false)
        pthread_detach(pthread_self());

    running = false;
    return NULL;
}

static void *decodeFunc(void *arg)
{
    streamId        oStream;
    pathId          oPath = *((pathId*) arg);
    stream_sample_t audioBuf[320];

    // Open output stream
    memset(audioBuf, 0x00, 320 * sizeof(stream_sample_t));
    oStream = audioStream_start(oPath, audioBuf, 320, 8000,
                                STREAM_OUTPUT | BUF_CIRC_DOUBLE);

    if(oStream < 0)
    {
    	pthread_detach(pthread_self());
    	running = false;
    	return NULL;
    }

 //   uint8_t   *cRxBuffer = (uint8_t*)malloc(166);
 //   uint8_t  *siAudioBuf = (uint8_t*)malloc(320);
    uint8_t siAudioBuf[320];
 //   if(siAudioBuf != NULL/* && cRxBuffer != NULL*/)
    {
    	// Ensure that thread start is correctly synchronized with the output
    	// stream to avoid having the decode function writing in a memory area
    	// being read at the same time by the output stream system causing cracking
    	// noises at speaker output. Behavior observed on both Module17
    	outputStream_sync(oStream, false);
    	int frame = -1;

    	while(reqStop == false)
    	{
    		// Invalid path, quit
    		if(audioPath_getStatus(oPath) != PATH_OPEN)
    			break;

    		bool newData = false;
    		stream_sample_t *audioBuf = outputStream_getIdleBuffer(oStream);
/*            int byte_count = 0;
            if (vcom_bytesReady() >= 1)
            {
            	// look for start of audio packet
            	vcom_readBlock((uint8_t*)cRxBuffer, 1);
            	if (cRxBuffer[0] != 0x61)
            		continue;
            }
    		// Read next 165 bytes of PCM audio from host
    		if (vcom_bytesReady() >= 165)
    		{
    			byte_count = vcom_readBlock((uint8_t*)cRxBuffer, 165);
    			if(byte_count == 165)
    			{
    				for(int i = 0;i < byte_count;i++)
    				{
    					if(frame >= 0)
    					{
       						int16_t audio = decodeTable[cRxBuffer[i]];
       	    				audio *= 2.0F;
    						siAudioBuf[frame++] = audio & 0x00ff;
    						siAudioBuf[frame++] = (audio >> 8) & 0xff;
    				//		audioBuf[frame++] = audio & 0x00ff;
    				//		audioBuf[frame++] = (audio >> 8) & 0xff;
    					}

    					// Check for header bytes
    					if(cRxBuffer[2] == 0x02 && frame < 0)
    					{
    						frame = 0; // start of PCM audio packet
    						i = i + 4; // advance past second header byte
    					}

    					if(frame == 320)
    					{
    						memcpy(audioBuf, (uint8_t*)siAudioBuf, 320);
    						newData = true;
    						frame = -1;
    					}
    				}
    			}
    			memset(cRxBuffer, 0x00, 166);
    		}
*/

    	    int16_t pcm[160];
    	    uint8_t imbe[11] = {0x04U, 0x0CU, 0xFDU, 0x7BU, 0xFBU, 0x7DU, 0xF2U, 0x7BU, 0x3DU, 0x9EU, 0x45U}; // silence

    		if (p25PacketBuffer.getData() >= 11)
    		{
    			for (int i=0;i<11;i++)
    				p25PacketBuffer.get(imbe[i]);
    			decodeIMBE(pcm, imbe);
    			for (int i=0;i<160;i++)
    			{
    				pcm[i] *= 1.5F;
				    siAudioBuf[frame++] = pcm[i] & 0x00ff;
				    siAudioBuf[frame++] = (pcm[i] >> 8) & 0xff;
    			}
				memcpy(audioBuf, (uint8_t*)siAudioBuf, 320);
    			newData = true;
    			frame = 0;
    		}

    		if(audioBuf == NULL)
    			break;

    		if(!newData)
    		{
    			memset(audioBuf, 0x00, 160 * sizeof(stream_sample_t));
    		}

    		outputStream_sync(oStream, true);
    	}

    	// Stop stream and wait until its effective termination
    	audioStream_stop(oStream);

   // 	while (vcom_readBlock((uint8_t*)cRxBuffer, 166) > 0);

   // 	if(siAudioBuf != NULL) free(siAudioBuf);
   // 	if(cRxBuffer != NULL) free(cRxBuffer);
    }

    // In case thread terminates due to invalid path or stream error, detach it
    // to ensure that its memory gets freed by the OS.
    if(reqStop == false)
        pthread_detach(pthread_self());

    running = false;
    return NULL;
}


static bool startThread(const pathId path, void *(*func) (void *))
{
    // Bad incoming path
    if(audioPath_getStatus(path) != PATH_OPEN)
        return false;

    // Handle access contention when starting the codec thread to ensure that
    // only one call at a time can effectively start the thread.
    pthread_mutex_lock(&init_mutex);
    if(running)
    {
        // Same path as before, path open, codec already running: all good.
        if(path == audioPath)
        {
            pthread_mutex_unlock(&init_mutex);
            return true;
        }

        // New path takes over the current one only if it has an higher priority
        // or the current one is closed/suspended.
        pathInfo_t newPath = audioPath_getInfo(path);
        pathInfo_t curPath = audioPath_getInfo(audioPath);
        if((curPath.status == PATH_OPEN) && (curPath.prio >= newPath.prio))
        {
            pthread_mutex_unlock(&init_mutex);
            return false;
        }
        else
        {
            stopThread();
        }
    }

    running   = true;
    audioPath = path;
    pthread_mutex_unlock(&init_mutex);

    reqStop     = false;

    pthread_attr_init(&codecAttr);

    #if defined(_MIOSIX)
    // Set stack size of IMBE thread to 16kB.
    pthread_attr_setstacksize(&codecAttr, IMBE_THREAD_STKSIZE);

    // Set priority of IMBE thread to the maximum one, the same of RTX thread.
    struct sched_param param;
    param.sched_priority = THREAD_PRIO_HIGH;
    pthread_attr_setschedparam(&codecAttr, &param);
    #elif defined(__ZEPHYR__)
    // Allocate and set the stack for AMBE thread
    void *ambe_thread_stack = malloc(IMBE_THREAD_STKSIZE * sizeof(uint8_t));
    pthread_attr_setstack(&codecAttr, imbe_thread_stack, IMBE_THREAD_STKSIZE);
    #endif

    // Start thread
    int ret = pthread_create(&codecThread, &codecAttr, func, &audioPath);
    if(ret < 0)
        running = false;

    return running;
}

static void stopThread()
{
    reqStop = true;
    pthread_join(codecThread, NULL);
    running = false;

    #ifdef __ZEPHYR__
    void  *addr;
    size_t size;

    pthread_attr_getstack(&codecAttr, &addr, &size);
    free(addr);
    #endif
}

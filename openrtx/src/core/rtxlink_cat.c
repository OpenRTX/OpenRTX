/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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
 ***************************************************************************/

#define __LINUX_ERRNO_EXTENSIONS__

#include <stdio.h>
#include <arpa/inet.h>

#include <interfaces/platform.h>
#include <interfaces/radio.h>
#include <rtxlink_cat.h>
#include <stdbool.h>
#include <rtxlink.h>
#include <string.h>
#include <errno.h>
#include <state.h>
#include <rtx.h>
#include <utils.h>

enum catFrameType
{
    CAT_FRAME_GET  = 0x47,
    CAT_FRAME_SET  = 0x53,
    CAT_FRAME_PEEK = 0x50,
    CAT_FRAME_ACK  = 0x41,
    CAT_FRAME_DATA = 0x44,
};

extern pthread_mutex_t rtx_mutex;

static size_t catCommandGet(const uint8_t *args, const size_t len,
                            uint8_t *reply)
{
    (void) len;

    uint16_t id = (args[0] << 8) | args[1];
    size_t  ret = 1;
    reply[0]    = CAT_FRAME_DATA;

    switch(id)
    {
        case 0x494E:    // Device ID
        {
            const hwInfo_t *hwinfo = platform_getHwInfo();
            size_t sl = strlen(hwinfo->name);
            if(sl > 16) sl = 16;
            memset(&reply[1], 0x00, 16);
            memcpy(&reply[1], hwinfo->name, sl);
            ret += sl;
        }
            break;

        case 0x5246:    // Receive frequency
        {
            rtxStatus_t status = rtx_getCurrentStatus();
            memcpy(&reply[1], &status.rxFrequency, 4);
            ret += 4;
        }
            break;

        case 0x5446:    // Transmit frequency
        {
            rtxStatus_t status = rtx_getCurrentStatus();
            memcpy(&reply[1], &status.txFrequency, 4);
            ret += 4;
        }
            break;

        default:
            reply[0] = CAT_FRAME_ACK;
            reply[1] = EBADRQC;
            ret = 2;
            break;
    }

    return ret;
}

static void catConfigureRtx(void)
{
    rtxStatus_t rtx_cfg;

    /*
     * This is supposed to change the configuration, but it does not,
     * subsequent calls to get the frequency return 0 or -1.
     */
    pthread_mutex_lock(&rtx_mutex);
    rtx_cfg.opMode      = state.channel.mode;
    rtx_cfg.bandwidth   = state.channel.bandwidth;
    rtx_cfg.rxFrequency = state.channel.rx_frequency;
    rtx_cfg.txFrequency = state.channel.tx_frequency;
    rtx_cfg.txPower     = dBmToWatt(state.channel.power);
    rtx_cfg.sqlLevel    = state.settings.sqlLevel;
    rtx_cfg.rxToneEn    = state.channel.fm.rxToneEn;
    rtx_cfg.rxTone      = ctcss_tone[state.channel.fm.rxTone];
    rtx_cfg.txToneEn    = state.channel.fm.txToneEn;
    rtx_cfg.txTone      = ctcss_tone[state.channel.fm.txTone];
    rtx_cfg.toneEn      = state.tone_enabled;

    // Enable Tx if channel allows it and we are in UI main screen
    rtx_cfg.txDisable = state.channel.rx_only || state.txDisable;

    // Copy new M17 CAN, source and destination addresses
    rtx_cfg.can = state.settings.m17_can;
    rtx_cfg.canRxEn = state.settings.m17_can_rx;
    strncpy(rtx_cfg.source_address,      state.settings.callsign, 10);
    strncpy(rtx_cfg.destination_address, state.settings.m17_dest, 10);
    pthread_mutex_unlock(&rtx_mutex);
    rtx_configure(&rtx_cfg);
}

static size_t catCommandSet(const uint8_t *args, const size_t len,
                            uint8_t *reply)
{
    (void) len;

    uint16_t id = (args[0] << 8) | args[1];
    reply[0] = CAT_FRAME_ACK;
    reply[1] = 0;

    switch(id)
    {
        case 0x5246:    // Receive frequency

            pthread_mutex_lock(&state_mutex);
            state.channel.rx_frequency = *(uint32_t *)&args[2];
            pthread_mutex_unlock(&state_mutex);
            catConfigureRtx();
            break;

        case 0x5446:    // Transmit frequency

            pthread_mutex_lock(&state_mutex);
            state.channel.tx_frequency = *(uint32_t *)&args[2];
            pthread_mutex_unlock(&state_mutex);
            catConfigureRtx();
            break;

        case 0x5043:    // Power cycle

            // TODO: to be implemented
            reply[1] = ENOTSUP;
            break;

        case 0x4252:    // Baud rate
        {
            uint32_t baud = *(uint32_t *)(args + 2);

            // ioctl returns a negative value in case of errors: we change its
            // sign and cast it to a uint8_t before sending it out.
            int ret = chardev_ioctl(RTXLINK_DEV, IOCTL_SETSPEED, &baud);
            if(ret < 0)
                reply[1] = (uint8_t)(-ret);
        }
            break;

        case 0x4654:    // File transfer mode

            pthread_mutex_lock(&state_mutex);
            state.devStatus = DATATRANSFER;
            pthread_mutex_unlock(&state_mutex);
            break;

        default:
            reply[1] = EBADRQC;
            break;
    }

    return 2;
}

static size_t catProtocolHandler(const uint8_t *rxData, size_t rxLen,
                                 uint8_t *txData, size_t txMaxLen)
{
    (void) txMaxLen;
    size_t rLen = 0;

    // A CAT command frame must contain at least one command byte and two bytes
    // of entity ID.
    if(rxLen < 3)
    {
        txData[0] = CAT_FRAME_ACK;
        txData[1] = EPROTO;

        return 2;
    }

    // Separate command and arguments, adjust the data length accordingly
    const uint8_t  cmd  = rxData[0];
    const uint8_t *args = rxData + 1;
    rxLen -= 1;

    // Set the default response as an ACK response with length 2 to avoid
    // replicating the same code when handling errors.
    txData[0] = CAT_FRAME_ACK;
    rLen = 2;

    switch(cmd)
    {
        case CAT_FRAME_GET:
            rLen = catCommandGet(args, rxLen, txData);
            break;

        case CAT_FRAME_SET:
            rLen = catCommandSet(args, rxLen, txData);
            break;

        case CAT_FRAME_PEEK:
        {
            // Arguments must be one byte for the length plus the size of the
            // memory address
            if(rxLen != (sizeof(void *) + 1))
            {
                txData[1] = EPROTO;
                break;
            }

            // Size of transfer
            size_t dlen = args[0];
            if(dlen > 31)
            {
                txData[1] = ENOBUFS;
                break;
            }

            // Set up and fill reply frame
            txData[0] = CAT_FRAME_DATA;
            rLen      = dlen + 1;

            void  *ptr;
            memcpy(&ptr, args + 1, sizeof(void *));
            memcpy(&txData[1], ptr, dlen);
        }
            break;

        default:
            txData[1] = EPERM;
            break;
    }

    return rLen;
}


void cat_init()
{
    rtxlink_setProtocolHandler(RTXLINK_FRAME_CAT, catProtocolHandler);
}

void cat_terminate()
{
    rtxlink_removeProtocolHandler(RTXLINK_FRAME_CAT);
}

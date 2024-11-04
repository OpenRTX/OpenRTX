/***************************************************************************
 *   Copyright (C) 2023, 2024 by Federico Amedeo Izzo IU2NUO,              *
 *                               Niccolò Izzo IU2KIN                       *
 *                               Frederik Saraci IU2NRO                    *
 *                               Silvano Seva IU2KWO                       *
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

#include <sys/param.h>

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

enum catCommand
{
    // CAT command to get/set resources
    CAT_DEVICE_ID    = 0x494E,
    CAT_RX_FREQUENCY = 0x5246,
    CAT_TX_FREQUENCY = 0x5446,
    CAT_OP_MODE      = 0x4F4D,
    CAT_M17_CALLSIGN = 0x4D43,
    CAT_M17_DEST     = 0x4D44,
    CAT_M17_CAN      = 0x4341,
    CAT_PTT          = 0x5054,

    // Miscellaneous CAT command
    CAT_POWER_CYCLE  = 0x5043,
    CAT_BAUD_RATE    = 0x4252,
    CAT_DATATRANSFER = 0x4654
};

enum catOpMode
{
    CAT_OP_MODE_NONE = 0x00,    // No op mode selected
    CAT_OP_MODE_FM   = 0x01,    // FM with 25.0 kHz bandwidth
    CAT_OP_MODE_FM20 = 0x02,    // FM with 20.0 kHz bandwidth
    CAT_OP_MODE_NFM  = 0x03,    // Narrowband FM with 12.5 kHz bandwidth
    CAT_OP_MODE_DMR  = 0x04,    // Digital mobile radio
    CAT_OP_MODE_M17  = 0x05     // M17
};

extern pthread_mutex_t rtx_mutex;

static size_t catCommandGet(const uint8_t *args, const size_t len,
                            uint8_t *reply)
{
    (void) len;

    uint16_t id = (args[0] << 8) | args[1];
    size_t  ret = 1;
    reply[0]    = CAT_FRAME_DATA;
    rtxStatus_t status;

    switch(id)
    {
        case CAT_DEVICE_ID:
        {
            const hwInfo_t *hwinfo = platform_getHwInfo();
            size_t sl = strlen(hwinfo->name);
            if(sl > 16) sl = 16;
            memset(&reply[1], 0x00, 16);
            memcpy(&reply[1], hwinfo->name, sl);
            ret += sl;
        }
            break;

        case CAT_RX_FREQUENCY:
            status = rtx_getCurrentStatus();
            memcpy(&reply[1], &status.rxFrequency, 4);
            ret += 4;
            break;

        case CAT_TX_FREQUENCY:
            status = rtx_getCurrentStatus();
            memcpy(&reply[1], &status.txFrequency, 4);
            ret += 4;
            break;

        case CAT_OP_MODE:

            status = rtx_getCurrentStatus();
            switch (status.opMode)
            {
                case OPMODE_FM:
                    switch (status.bandwidth)
                    {
                        case BW_12_5:
                            reply[1] = CAT_OP_MODE_NFM;
                            break;
                        case BW_20:
                            reply[1] = CAT_OP_MODE_FM20;
                            break;
                        case BW_25:
                            reply[1] = CAT_OP_MODE_FM;
                            break;
                    }
                    break;
                case OPMODE_DMR:
                    reply[1] = CAT_OP_MODE_DMR;
                    break;
                case OPMODE_M17:
                    reply[1] = CAT_OP_MODE_M17;
                    break;
                default:
                    reply[1] = CAT_OP_MODE_NONE;
            }
            ret += 1;
            break;

        case CAT_PTT:

            status = rtx_getCurrentStatus();
            reply[1] = status.opStatus;
            ret += 1;
            break;

        case CAT_M17_CALLSIGN:

            status = rtx_getCurrentStatus();
            memset(&reply[1], 0x00, 16);
            memcpy(&reply[1], status.source_address, sizeof(status.source_address));
            ret += sizeof(status.source_address);
            break;

        case CAT_M17_DEST:

            status = rtx_getCurrentStatus();
            memset(&reply[1], 0x00, 16);
            memcpy(&reply[1], status.destination_address, sizeof(status.destination_address));
            ret += sizeof(status.M17_dst);
            break;

        case CAT_M17_CAN:

            status = rtx_getCurrentStatus();
            reply[1] = status.can;
            ret += 1;
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

    pthread_mutex_lock(&rtx_mutex);
    rtx_cfg.ptt         = state.ptt;
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
    strncpy(rtx_cfg.source_address, state.settings.callsign,
        sizeof(rtx_cfg.source_address));
    strncpy(rtx_cfg.destination_address, state.settings.m17_dest,
        sizeof(rtx_cfg.destination_address));
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
        case CAT_RX_FREQUENCY:

            pthread_mutex_lock(&state_mutex);
            state.channel.rx_frequency = *(uint32_t *)&args[2];
            pthread_mutex_unlock(&state_mutex);
            catConfigureRtx();
            break;

        case CAT_TX_FREQUENCY:

            pthread_mutex_lock(&state_mutex);
            state.channel.tx_frequency = *(uint32_t *)&args[2];
            pthread_mutex_unlock(&state_mutex);
            catConfigureRtx();
            break;

        case CAT_OP_MODE:

            pthread_mutex_lock(&state_mutex);
            switch (args[2])
            {
                case CAT_OP_MODE_FM:
                    state.channel.mode = OPMODE_FM;
                    state.channel.bandwidth = BW_25;
                    break;
                case CAT_OP_MODE_FM20:
                    state.channel.mode = OPMODE_FM;
                    state.channel.bandwidth = BW_20;
                    break;
                case CAT_OP_MODE_NFM:
                    state.channel.mode = OPMODE_FM;
                    state.channel.bandwidth = BW_12_5;
                    break;
                case CAT_OP_MODE_DMR:
                    state.channel.mode = OPMODE_DMR;
                    break;
                case CAT_OP_MODE_M17:
                    state.channel.mode = OPMODE_M17;
                    break;
                default:
                    state.channel.mode = OPMODE_NONE;
            }
            pthread_mutex_unlock(&state_mutex);
            catConfigureRtx();
            break;

        case CAT_PTT:

            if (args[2] <= RTX_TX)
            {
                pthread_mutex_lock(&state_mutex);
                state.ptt = args[2] == TX ? 1 : 0;
                pthread_mutex_unlock(&state_mutex);
                catConfigureRtx();
            }
            break;

        case CAT_M17_CALLSIGN:

            pthread_mutex_lock(&state_mutex);
            strncpy(state.settings.callsign, (const char *)&args[2],
                MIN(strlen((const char *)&args[2]),
                sizeof(state.settings.callsign)));
            pthread_mutex_unlock(&state_mutex);
            catConfigureRtx();
            break;

        case CAT_M17_DEST:

            pthread_mutex_lock(&state_mutex);
            strncpy(state.settings.m17_dest, (const char *)&args[2],
                MIN(strlen((const char *)&args[2]),
                sizeof(state.settings.m17_dest)));
            pthread_mutex_unlock(&state_mutex);
            catConfigureRtx();
            break;

        case CAT_M17_CAN:

            if (args[2] <= 15)
            {
                pthread_mutex_lock(&state_mutex);
                state.settings.m17_can = args[2];
                pthread_mutex_unlock(&state_mutex);
                catConfigureRtx();
            }
            break;

        case CAT_POWER_CYCLE:

            // TODO: to be implemented
            reply[1] = ENOTSUP;
            break;

        case CAT_BAUD_RATE:
        {
            uint32_t baud = *(uint32_t *)(args + 2);

            // ioctl returns a negative value in case of errors: we change its
            // sign and cast it to a uint8_t before sending it out.
            int ret = chardev_ioctl(RTXLINK_DEV, IOCTL_SETSPEED, &baud);
            if(ret < 0)
                reply[1] = (uint8_t)(-ret);
        }
            break;

        case CAT_DATATRANSFER:

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

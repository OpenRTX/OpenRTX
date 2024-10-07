/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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
 ***************************************************************************/

#include <interfaces/cps_io.h>
#include <interfaces/delays.h>
#include <string.h>
#include <wchar.h>
#include <utils.h>
#include "cps_data_A36plus.h"
#include "W25Qx.h"

static const uint32_t chDataBaseAddr  = 0x0;  /**< Base address of channel data         */
static const uint32_t maxNumChannels  = 512;     /**< Maximum number of channels in memory */


/**
 * This function does not apply to address-based codeplugs
 */
int cps_open(char *cps_name)
{

    (void) cps_name;
    return 0;
}

/**
 * This function does not apply to address-based codeplugs
 */
void cps_close()
{
}

/**
 * This function does not apply to address-based codeplugs
 */
int cps_create(char *cps_name)
{
    (void) cps_name;
    return 0;
}

int cps_readChannel(channel_t *channel, uint16_t pos)
{
    if(pos > maxNumChannels) return -1;

    memset(channel, 0x00, sizeof(channel_t));

    W25Qx_wakeup();
    delayUs(5);

    a36plusChannel_t chData;
    uint32_t readAddr = chDataBaseAddr + pos * sizeof(a36plusChannel_t);
    W25Qx_readData(readAddr, ((uint8_t *) &chData), sizeof(a36plusChannel_t));
    W25Qx_sleep();

    // Check if channel is empty
    if(chData.rx_frequency == 0xFFFFFFFF)
    {
        return -1;
    }

    channel->mode            = OPMODE_FM;
    channel->bandwidth       = !chData.bandwidth;            // inverted because 0x00 = 25 kHz, 0x01 = 12.5 kHz
    channel->power           = chData.power == 0x00 ? 10000 : chData.power == 0x01 ? 1000 : 5000; // 0x00 = high, 0x01 = low, 0x02 = medium
    channel->rx_frequency    = bcdToBin(chData.rx_frequency) * 10;
    channel->tx_frequency    = bcdToBin(chData.tx_frequency) * 10;
    channel->rx_only         = channel->tx_frequency < 136000000 ? 1 : 0;
    channel->scanList_index  = 0;
    channel->groupList_index = 0;

    for(uint16_t i = 0; i < 12; i++)
    {
        // If we encounter 0xFF, break the loop
        if(chData.name[i] == 0xFF)
        {
            channel->name[i] = '\0';
            break;
        }
        channel->name[i] = chData.name[i];
    }
    channel->name[12] = '\0'; // necessary, as the name is sometimes not null-terminated in the original data

    /* Load mode-specific parameters */
    if(channel->mode == OPMODE_FM)
    {
        channel->fm.txToneEn = chData.ctcss_transmit ? 1 : 0;
        channel->fm.rxToneEn = chData.ctcss_receive ? 1 : 0;
        uint16_t rx_css = chData.ctcss_receive;
        uint16_t tx_css = chData.ctcss_transmit;

        // TODO: Implement binary search to speed up this lookup
        if((rx_css != 0) && (rx_css != 0xFFFF))
        {
            for(int i = 0; i < MAX_TONE_INDEX; i++)
            {
                if(ctcss_tone[i] == (rx_css))
                {
                    channel->fm.rxTone = i;
                    channel->fm.rxToneEn = 1;
                    break;
                }
            }
        }

        if((tx_css != 0) && (tx_css != 0xFFFF))
        {
            for(int i = 0; i < MAX_TONE_INDEX; i++)
            {
                if(ctcss_tone[i] == (tx_css))
                {
                    channel->fm.txTone = i;
                    channel->fm.txToneEn = 1;
                    break;
                }
            }
        }

        // TODO: Implement warning screen if tone was not found
    }

    return 0;
}


int cps_readBankHeader(bankHdr_t *b_header, uint16_t pos)
{
    return 0;
}

int cps_readBankData(uint16_t bank_pos, uint16_t ch_pos)
{
    return 0;
}

int cps_readContact(contact_t *contact, uint16_t pos)
{
    return 0;
}

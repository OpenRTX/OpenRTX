/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/tuner.h"
#include "core/cps.h"

#include "interfaces/cps_io.h"
#include "rtx/rtx.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

struct tuner tuner_getDefault(void)
{
    struct tuner t = {
        .core = {
            .tuner_mode = TUNER_VFO,
            .power = 1000, // 1W
            .bank_index = 0,
            .channel_index = 0,
            .vfo = {
                #ifdef PLATFORM_MOD17
                .op_mode = OPMODE_M17,
                #else
                .op_mode = OPMODE_FM,
                #endif
                .m17 = {
                    .can = 0,
                    .encr = PLAIN,
                    .gps_mode = NO_GPS,
                    .mode = DIGITAL_VOICE,
                    .dest = "ALL",
                },
                #ifndef PLATFORM_MOD17
                .fm = {
                    .rxToneEn = 0,
                    .rxTone = 0,
                    .txToneEn = 0,
                    .txTone = 0,
                    .sqlLevel = 4,  // S3
                },
                .rx_frequency = 433500000,
                .tx_frequency = 433500000
                #else
                .rx_frequency = 433475000,
                .tx_frequency = 433475000
                #endif
            },
        },
    };

    return t;
}

enum tuner_mode tuner_get_mode(const struct tuner *t)
{
    if(t == NULL)
        return TUNER_VFO;
    return t->core.tuner_mode;
}

uint32_t tuner_get_tx_frequency(const struct tuner *t)
{
    if(t == NULL)
        return 0;

    if(t->core.tuner_mode == TUNER_VFO)
        return t->core.vfo.tx_frequency;

    return t->cache.channel.tx_frequency;
}

uint32_t tuner_get_rx_frequency(const struct tuner *t)
{
    if(t == NULL)
        return 0;

    if(t->core.tuner_mode == TUNER_VFO)
        return t->core.vfo.rx_frequency;

    return t->cache.channel.rx_frequency;
}

uint32_t tuner_get_power(const struct tuner *t)
{
    if(t == NULL)
        return 0;

    return t->core.power;
}

uint8_t tuner_get_opmode(const struct tuner *t)
{
    if(t == NULL)
        return 0;

    if(t->core.tuner_mode == TUNER_VFO)
        return t->core.vfo.op_mode;

    return t->cache.channel.mode;
}

int tuner_get_M17_params(const struct tuner *t, m17Info_t *params)
{
    if( (t == NULL) || (params == NULL))
        return -EINVAL;

    if(t->core.tuner_mode == TUNER_VFO)
        *params = t->core.vfo.m17;
    else
    {
        if(t->cache.channel.mode == OPMODE_M17)
            *params = t->cache.channel.m17;
        else
            return -EINVAL;
    }

    return 0;
}

int tuner_get_FM_params(const struct tuner *t, fmInfo_t *params)
{
    if( (t == NULL) || (params == NULL))
        return -EINVAL;

    if(t->core.tuner_mode == TUNER_VFO)
        *params = t->core.vfo.fm;
    else
    {
        if(t->cache.channel.mode == OPMODE_FM)
            *params = t->cache.channel.fm;
        else
            return -EINVAL;
    }

    return 0;
}

int tuner_get_DMR_params(const struct tuner *t, dmrInfo_t *params)
{
    if( (t == NULL) || (params == NULL))
        return -EINVAL;

    if(t->core.tuner_mode == TUNER_VFO)
        *params = t->core.vfo.dmr;
    else
    {
        if(t->cache.channel.mode == OPMODE_DMR)
            *params = t->cache.channel.dmr;
        else
            return -EINVAL;
    }

    return 0;
}

void tuner_get_channel_name(const struct tuner *t, char *s, const size_t n)
{
    if(n == 0 || t == NULL || s == NULL)
        return;

    if(t->core.tuner_mode == TUNER_VFO)
        s[0] = '\0';
    else
        strncpy(s, t->cache.channel.name, n);
}

const channel_t *tuner_get_channel(const struct tuner *t)
{
    if(t == NULL || t->core.tuner_mode == TUNER_VFO)
        return NULL;

    return &t->cache.channel;
}

enum bandwidth tuner_get_bandwidth(const struct tuner *t)
{
    if(t == NULL)
        return BW_12_5;

    if(tuner_get_mode(t) == TUNER_VFO)
    {
        switch(tuner_get_opmode(t))
        {
            case OPMODE_FM:
                return t->core.vfo.bandwidth;
            case OPMODE_DMR:
            case OPMODE_M17:
            default:
                return BW_12_5;
        }
    }

    return t->cache.channel.bandwidth;
}

int tuner_switch_mode(struct tuner *t, enum tuner_mode mode)
{
    switch(mode)
    {
        case TUNER_VFO:
            t->core.tuner_mode = TUNER_VFO;
            break;
        case TUNER_CHAN:
            return tuner_set_mode_channel(t, t->core.channel_index);
            break;
        case TUNER_BANK:
            return tuner_set_mode_bank(t, t->core.bank_index, t->core.channel_index);
            break;
    }
    return 0;
}

int tuner_set_mode_bank(struct tuner *t, const uint16_t bank, const uint16_t channel)
{
    if(t == NULL)
        return -EINVAL;

    t->core.tuner_mode = TUNER_BANK;
    t->core.bank_index = bank;
    t->core.channel_index = channel;

    cps_readBankHeader(&(t->cache.bank), t->core.bank_index);
    if(t->core.bank_index > t->cache.bank.ch_count)
        return -EINVAL;
    int ch = cps_readBankData(bank, channel);
    if(ch < 0)
        return -EINVAL;

    return cps_readChannel(&(t->cache.channel), ch);
}

int tuner_set_mode_channel(struct tuner *t, const uint16_t channel)
{
    if(t == NULL)
        return -EINVAL;

    t->core.tuner_mode = TUNER_CHAN;
    t->core.channel_index = channel;

    return cps_readChannel(&(t->cache.channel), channel);
}

void tuner_set_power(struct tuner *t, const uint32_t power)
{
    if(t == NULL)
        return;
    t->core.power = power;
}

void tuner_set_vfo_rx_frequency(struct tuner *t, const uint32_t freq)
{
    if(t == NULL)
        return;

    t->core.vfo.rx_frequency = freq;
}

void tuner_set_vfo_tx_frequency(struct tuner *t, const uint32_t freq)
{
    if(t == NULL)
        return;

    t->core.vfo.tx_frequency = freq;
}

void tuner_set_vfo_opmode(struct tuner *t, const uint8_t opmode, const void *params)
{
    if( (t == NULL) || (params == NULL) )
        return;

    switch(opmode)
    {
        case OPMODE_M17:
            t->core.vfo.op_mode = opmode;
            m17Info_t *m17 = (m17Info_t *)params;
            t->core.vfo.m17 = *m17;
            break;
        case OPMODE_FM:
            t->core.vfo.op_mode = opmode;
            fmInfo_t *fm = (fmInfo_t *)params;
            t->core.vfo.fm = *fm;
            break;
        default:
            return;
    }

}

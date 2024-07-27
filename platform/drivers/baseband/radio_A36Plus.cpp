/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *   Copyright (C) 2024 by Jamiexu                                         *
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

#include <drivers/USART0.h>
#include <gd32f3x0.h>
#include <hwconfig.h>
#include <interfaces/nvmem.h>
#include <interfaces/radio.h>
#include <peripherals/gpio.h>
#include <utils.h>

#include <algorithm>
#include <string>

#include "bk4819.h"
#include "bk1080.h"
#include "radioUtils.h"

static const rtxStatus_t*
    config;  // Pointer to data structure with radio configuration

// static gdxCalibration_t calData;        // Calibration data
// static Band currRxBand     = BND_NONE;  // Current band for RX
// static Band currTxBand     = BND_NONE;  // Current band for TX
// static uint16_t apcVoltage = 0;  // APC voltage for TX output power control

static enum opstatus radioStatus;  // Current operating status

// static HR_C6000& C6000  = HR_C6000::instance();  // HR_C5000 driver
// static AT1846S& at1846s = AT1846S::instance();   // AT1846S driver


/**
 * Calculate DCS parity and compose
 */
static uint32_t cdcss_compose(uint16_t cdcss_code){
    uint32_t data = 0;

    data |= (((cdcss_code & 0x01) ^ ((cdcss_code >> 1)& 0x01) ^ ((cdcss_code >> 2)& 0x01) ^ ((cdcss_code >> 3)& 0x01) ^ ((cdcss_code >> 4)& 0x01) ^ ((cdcss_code >> 7)& 0x01)) << 12);
    data |= (!(((cdcss_code >> 1)& 0x01) ^ ((cdcss_code >> 2)& 0x01) ^ ((cdcss_code >> 3)& 0x01) ^ ((cdcss_code >> 4)& 0x01) ^ ((cdcss_code >> 5)& 0x01) ^ ((cdcss_code >> 8)& 0x01)) << 13);
    data |= (((cdcss_code & 0x01) ^ ((cdcss_code >> 1)& 0x01) ^ ((cdcss_code >> 5)& 0x01) ^ ((cdcss_code >> 6)& 0x01) ^ ((cdcss_code >> 8)& 0x01)) << 14);
    data |= (!(((cdcss_code >> 1)& 0x01) ^ ((cdcss_code >> 2)& 0x01) ^ ((cdcss_code >> 6)& 0x01) ^ ((cdcss_code >> 7)& 0x01) ^ ((cdcss_code >> 8)& 0x01)) << 15);
    data |= (!((cdcss_code & 0x01) ^ ((cdcss_code >> 1)& 0x01) ^ ((cdcss_code >> 4)& 0x01) ^ ((cdcss_code >> 8)& 0x01)) << 16);
    data |= (!(((cdcss_code >> 1)& 0x01) ^ ((cdcss_code >> 3)& 0x01) ^ ((cdcss_code >> 4)& 0x01) ^ ((cdcss_code >> 5)& 0x01) ^ ((cdcss_code >> 7)& 0x01)) << 17);
    data |= (((cdcss_code & 0x01) ^ ((cdcss_code >> 2)& 0x01) ^ ((cdcss_code >> 3)& 0x01) ^ ((cdcss_code >> 5)& 0x01) ^ ((cdcss_code >> 6)& 0x01) ^ ((cdcss_code >> 7)& 0x01) ^ ((cdcss_code >> 8)& 0x01)) << 18);
    data |= ((((cdcss_code >> 1)& 0x01) ^ ((cdcss_code >> 3)& 0x01) ^ ((cdcss_code >> 4)& 0x01) ^ ((cdcss_code >> 6)& 0x01) ^ ((cdcss_code >> 7)& 0x01) ^ ((cdcss_code >> 8)& 0x01)) << 19);
    data |= ((((cdcss_code >> 2)& 0x01) ^ ((cdcss_code >> 4)& 0x01) ^ ((cdcss_code >> 5)& 0x01) ^ ((cdcss_code >> 7)& 0x01) ^ ((cdcss_code >> 8)& 0x01)) << 20);
    data |= (!(((cdcss_code >> 3)& 0x01) ^ ((cdcss_code >> 5)& 0x01) ^ ((cdcss_code >> 6)& 0x01) ^ ((cdcss_code >> 8)& 0x01)) << 21);
    data |= (!((cdcss_code & 0x01) ^ ((cdcss_code >> 1)& 0x01) ^ ((cdcss_code >> 2)& 0x01) ^ ((cdcss_code >> 3)& 0x01) ^ ((cdcss_code >> 6)& 0x01)) << 22);

    data |= (0x04 << 9);

    data |= cdcss_code;

    return data;
    
}

void radio_init(const rtxStatus_t* rtxState)
{
    config      = rtxState;
    radioStatus = OFF;

    /*
     * Configure RTX GPIOs
     */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOF);


    gpio_setMode(BK4819_CLK, OUTPUT);
    gpio_setMode(BK4819_DAT, OUTPUT);
    gpio_setMode(BK4819_CS, OUTPUT);
    gpio_setMode(MIC_SPK_EN, OUTPUT);

    gpio_setMode(BK1080_DAT, OUTPUT);
    gpio_setMode(BK1080_CLK, OUTPUT);
    gpio_setMode(BK1080_EN, OUTPUT);

    // gpio_clearPin(BK1080_EN);
    gpio_clearPin(MIC_SPK_EN);
    bk4819_init();
    
    bk4819_enable_freq_scan(BK4819_SCAN_FRE_TIME_2);
    // bk4819_enable_vox(0, 0x10, 0x30, 0x30);
}

void radio_terminate()
{
}

void radio_tuneVcxo(const int16_t vhfOffset, const int16_t uhfOffset)
{
    (void)vhfOffset;
    (void)uhfOffset;
}

void radio_setOpmode(const enum opmode mode)
{
    (void)mode;
}

bool radio_checkRxDigitalSquelch()
{
    return bk4819_get_ctcss();
}

void radio_enableAfOutput()
{
    
}

void radio_disableAfOutput()
{
    
}

void radio_checkVOX(){
    radio_disableRtx();
    if (bk4819_get_vox()){
        if (radioStatus != TX)
            radio_enableTx();
    }else{
        if (radioStatus != RX)
        radio_enableRx();
    }
}

void radio_enableRx()
{
    bk4819_set_freq(config->rxFrequency / 10);
    if (config->rxToneEn){
        bk4819_enable_rx_ctcss(config->rxTone / 10);
    }
    bk4819_rx_on();
    radioStatus = RX;
}

void radio_enableTx()
{
    bk4819_set_freq(config->txFrequency / 10);
    bk4819_enable_tx_cdcss(1, 0, cdcss_compose(492));
    if (config->txToneEn){
        bk4819_enable_tx_ctcss(config->txTone / 10);
    }
    bk4819_tx_on();
    radioStatus = TX;
}

void radio_disableRtx()
{
    // bk4819_disable_ctdcss();


    radioStatus = OFF;
}

void radio_updateConfiguration()
{

}

rssi_t radio_getRssi()
{
    return bk4819_get_rssi();
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}

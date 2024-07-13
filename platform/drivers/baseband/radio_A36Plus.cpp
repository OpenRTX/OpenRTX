/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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

void radio_init(const rtxStatus_t* rtxState)
{
    config      = rtxState;
    radioStatus = OFF;

    /*
     * Configure RTX GPIOs
     */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);

    gpio_setMode(BK4819_CLK, OUTPUT);
    gpio_setMode(BK4819_DAT, OUTPUT);
    gpio_setMode(BK4819_CS, OUTPUT);
    gpio_setMode(MIC_SPK_EN, OUTPUT);
    gpio_setPin(MIC_SPK_EN);
    bk4819_init();
    /*
     * Enable and configure DAC for PA drive control
     */
    // SIM->SCGC6 |= SIM_SCGC6_DAC0_MASK;
    // DAC0->DAT[0].DATL = 0;
    // DAC0->DAT[0].DATH = 0;
    // DAC0->C0   |= DAC_C0_DACRFS_MASK    // Reference voltage is Vref2
    //            |  DAC_C0_DACEN_MASK;    // Enable DAC

    // /*
    //  * Load calibration data
    //  */
    // nvm_readCalibData(&calData);

    // /*
    //  * Enable and configure both AT1846S and HR_C6000, keep AF output
    //  disabled
    //  * at power on.
    //  */
    // at1846s.init();
    // C6000.init();
    // radio_disableAfOutput();
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
    return false;
}

void radio_enableAfOutput()
{
    // gpio_clearPin(MIC_EN);  // open microphone
}

void radio_disableAfOutput()
{
    // gpio_setPin(MIC_EN);  // close microphone
}

void radio_enableRx()
{
    gpio_setPin(MIC_SPK_EN);  // open speaker
    bk4819_set_freq(config->rxFrequency / 10);
    bk4819_rx_on();
    radioStatus = RX;
}

void radio_enableTx()
{
    gpio_clearPin(MIC_SPK_EN);  // open microphone
    bk4819_set_freq(config->txFrequency / 10);
    // if (config->txToneEn){
        bk4819_CTDCSS_enable(1);
        bk4819_set_CTCSS(0, 1646);
    // }
    bk4819_tx_on();
    radioStatus = TX;
}

void radio_disableRtx()
{
    // if (radioStatus == TX){
    //     gpio_setPin(MIC_EN);  // close microphone
    // }
    // radioStatus = OFF;
}

void radio_updateConfiguration()
{
    bk4819_set_freq(config->rxFrequency / 10);
}

rssi_t radio_getRssi()
{
    return (ReadRegister(0x67) & 0xff) / 2 - 160;
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}

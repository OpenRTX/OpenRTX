/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <hwconfig.h>
#include <datatypes.h>
#include <gpio.h>
#include "rtx.h"
#include "pll_MD3x0.h"
#include "ADC1_MDxx380.h"
#include "HR-C5000_MD3x0.h"

const freq_t IF_FREQ = 49950000.0f;  /* Intermediate frequency: 49.95MHz */

freq_t rxFreq = 430000000.0f;
freq_t txFreq = 430000000.0f;

void _setApcTv(uint16_t value)
{
    DAC->DHR12R1 = value;
}

void rtx_init()
{
    /*
     * Configure GPIOs
     */
    gpio_setMode(PLL_PWR,   OUTPUT);
    gpio_setMode(VCOVCC_SW, OUTPUT);
    gpio_setMode(DMR_SW,    OUTPUT);
    gpio_setMode(WN_SW,     OUTPUT);
    gpio_setMode(FM_SW,     OUTPUT);
    gpio_setMode(RF_APC_SW, OUTPUT);
    gpio_setMode(TX_STG_EN, OUTPUT);
    gpio_setMode(RX_STG_EN, OUTPUT);

    gpio_clearPin(PLL_PWR);    /* PLL off                                           */
    gpio_setPin(VCOVCC_SW);    /* VCOVCC high enables RX VCO, TX VCO if low         */
    gpio_clearPin(WN_SW);      /* 25kHz band (?)                                    */
    gpio_clearPin(DMR_SW);     /* Disconnect HR_C5000 input IF signal and audio out */
    gpio_clearPin(FM_SW);      /* Disconnect analog FM audio path                   */
    gpio_clearPin(RF_APC_SW);  /* Disable RF power control                          */
    gpio_clearPin(TX_STG_EN);  /* Disable TX power stage                            */
    gpio_clearPin(RX_STG_EN);  /* Disable RX input stage                            */

    /*
     * Configure and enble DAC
     */
    gpio_setMode(APC_TV,    INPUT_ANALOG);
    gpio_setMode(MOD2_BIAS, INPUT_ANALOG);

    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR = DAC_CR_EN2 | DAC_CR_EN1;
    DAC->DHR12R2 = 0;
    DAC->DHR12R1 = 0;

    /*
     * Enable and configure PLL
     */
    gpio_setPin(PLL_PWR);
    pll_init();

    /*
     * Configure HR_C5000
     */
    C5000_init();

    /*
     * Modulation bias settings
     */
    const uint8_t mod2_bias = 0x60;        /* TODO use calibration  */
    DAC->DHR12R2 = mod2_bias*4 + 0x600;    /* Original FW does this */
    C5000_setModOffset(mod2_bias);
}

void rtx_terminate()
{
    pll_terminate();

    gpio_clearPin(PLL_PWR);    /* PLL off                                           */
    gpio_clearPin(DMR_SW);     /* Disconnect HR_C5000 input IF signal and audio out */
    gpio_clearPin(FM_SW);      /* Disconnect analog FM audio path                   */
    gpio_clearPin(RF_APC_SW);  /* Disable RF power control                          */
    gpio_clearPin(TX_STG_EN);  /* Disable TX power stage                            */
    gpio_clearPin(RX_STG_EN);  /* Disable RX input stage                            */

    DAC->DHR12R2 = 0;
    DAC->DHR12R1 = 0;
    RCC->APB1ENR &= ~RCC_APB1ENR_DACEN;
}

void rtx_setTxFreq(freq_t freq)
{
    txFreq = freq;
}

void rtx_setRxFreq(freq_t freq)
{
    rxFreq = freq;
}

void rtx_setFuncmode(enum funcmode mode)
{
    switch(mode)
    {
        case OFF:
            gpio_clearPin(TX_STG_EN);
            gpio_clearPin(RX_STG_EN);
            break;

        case RX:
            gpio_clearPin(TX_STG_EN);

            gpio_clearPin(RF_APC_SW);
            gpio_setPin(VCOVCC_SW);
            pll_setFrequency(rxFreq - IF_FREQ, 5);
            _setApcTv(0x956);                   /* TODO use calibration      */

            gpio_setPin(RX_STG_EN);
            break;

        case TX:
            gpio_clearPin(RX_STG_EN);

            gpio_setPin(RF_APC_SW);
            gpio_clearPin(VCOVCC_SW);
            pll_setFrequency(txFreq, 5);

            _setApcTv(0x02);

            gpio_setPin(TX_STG_EN);
            break;

        default:
            /* TODO */
            break;
    }
}

void rtx_setToneRx(enum tone t)
{
    /* TODO */
}

void rtx_setToneTx(enum tone t)
{
    /* TODO */
}

void rtx_setBandwidth(enum bw b)
{
    switch(b)
    {
        case BW_25:
            gpio_clearPin(WN_SW);
            break;

        case BW_12_5:
            gpio_setPin(WN_SW);
            break;

        default:
            /* TODO */
            break;
    }
}

float rtx_getRssi()
{
    return adc1_getMeasurement(1);
}

void rtx_setOpmode(enum opmode mode)
{
    switch(mode)
    {
        case FM:
            gpio_clearPin(DMR_SW);
            gpio_setPin(FM_SW);
            C5000_fmMode();
            break;

        case DMR:
            gpio_clearPin(FM_SW);
            gpio_setPin(DMR_SW);
            break;

        default:
            /* TODO */
            break;
    }
}

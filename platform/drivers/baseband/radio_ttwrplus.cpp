/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/radio.h"
#include <algorithm>
#include "pmu.h"
#include "radioUtils.h"
#include "drivers/baseband/AT1846S.h"
#include "drivers/baseband/SA8x8.h"

static const rtxStatus_t *config;                // Pointer to data structure with radio configuration

static Band currRxBand = BND_NONE;               // Current band for RX
static Band currTxBand = BND_NONE;               // Current band for TX
static enum opstatus radioStatus;                // Current operating status

static AT1846S& at1846s = AT1846S::instance();   // AT1846S driver


void radio_init(const rtxStatus_t *rtxState)
{
    config      = rtxState;
    radioStatus = OFF;

    // Set SA8x8 serial to 115200 baud, module has alredy been initialized in
    // platform_init()
    sa8x8_enableHSMode();

    // Mute speaker power amplifier by default
    sa8x8_setAudio(false);

    /*
     * Configure AT1846S, keep AF output disabled at power on.
     */
    at1846s.init();
}

void radio_disableRtx()
{
    at1846s.disableCtcss();
    at1846s.setFuncMode(AT1846S_FuncMode::OFF);
    radioStatus = OFF;
}

void radio_terminate()
{
    radio_disableRtx();
    at1846s.terminate();
}

void radio_setOpmode(const enum opmode mode)
{
    switch(mode)
    {
        case OPMODE_FM:
            at1846s.setOpMode(AT1846S_OpMode::FM);  // AT1846S in FM mode
            break;

        case OPMODE_DMR:
            at1846s.setOpMode(AT1846S_OpMode::DMR);
            at1846s.setBandwidth(AT1846S_BW::_12P5);
            break;

        case OPMODE_M17:
            at1846s.setOpMode(AT1846S_OpMode::DMR); // AT1846S in DMR mode, disables RX filter
            at1846s.setBandwidth(AT1846S_BW::_25);  // Set bandwidth to 25kHz for proper deviation
            break;

        default:
            break;
    }
}

bool radio_checkRxDigitalSquelch()
{
    return at1846s.rxCtcssDetected();
}

void radio_enableAfOutput()
{
    ;
}

void radio_disableAfOutput()
{
    ;
}

void radio_enableRx()
{
    if(currRxBand == BND_NONE) return;

    at1846s.setFrequency(config->rxFrequency);
    at1846s.setFuncMode(AT1846S_FuncMode::RX);

    if(config->rxToneEn)
    {
        at1846s.enableRxCtcss(config->rxTone);
    }

    radioStatus = RX;
}

void radio_enableTx()
{
    if(config->txDisable == 1) return;

    // Constrain output power between 1W and 5W.
    uint32_t power = std::max(std::min(config->txPower, 5000U), 1000U);
    sa8x8_setTxPower(power);

    at1846s.setFrequency(config->txFrequency);
    at1846s.setFuncMode(AT1846S_FuncMode::TX);

    if(config->txToneEn)
    {
        at1846s.enableTxCtcss(config->txTone);
    }

    radioStatus = TX;
}

void radio_updateConfiguration()
{
    currRxBand = getBandFromFrequency(config->rxFrequency);
    currTxBand = getBandFromFrequency(config->txFrequency);

    if((currRxBand == BND_NONE) || (currTxBand == BND_NONE)) return;

    // Set bandwidth, only for analog FM mode
    if(config->opMode == OPMODE_FM)
    {
        switch(config->bandwidth)
        {
            case BW_12_5:
                at1846s.setBandwidth(AT1846S_BW::_12P5);
                break;

             case BW_25:
                at1846s.setBandwidth(AT1846S_BW::_25);
                break;

             default:
                 break;
        }
    }

    /*
     * Update VCO frequency and tuning parameters if current operating status
     * is different from OFF.
     * This is done by calling again the corresponding functions, which is safe
     * to do and avoids code duplication.
     */
    if(radioStatus == RX) radio_enableRx();
    if(radioStatus == TX) radio_enableTx();
}

rssi_t radio_getRssi()
{
    return static_cast< rssi_t > (at1846s.readRSSI());
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}

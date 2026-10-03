/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <interfaces/nvmem.h>

#include <hwconfig.h>
#include <interfaces/nvmem.h>
#include <interfaces/radio.h>
#include <core/utils.h>

#include <algorithm>
#include <string>

#include "drivers/baseband/BK4819.h"
#include "radioUtils.h"

/* platform function to control APC */
extern "C" {
void platform_set_tx_power(uint8_t power_percent);
}

static const rtxStatus_t
    *config; // Pointer to data structure with radio configuration

static enum opstatus radioStatus; // Current operating status

/* The BK4819 GPIOs to control the C62 LNA and PA */
enum {
    GPIO_VHF_RX_LNA = 0,
    GPIO_UHF_RX_LNA,
    GPIO_VHF_TX_PA,
    GPIO_UHF_TX_PA,
    GPIO_ALC_TX_LED
};

/**
 * Calculate DCS parity and compose
 */
static uint32_t cdcss_compose(uint16_t cdcss_code)
{
    uint32_t data = 0;

    data |= (((cdcss_code & 0x01) ^ ((cdcss_code >> 1) & 0x01)
              ^ ((cdcss_code >> 2) & 0x01) ^ ((cdcss_code >> 3) & 0x01)
              ^ ((cdcss_code >> 4) & 0x01) ^ ((cdcss_code >> 7) & 0x01))
             << 12);
    data |= (!(((cdcss_code >> 1) & 0x01) ^ ((cdcss_code >> 2) & 0x01)
               ^ ((cdcss_code >> 3) & 0x01) ^ ((cdcss_code >> 4) & 0x01)
               ^ ((cdcss_code >> 5) & 0x01) ^ ((cdcss_code >> 8) & 0x01))
             << 13);
    data |= (((cdcss_code & 0x01) ^ ((cdcss_code >> 1) & 0x01)
              ^ ((cdcss_code >> 5) & 0x01) ^ ((cdcss_code >> 6) & 0x01)
              ^ ((cdcss_code >> 8) & 0x01))
             << 14);
    data |= (!(((cdcss_code >> 1) & 0x01) ^ ((cdcss_code >> 2) & 0x01)
               ^ ((cdcss_code >> 6) & 0x01) ^ ((cdcss_code >> 7) & 0x01)
               ^ ((cdcss_code >> 8) & 0x01))
             << 15);
    data |= (!((cdcss_code & 0x01) ^ ((cdcss_code >> 1) & 0x01)
               ^ ((cdcss_code >> 4) & 0x01) ^ ((cdcss_code >> 8) & 0x01))
             << 16);
    data |= (!(((cdcss_code >> 1) & 0x01) ^ ((cdcss_code >> 3) & 0x01)
               ^ ((cdcss_code >> 4) & 0x01) ^ ((cdcss_code >> 5) & 0x01)
               ^ ((cdcss_code >> 7) & 0x01))
             << 17);
    data |= (((cdcss_code & 0x01) ^ ((cdcss_code >> 2) & 0x01)
              ^ ((cdcss_code >> 3) & 0x01) ^ ((cdcss_code >> 5) & 0x01)
              ^ ((cdcss_code >> 6) & 0x01) ^ ((cdcss_code >> 7) & 0x01)
              ^ ((cdcss_code >> 8) & 0x01))
             << 18);
    data |= ((((cdcss_code >> 1) & 0x01) ^ ((cdcss_code >> 3) & 0x01)
              ^ ((cdcss_code >> 4) & 0x01) ^ ((cdcss_code >> 6) & 0x01)
              ^ ((cdcss_code >> 7) & 0x01) ^ ((cdcss_code >> 8) & 0x01))
             << 19);
    data |= ((((cdcss_code >> 2) & 0x01) ^ ((cdcss_code >> 4) & 0x01)
              ^ ((cdcss_code >> 5) & 0x01) ^ ((cdcss_code >> 7) & 0x01)
              ^ ((cdcss_code >> 8) & 0x01))
             << 20);
    data |= (!(((cdcss_code >> 3) & 0x01) ^ ((cdcss_code >> 5) & 0x01)
               ^ ((cdcss_code >> 6) & 0x01) ^ ((cdcss_code >> 8) & 0x01))
             << 21);
    data |= (!((cdcss_code & 0x01) ^ ((cdcss_code >> 1) & 0x01)
               ^ ((cdcss_code >> 2) & 0x01) ^ ((cdcss_code >> 3) & 0x01)
               ^ ((cdcss_code >> 6) & 0x01))
             << 22);

    data |= (0x04 << 9);

    data |= cdcss_code;

    return data;
}

void radio_init(const rtxStatus_t *rtxState)
{
    config = rtxState;
    radioStatus = OFF;

    BK4819_SetAF(&c62_bk4819, 0);

    bk4819_gpio_pin_set(&c62_bk4819, GPIO_VHF_RX_LNA,
                        false); // VHF RX LNA
    bk4819_gpio_pin_set(&c62_bk4819, GPIO_UHF_RX_LNA,
                        false); // UHF RX LNA
    bk4819_gpio_pin_set(&c62_bk4819, GPIO_VHF_TX_PA,
                        false); // VHF TX PA
    bk4819_gpio_pin_set(&c62_bk4819, GPIO_UHF_TX_PA,
                        false); // UHF TX PA
    bk4819_gpio_pin_set(&c62_bk4819, GPIO_ALC_TX_LED,
                        false); // ALC / TX LED
}

void radio_terminate()
{
}

void radio_setBandwidth(const uint8_t bandwidth)
{
    bk4819_SetFilterBandwidth(&c62_bk4819, bandwidth);
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
    return bk4819_get_ctcss(&c62_bk4819);
}

void radio_enableAfOutput()
{
    bk4819_set_modulation(&c62_bk4819, true);
    return;
}

void radio_disableAfOutput()
{
    BK4819_SetAF(&c62_bk4819, 0);
}

void radio_checkVOX()
{
    return;
    radio_disableRtx();
    if (bk4819_get_vox(&c62_bk4819)) {
        if (radioStatus != TX)
            radio_enableTx();
    } else {
        if (radioStatus != RX)
            radio_enableRx();
    }
}

void radio_setRxFilters(uint32_t freq)
{
    if (freq < 174000000) {
        bk4819_gpio_pin_set(&c62_bk4819, GPIO_VHF_RX_LNA,
                            true); // VHF RX LNA
    } else {
        bk4819_gpio_pin_set(&c62_bk4819, GPIO_UHF_RX_LNA,
                            true); // UHF RX LNA
    }
}

void radio_enableRx()
{
    bk4819_gpio_pin_set(&c62_bk4819, 0, false); // VHF RX LNA
    bk4819_gpio_pin_set(&c62_bk4819, 1, false); // UHF RX LNA
    bk4819_gpio_pin_set(&c62_bk4819, 2, false); // UHF TX PA
    bk4819_gpio_pin_set(&c62_bk4819, 3, false); // UHF TX PA
    bk4819_gpio_pin_set(&c62_bk4819, 4, false); // ALC / TX LED

    radio_setRxFilters(config->rxFrequency);
    bk4819_set_freq(&c62_bk4819, config->rxFrequency);

    if (config->rxToneEn) {
        bk4819_enable_rx_ctcss(&c62_bk4819, config->rxTone);
    }

    bk4819_rx_on(&c62_bk4819);
    radioStatus = RX;
}

void radio_enableTx()
{
    if (config->txDisable == 1)
        return;

    bk4819_gpio_pin_set(&c62_bk4819, GPIO_VHF_RX_LNA,
                        false); // VHF RX LNA
    bk4819_gpio_pin_set(&c62_bk4819, GPIO_UHF_RX_LNA,
                        false); // UHF RX LNA
    bk4819_gpio_pin_set(&c62_bk4819, GPIO_VHF_TX_PA,
                        false); // VHF TX PA
    bk4819_gpio_pin_set(&c62_bk4819, GPIO_UHF_TX_PA,
                        false); // UHF TX PA
    bk4819_gpio_pin_set(&c62_bk4819, GPIO_ALC_TX_LED,
                        false); // ALC / TX LED

    if (config->txFrequency < 136000000 || config->txFrequency > 600000000)
        return;

    bk4819_set_freq(&c62_bk4819, config->txFrequency);

    if (config->txToneEn) {
        bk4819_enable_tx_ctcss(&c62_bk4819, config->txTone);
    }

    if (config->txFrequency < 174000000) {
        bk4819_gpio_pin_set(&c62_bk4819, GPIO_VHF_TX_PA,
                            true); // VHF TX PA
    } else {
        bk4819_gpio_pin_set(&c62_bk4819, GPIO_UHF_TX_PA,
                            true); // UHF TX PA
    }

    bk4819_gpio_pin_set(&c62_bk4819, GPIO_ALC_TX_LED,
                        true); // ALC / TX LED

    // depending on power level set PWM duty cycle for APC voltage control
    // Maybe need table for this instead of crude linear mapping, and also consider frequency dependence of PA efficiency
    platform_set_tx_power(std::min(
        config->txPower * 100 / 5000,
        100U)); // crude linear mapping of power to duty cycle, max at 5W

    bk4819_tx_on(&c62_bk4819);
    radioStatus = TX;
}

void radio_disableRtx()
{
    bk4819_disable_ctdcss(&c62_bk4819);

    bk4819_gpio_pin_set(&c62_bk4819, GPIO_VHF_RX_LNA,
                        false); // VHF RX LNA
    bk4819_gpio_pin_set(&c62_bk4819, GPIO_UHF_RX_LNA,
                        false); // UHF RX LNA
    bk4819_gpio_pin_set(&c62_bk4819, GPIO_VHF_TX_PA,
                        false); // VHF TX PA
    bk4819_gpio_pin_set(&c62_bk4819, GPIO_UHF_TX_PA,
                        false); // UHF TX PA
    bk4819_gpio_pin_set(&c62_bk4819, GPIO_ALC_TX_LED,
                        false); // ALC / TX LED

    bk4819_rtx_off(&c62_bk4819);
    radioStatus = OFF;
}

void radio_updateConfiguration()
{
    // Set squelch
    int squelch = -127 + (config->sqlLevel * 66) / 15;
    bk4819_set_Squelch(&c62_bk4819, ((squelch + 160) * 2),
                       ((squelch - 3 + 160) * 2), 0x5f, 0x5e, 0x20, 0x08);

    // Set BK4819 PA Gain tuning according to TX power and frequency

    bk4819_set_freq(&c62_bk4819, config->rxFrequency);

    if (radioStatus == RX) {
        radio_setRxFilters(config->rxFrequency);
        radio_setBandwidth(config->bandwidth);
    }
}

rssi_t radio_getRssi()
{
    return bk4819_get_rssi(&c62_bk4819);
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}
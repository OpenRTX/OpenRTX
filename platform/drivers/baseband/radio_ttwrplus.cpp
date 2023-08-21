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

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <interfaces/delays.h>
#include <interfaces/radio.h>
#include <rtx.h>
#include <ui.h>
#include <algorithm>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "AT1846S.h"
#include "radioUtils.h"

/*
 * Define radio node to control the SA868
 */
#if DT_NODE_HAS_STATUS(DT_ALIAS(radio), okay)
#define UART_RADIO_DEV_NODE DT_ALIAS(radio)
#else
#error "Please select the correct radio UART device"
#endif

#define SA8X8_MSG_SIZE 32

K_MSGQ_DEFINE(uart_msgq, SA8X8_MSG_SIZE, 10, 4);

/* receive buffer used in UART ISR callback */
static char rx_buf[SA8X8_MSG_SIZE];
static uint16_t rx_buf_pos;

static const struct device *const radio_dev = DEVICE_DT_GET(UART_RADIO_DEV_NODE);

#define RADIO_PDN_NODE DT_ALIAS(radio_pdn)

static const struct gpio_dt_spec radio_pdn = GPIO_DT_SPEC_GET(RADIO_PDN_NODE, gpios);


const rtxStatus_t    *config;   // Pointer to data structure with radio configuration

Band    currRxBand = BND_NONE;  // Current band for RX
Band    currTxBand = BND_NONE;  // Current band for TX

enum opstatus radioStatus;      // Current operating status

AT1846S& at1846s = AT1846S::instance();   // AT1846S driver

void radio_serialCb(const struct device *dev, void *user_data)
{
    uint8_t c;

    if (!uart_irq_update(radio_dev)) {
        return;
    }

    if (!uart_irq_rx_ready(radio_dev)) {
        return;
    }

    /* read until FIFO empty */
    while (uart_fifo_read(radio_dev, &c, 1) == 1) {
        if (c == '\n' && rx_buf_pos > 0) {
            /* terminate string */
            rx_buf[rx_buf_pos] = '\0';

            /* if queue is full, message is silently dropped */
            k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);

            /* reset the buffer (it was copied to the msgq) */
            rx_buf_pos = 0;
        } else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
            rx_buf[rx_buf_pos++] = c;
        }
        /* else: characters beyond buffer size are dropped */
    }
}

void radio_uartPrint(const char *fmt, ...)
{
    char buf[SA8X8_MSG_SIZE] = { 0 };
    va_list args;
    va_start(args, fmt);
    vsnprintk(buf, SA8X8_MSG_SIZE, fmt, args);
    int msg_len = strnlen(buf, SA8X8_MSG_SIZE);
    for (uint16_t i = 0; i < msg_len; i++) {
        uart_poll_out(radio_dev, buf[i]);
    }
    va_end(args);
}

void radio_uartScan(char *buf)
{
    k_msgq_get(&uart_msgq, buf, K_FOREVER);
}

char *radio_getFwVersion()
{
    char *tx_buf = (char *) malloc(sizeof(char) * SA8X8_MSG_SIZE);
    radio_uartPrint("AT+VERSION\r\n");
    k_msgq_get(&uart_msgq, tx_buf, K_FOREVER);
    return tx_buf;
}

char *radio_getModel()
{
    char *tx_buf = (char *) malloc(sizeof(char) * SA8X8_MSG_SIZE);
    radio_uartPrint("AT+MODEL\r\n");
    k_msgq_get(&uart_msgq, tx_buf, K_FOREVER);
    return tx_buf;
}

void radio_init(const rtxStatus_t *rtxState)
{
    config      = rtxState;
    radioStatus = OFF;
    int ret;

    // Initialize GPIO for SA868S power down
    if (!gpio_is_ready_dt(&radio_pdn)) {
        printk("Error: radio device %s is not ready\n",
               radio_pdn.port->name);
    }

    ret = gpio_pin_configure_dt(&radio_pdn, GPIO_OUTPUT);
    if (ret != 0) {
        printk("Error %d: failed to configure %s pin %d\n", ret, radio_pdn.port->name, radio_pdn.pin);
    }

    if (!device_is_ready(radio_dev)) {
        printk("UART device not found!\n");
        return;
    }

    ret = uart_irq_callback_user_data_set(radio_dev, radio_serialCb, NULL);

    if (ret < 0) {
        if (ret == -ENOTSUP) {
            printk("Interrupt-driven UART support not enabled\n");
        } else if (ret == -ENOSYS) {
            printk("UART device does not support interrupt-driven API\n");
        } else {
            printk("Error setting UART callback: %d\n", ret);
        }

        return;
    }

    uart_irq_rx_enable(radio_dev);

    ret = gpio_pin_toggle_dt(&radio_pdn);
    if (ret != 0) {
        printk("Failed to toggle radio power down");
        return;
    }

    // A small delay is needed to have SA8x8 ready to serve commands
    delayMs(100);

    // Check for minimum supported firmware version.
    char *fwVersionStr = radio_getFwVersion();
    uint8_t major = 0, minor = 0, patch = 0, release = 0;
    sscanf(fwVersionStr, "sa8x8-fw/v%hhu.%hhu.%hhu.r%hhu", &major, &minor, &patch, &release);
    if (major < 1 || (major == 1 && minor < 1) || (major == 1 && minor == 1 && patch == 0 && release < 14))
    {
        printk("Error: unsupported baseband firmware, please update!\n");
        return;
    }
    free(fwVersionStr);

    // TODO: Implement audio paths configuration

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

void radio_tuneVcxo(const int16_t vhfOffset, const int16_t uhfOffset)
{
    //TODO: this part will be implemented in the future, when proved to be
    // necessary.
    (void) vhfOffset;
    (void) uhfOffset;
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

             case BW_20:
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

float radio_getRssi()
{
    return static_cast< float > (at1846s.readRSSI());
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}

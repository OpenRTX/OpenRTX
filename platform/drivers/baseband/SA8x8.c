/***************************************************************************
 *   Copyright (C) 2023 by Silvano Seva IU2KWO                             *
 *                     and Niccolò Izzo IU2KIN                             *
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

#include <SA8x8.h>
#include <interfaces/delays.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

/*
 * Minimum required version of sa868-fw
 */
#define SA868FW_MAJOR 1
#define SA868FW_MINOR 3
#define SA868FW_PATCH 0

#if DT_NODE_HAS_STATUS(DT_ALIAS(radio), okay)
#define UART_RADIO_DEV_NODE DT_ALIAS(radio)
#else
#error "Please select the correct radio UART device"
#endif

#define RADIO_PDN_NODE DT_ALIAS(radio_pdn)
#define RADIO_PWR_NODE DT_NODELABEL(radio_pwr)

static const struct device* const uart_dev = DEVICE_DT_GET(UART_RADIO_DEV_NODE);
static const struct gpio_dt_spec radio_pdn =
    GPIO_DT_SPEC_GET(RADIO_PDN_NODE, gpios);
static const struct gpio_dt_spec radio_pwr =
    GPIO_DT_SPEC_GET_OR(RADIO_PWR_NODE, gpios, {0});

K_MSGQ_DEFINE(uart_msgq, SA8X8_MSG_SIZE, 10, 4);

static uint16_t rx_buf_pos;
static char rx_buf[SA8X8_MSG_SIZE];

static void uartCallback(const struct device* dev, void* user_data)
{
    uint8_t c;

    if (uart_irq_update(uart_dev) == false) return;

    if (uart_irq_rx_ready(uart_dev) == false) return;

    // read until FIFO empty
    while (uart_fifo_read(uart_dev, &c, 1) == 1)
    {
        if ((c == '\n') && (rx_buf_pos > 0))
        {
            rx_buf[rx_buf_pos] = '\0';

            // if queue is full, message is silently dropped
            k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);
            rx_buf_pos = 0;
        }
        else if (rx_buf_pos < (sizeof(rx_buf) - 1))
        {
            rx_buf[rx_buf_pos++] = c;
        }
    }
}

static void uartPrint(const char* fmt, ...)
{
    char buf[SA8X8_MSG_SIZE];

    va_list args;
    va_start(args, fmt);
    int len = vsnprintk(buf, SA8X8_MSG_SIZE, fmt, args);
    va_end(args);

    for (int i = 0; i < len; i++) uart_poll_out(uart_dev, buf[i]);
}

static inline bool waitUntilReady()
{
    char buf[SA8X8_MSG_SIZE] = {0};

    for (int i = 0; i < 5; i++)
    {
        uartPrint("AT\r\n");
        int ret = k_msgq_get(&uart_msgq, buf, K_MSEC(250));
        if (ret != 0) printk("SA8x8: baseband is not ready!\n");

        if (strncmp(buf, "OK\r", SA8X8_MSG_SIZE) == 0) return true;

        delayMs(250);
    }
    printk("SA8x8: baseband is still not ready, giving up!\n");
    return false;
}

static inline bool checkFwVersion()
{
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint8_t revision;

    const char* fwVersionStr = sa8x8_getFwVersion();
    sscanf(fwVersionStr, "sa8x8-fw/v%hhu.%hhu.%hhu.r%hhu", &major, &minor,
           &patch, &revision);

    if ((major > SA868FW_MAJOR) ||
        ((major == SA868FW_MAJOR) && (minor > SA868FW_MINOR)) ||
        ((major == SA868FW_MAJOR) && (minor == SA868FW_MINOR) &&
         (patch > SA868FW_PATCH)) ||
        ((major == SA868FW_MAJOR) && (minor == SA868FW_MINOR) &&
         (patch == SA868FW_PATCH)))
    {
        return true;
    }

    // Major, minor, or patch not matching.
    printk("SA8x8: unsupported baseband firmware!\n");
    return false;
}

int sa8x8_init()
{
    int ret;

    struct uart_config uart_config = {
        .baudrate  = 9600,
        .parity    = UART_CFG_PARITY_NONE,
        .stop_bits = UART_CFG_STOP_BITS_1,
        .flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
        .data_bits = UART_CFG_DATA_BITS_8,
    };

    // Initialize UART for SA868 communication
    if (device_is_ready(uart_dev) == false)
    {
        printk("SA8x8: UART device not ready!\n");
        return -1;
    }

    // Configure UART
    if ((ret = uart_configure(uart_dev, &uart_config)))
    {
        printk("SA8x8: error while setting UART configuration!\n");
        return ret;
    }

    // Initialize GPIO for SA868S power down
    if (gpio_is_ready_dt(&radio_pdn) == false)
    {
        printk("SA8x8: error %d, radio device %s is not ready\n", ret,
               radio_pdn.port->name);
        return -1;
    }

    if ((ret = gpio_pin_configure_dt(&radio_pdn, GPIO_OUTPUT)))
    {
        printk("SA8x8: error %d, failed to configure %s pin %d\n", ret,
               radio_pdn.port->name, radio_pdn.pin);
        return -1;
    }

    // Initialize GPIO for SA868S high power mode
    if (gpio_is_ready_dt(&radio_pwr) == false)
    {
        printk("SA8x8: error %d, high power GPIO %s is not ready\n", ret,
               radio_pdn.port->name);
        return -1;
    }

    if ((ret = gpio_pin_configure_dt(&radio_pwr, GPIO_OUTPUT)))
    {
        printk("SA8x8: error %d, failed to configure %s pin %d\n", ret,
               radio_pwr.port->name, radio_pwr.pin);
        return -1;
    }

    // Setup UART for communication
    if (device_is_ready(uart_dev) == false)
    {
        printk("SA8x8: error %d, UART device not ready!\n", ret);
        return -1;
    }

    // Reset the SA868S baseband
    gpio_pin_set_dt(&radio_pdn, 1);
    delayMs(500);
    gpio_pin_set_dt(&radio_pdn, 0);

    if ((ret = uart_irq_callback_user_data_set(uart_dev, uartCallback, NULL)))
    {
        switch (ret)
        {
            case -ENOTSUP:
                printk(
                    "SA8x8: error %d, interrupt-driven UART support not "
                    "enabled\n",
                    ret);
                break;

            case -ENOSYS:
                printk(
                    "SA8x8: error %d, UART device does not support "
                    "interrupt-driven API\n",
                    ret);
                break;

            default:
                printk("SA8x8: error %d, cannot set UART callback\n", ret);
                break;
        }

        return -1;
    }

    uart_irq_rx_enable(uart_dev);

    if (!waitUntilReady() || !checkFwVersion())
    {
        printk("SA8x8: must flash a supported baseband version!\n", ret);
        return -2;
    }

    return 0;
}

const char* sa8x8_getModel()
{
    static char model[SA8X8_MSG_SIZE] = {0};

    if (model[0] == 0)
    {
        uartPrint("AT+MODEL\r\n");
        int ret = k_msgq_get(&uart_msgq, model, K_MSEC(100));
        if (ret != 0) printk("SA8x8: error while reading radio model\n");
    }

    return model;
}

const char* sa8x8_getFwVersion()
{
    static char fw_version[SA8X8_MSG_SIZE] = {0};

    if (fw_version[0] == 0)
    {
        uartPrint("AT+VERSION\r\n");
        int ret = k_msgq_get(&uart_msgq, fw_version, K_MSEC(100));
        if (ret != 0) printk("SA8x8: error while reading FW version\n");
    }

    return fw_version;
}

int sa8x8_enableHSMode()
{
    char buf[SA8X8_MSG_SIZE] = {0};
    struct uart_config uart_config;

    int ret = uart_config_get(uart_dev, &uart_config);
    if (ret != 0)
    {
        printk("SA8x8: error while retrieving UART configuration!\n");
        return ret;
    }

    uartPrint("AT+TURBO\r\n");
    ret = k_msgq_get(&uart_msgq, buf, K_MSEC(100));
    if (ret != 0)
    {
        printk("SA8x8: error while retrieving turbo response!\n");
        return ret;
    }

    uart_config.baudrate = 115200;

    if ((ret = uart_configure(uart_dev, &uart_config)))
    {
        printk("SA8x8: error while setting UART configuration!\n");
        return ret;
    }

    return 0;
}

void sa8x8_setTxPower(const float power)
{
    char buf[SA8X8_MSG_SIZE] = {0};

    // TODO: Implement fine-grained power control through PA_BIAS SA8x8 register
    uint8_t amp_enable = (power > 1.0f) ? 1 : 0;
    int ret            = gpio_pin_set_dt(&radio_pwr, amp_enable);
    if (ret != 0) printk("SA8x8: failed to change power mode\n");
}

void sa8x8_setAudio(bool value)
{
    char buf[SA8X8_MSG_SIZE] = {0};

    uartPrint("AT+AUDIO=%d\r\n", value);
    k_msgq_get(&uart_msgq, buf, K_MSEC(100));

    // Check that response is "OK\r"
    if (strncmp(buf, "OK\r", 3U) != 0)
        printk("SA8x8: failed to enable control speaker power amplifier\n");
}

void sa8x8_writeAT1846Sreg(uint8_t reg, uint16_t value)
{
    char buf[SA8X8_MSG_SIZE] = {0};

    uartPrint("AT+POKE=%d,%d\r\n", reg, value);
    k_msgq_get(&uart_msgq, buf, K_MSEC(100));

    // Check that response is "OK\r"
    if (strncmp(buf, "OK\r", 3U) != 0)
        printk("SA8x8: error writing register %d = %d\n", reg, value);
}

uint16_t sa8x8_readAT1846Sreg(uint8_t reg)
{
    char buf[SA8X8_MSG_SIZE] = {0};
    uint16_t value           = 0;

    uartPrint("AT+PEEK=%d\r\n", reg);
    k_msgq_get(&uart_msgq, buf, K_MSEC(100));

    int ret = sscanf(buf, "%hd\r", &value);
    if (ret != 1) printk("SA8x8: error reading register %d\n", reg);

    return value;
}

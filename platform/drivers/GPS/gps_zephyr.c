/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include "hwconfig.h"
#include "drivers/GPS/nmea_rbuf.h"

#if !DT_NODE_HAS_STATUS(DT_ALIAS(gps), okay)
#error "Please select the correct gps UART device"
#endif

static const struct device *const gpsDev = DEVICE_DT_GET(DT_ALIAS(gps));
static struct nmeaRbuf ringBuf;

static void uartRxCallback(const struct device *dev, void *user_data)
{
    uint8_t c;

    if(uart_irq_update(dev) == false)
        return;

    if(uart_irq_rx_ready(dev) == false)
        return;

    // read until FIFO empty
    while(uart_fifo_read(dev, &c, 1) == 1)
        nmeaRbuf_putChar(&ringBuf, c);
}

void gpsZephyr_init()
{
    int ret = 0;

    // Check if GPS UART is ready
    if(device_is_ready(gpsDev) == false) {
        printk("UART device not found!\n");
        return;
    }

    ret = uart_irq_callback_user_data_set(gpsDev, uartRxCallback, NULL);
    if(ret < 0) {
        switch(ret) {
            case -ENOTSUP:
                printk("Interrupt-driven UART support not enabled\n");
                break;

            case -ENOSYS:
                printk("UART device does not support interrupt-driven API\n");
                break;

            default:
                printk("Error setting UART callback: %d\n", ret);
                break;
        }
    }

    nmeaRbuf_reset(&ringBuf);
    uart_irq_rx_enable(gpsDev);
}

void gpsZephyr_terminate()
{
     uart_irq_rx_disable(gpsDev);
}

int gpsZephyr_getNmeaSentence(void *priv, char *buf, const size_t maxLength)
{
    (void) priv;

    return nmeaRbuf_getSentence(&ringBuf, buf, maxLength);
}

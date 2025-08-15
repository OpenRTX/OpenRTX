/***************************************************************************
 *   Copyright (C) 2023 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <hwconfig.h>
#include <nmea_rbuf.h>

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

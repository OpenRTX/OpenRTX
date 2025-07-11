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
#include <interfaces/delays.h>
#include <peripherals/gps.h>
#include <hwconfig.h>
#include <string.h>
#include <pmu.h>


#if DT_NODE_HAS_STATUS(DT_ALIAS(gps), okay)
#define UART_GPS_DEV_NODE DT_ALIAS(gps)
#else
#error "Please select the correct gps UART device"
#endif

#define NMEA_MSG_SIZE 82

K_MSGQ_DEFINE(gps_uart_msgq, NMEA_MSG_SIZE, 10, 4);

static const struct device *const gps_dev = DEVICE_DT_GET(UART_GPS_DEV_NODE);

// receive buffer used in UART ISR callback
static char rx_buf[NMEA_MSG_SIZE];
static uint16_t rx_buf_pos;


static void gps_serialCb(const struct device *dev, void *user_data)
{
    uint8_t c;

    if (uart_irq_update(gps_dev) == false)
        return;

    if (uart_irq_rx_ready(gps_dev) == false)
        return;

    // read until FIFO empty
    while (uart_fifo_read(gps_dev, &c, 1) == 1)
    {
        if (c == '$' && rx_buf_pos > 0)
        {
            // terminate string
            rx_buf[rx_buf_pos] = '\0';

            // if queue is full, message is silently dropped
            k_msgq_put(&gps_uart_msgq, &rx_buf, K_NO_WAIT);

            // reset the buffer (it was copied to the msgq)
            rx_buf_pos = 0;
            rx_buf[rx_buf_pos++] = '$';
        }
        else if (rx_buf_pos < (sizeof(rx_buf) - 1))
        {
            rx_buf[rx_buf_pos++] = c;
        }
    }
}


void gps_init(const uint16_t baud)
{
    int ret = 0;

    // Check if GPS UART is ready
    if (device_is_ready(gps_dev) == false)
    {
        printk("UART device not found!\n");
        return;
    }

    ret = uart_irq_callback_user_data_set(gps_dev, gps_serialCb, NULL);
    if (ret < 0)
    {
        if (ret == -ENOTSUP)
        {
            printk("Interrupt-driven UART support not enabled\n");
        }
        else if (ret == -ENOSYS)
        {
            printk("UART device does not support interrupt-driven API\n");
        }
        else
        {
            printk("Error setting UART callback: %d\n", ret);
        }

        return;
    }

    uart_irq_rx_enable(gps_dev);
}

void gps_terminate()
{
    gps_disable();
}

void gps_enable()
{
    pmu_setGPSPower(true);
}

void gps_disable()
{
    pmu_setGPSPower(false);
}

bool gps_detect(uint16_t timeout)
{
    return true;
}

int gps_getNmeaSentence(char *buf, const size_t maxLength)
{
    k_msgq_get(&gps_uart_msgq, buf, K_FOREVER);
    return 0;
}

bool gps_nmeaSentenceReady()
{
    return k_msgq_num_used_get(&gps_uart_msgq) != 0;
}

void gps_waitForNmeaSentence()
{
    while (k_msgq_num_used_get(&gps_uart_msgq) != 0)
    {
        sleepFor(0, 100);
    }
}

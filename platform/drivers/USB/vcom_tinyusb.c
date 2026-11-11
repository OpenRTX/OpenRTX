/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include "core/threads.h"
#include "interfaces/delays.h"
#include "interfaces/vcom.h"
#include "tusb.h"
#include "usb.h"

/*
 * Virtual serial port on TinyUSB's CDC-ACM class, for any MCU whose USB
 * controller usb_init() brings up. A low-priority thread runs the stack;
 * writers and readers share it under one lock, since TinyUSB's device
 * stack is not reentrant.
 */

/** How often the USB thread runs the stack, in milliseconds. */
#define USB_TASK_PERIOD_MS 1

/** How long a write waits for the host to make room, in milliseconds. */
#define WRITE_TIMEOUT_MS 50

static pthread_mutex_t usbMutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * The last write timed out: the host has the port open but is not reading.
 * Writes then give up as soon as the FIFO is full, until it drains.
 */
static bool txStalled;

/**
 * Whether a host has the port open on a running bus. Without VBUS sensing,
 * an unplugged cable shows as a suspended bus; with the port closed (DTR
 * low) TinyUSB would discard the data anyway.
 */
static bool hostReading(void)
{
    return tud_mounted() && !tud_suspended() && tud_cdc_connected();
}

/**
 * Tell the host the line has carrier (DCD and DSR), as a modem would.
 */
static void reportCarrier(void)
{
    const cdc_notify_uart_state_t state = {
        .dcd = 1,
        .dsr = 1,
    };

    tud_cdc_notify_uart_state(&state);
}

static void *usbThread(void *arg)
{
    (void)arg;

    for (;;) {
        pthread_mutex_lock(&usbMutex);
        tud_task();
        // Send what a write left in the FIFO while the endpoint was busy
        tud_cdc_write_flush();
        pthread_mutex_unlock(&usbMutex);

        sleepFor(0, USB_TASK_PERIOD_MS);
    }

    return NULL;
}

int vcom_init()
{
    pthread_attr_t attr;
    pthread_t thread;
    struct sched_param param = {
        .sched_priority = THREAD_PRIO_LOW,
    };
    int ret;

    ret = usb_init();
    if (ret < 0)
        return ret;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, USB_THREAD_STKSIZE);
    pthread_attr_setschedparam(&attr, &param);
    ret = pthread_create(&thread, &attr, usbThread, NULL);
    pthread_attr_destroy(&attr);

    return -ret;
}

ssize_t vcom_writeBlock(const void *buf, size_t len)
{
    const uint8_t *data = buf;
    ssize_t ret = (ssize_t)len;
    size_t sent = 0;
    uint32_t waited = 0;

    pthread_mutex_lock(&usbMutex);

    while (sent < len) {
        if (!hostReading()) {
            ret = -ENOTCONN;
            break;
        }

        const uint32_t accepted = tud_cdc_write(data + sent, len - sent);
        tud_cdc_write_flush();
        if (accepted > 0) {
            sent += accepted;
            waited = 0;
            continue;
        }

        // The FIFO is full: let the USB thread send it, unless the host
        // has stopped reading
        if (txStalled || (waited >= WRITE_TIMEOUT_MS)) {
            ret = -ETIMEDOUT;
            break;
        }

        pthread_mutex_unlock(&usbMutex);
        sleepFor(0, 1);
        waited++;
        pthread_mutex_lock(&usbMutex);
    }

    txStalled = (ret == -ETIMEDOUT);
    pthread_mutex_unlock(&usbMutex);

    return ret;
}

ssize_t vcom_readBlock(void *buf, size_t len)
{
    pthread_mutex_lock(&usbMutex);
    const uint32_t count = tud_cdc_read(buf, len);
    pthread_mutex_unlock(&usbMutex);

    return (ssize_t)count;
}

/*
 * TinyUSB callbacks, run by tud_task() on the USB thread with the lock held.
 * The host reads the modem status on enumeration and when it opens the port.
 */

void tud_mount_cb(void)
{
    reportCarrier();
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void)itf;
    (void)rts;

    if (dtr)
        reportCarrier();
}

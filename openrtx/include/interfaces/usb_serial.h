/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef USB_SERIAL_H
#define USB_SERIAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise the USB serial driver and bring up the USB device stack.
 */
void usb_serial_init(void);

/**
 * Shut down the USB serial driver.
 */
void usb_serial_terminate(void);

/**
 * Service the USB stack and flush any pending transmit data.
 * Must be called regularly from exactly one thread (the main thread).
 */
void usb_serial_task(void);

/**
 * \return true if a host is connected and the CDC port is open.
 */
bool usb_serial_connected(void);

/**
 * \return number of bytes waiting in the receive FIFO.
 */
uint32_t usb_serial_available(void);

/**
 * Write data to the USB CDC port.
 *
 * Non-blocking and thread-safe: data is accepted into an internal transmit
 * buffer and returned immediately.  usb_serial_task() flushes the buffer
 * to the CDC FIFO on its next call.
 *
 * A short write (return value < len) indicates the transmit buffer is full;
 * the caller is responsible for retrying the unsent remainder if needed.
 *
 * \param buf  data to send.
 * \param len  number of bytes to send.
 * \return number of bytes accepted, or -1 if not mounted.
 */
ssize_t usb_serial_write(const void *buf, size_t len);

/**
 * Read data from the USB CDC port (non-blocking).
 *
 * \param buf  destination buffer.
 * \param len  maximum number of bytes to read.
 * \return number of bytes read, 0 if none available, or -1 if not connected.
 */
ssize_t usb_serial_read(void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* USB_SERIAL_H */

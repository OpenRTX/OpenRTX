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
 * Blocking: execution does not return until all bytes have been written
 * through the CDC TX FIFO and flushed to the USB endpoint.
 *
 * Returns -1 immediately (without writing any data) if the USB device is
 * not mounted, avoiding an indefinite block when the cable is unplugged.
 * Also returns -1 if no forward progress is made within ~50 ms (transfer
 * stalled while mounted), clearing the TX FIFO before returning.
 *
 * Thread-safe: concurrent callers are serialised by an internal mutex.
 * Do not call this function from within a tinyUSB callback, as it may
 * call tud_task() internally when the CDC TX FIFO is full.
 *
 * \param buf  data to send.
 * \param len  number of bytes to send.
 * \return len on success, or -1 on error.
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

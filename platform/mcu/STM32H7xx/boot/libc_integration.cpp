/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <pthread.h>
#include <stdio.h>
#include <reent.h>
#include "filesystem/file_access.h"
#include "hwconfig.h"
#include "interfaces/usb_serial.h"

#ifdef CONFIG_USB_SERIAL
static pthread_mutex_t stdio_usb_mutex = PTHREAD_MUTEX_INITIALIZER;
/*
 * If the previous _write_r ended by sending a lone '\r' (e.g. printf split
 * "foo\r" and "\n" across two writes), the next write must send only '\n' to
 * complete CRLF — not '\r\n' again — or the wire sees '\r\r\n' (staircase).
 */
static bool usb_out_pending_cr;
#endif

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \internal
 * _write_r, write to a file
 */
int _write_r(struct _reent *ptr, int fd, const void *buf, size_t cnt)
{
#ifdef CONFIG_USB_SERIAL
    if(fd == STDOUT_FILENO || fd == STDERR_FILENO)
    {
        pthread_mutex_lock(&stdio_usb_mutex);
        /*
         * Best-effort lossy console output.
         *
         * This function always returns cnt regardless of how many bytes
         * usb_serial_write() actually sent.  This is intentional: the USB
         * serial port is a debug console, not a reliable byte stream.
         * Propagating partial counts would cause stdio to retry, which can
         * stall the caller when the CDC TX FIFO is full or the device is not
         * yet mounted.  Callers that require reliable delivery must not use
         * stdio for that purpose.
         *
         * Before usb_serial_init() completes, usb_mounted is false and
         * usb_serial_write() returns -1 immediately; output is silently
         * discarded until the USB device is enumerated.
         *
         * Translate \n -> \r\n so output renders correctly in serial
         * terminals.
         * The translation is done here in the stdio layer,
         * not in the driver, so the driver can be used for binary
         * protocols (TNC, NMEA passthrough) without corruption.
         *
         * Note: the Linux host TTY (/dev/ttyACMx) has ICRNL enabled by
         * default, which translates incoming \r (0x0D) to \n (0x0A) before
         * the data reaches the application.  This converts our \r\n into
         * \n\n, causing double-spacing and loss of carriage return in
         * terminal emulators. minicom and HTerm work fine out of the box.
         * picocom should work with --imap lfcrlf
         */
        const char *p    = (const char *) buf;
        size_t      left = cnt;
        size_t      i    = 0;

        while(i < left)
        {
            if(usb_out_pending_cr)
            {
                if(p[i] == '\n')
                {
                    usb_out_pending_cr = false;
                    i++;
                    pthread_mutex_unlock(&stdio_usb_mutex);
                    usb_serial_write("\n", 1);
                    pthread_mutex_lock(&stdio_usb_mutex);
                    continue;
                }
                usb_out_pending_cr = false;
            }

            size_t chunk = left - i;
            if(chunk > 512)
                chunk = 512;

            char   out[1024];
            size_t o = 0;
            for(size_t j = 0; j < chunk; j++)
            {
                char c = p[i + j];
                if(c == '\n')
                {
                    if(o > 0 && out[o - 1] == '\r')
                        out[o++] = '\n';
                    else
                    {
                        out[o++] = '\r';
                        out[o++] = '\n';
                    }
                }
                else
                {
                    out[o++] = c;
                }
            }
            if(o > 0 && out[o - 1] == '\r')
                usb_out_pending_cr = true;
            else
                usb_out_pending_cr = false;

            i += chunk;
            /*
             * Do not hold stdio_usb_mutex across usb_serial_write(): that path
             * can block ~50 ms retrying CDC TX while pumping would otherwise be
             * starved and other threads could not use stdio.
             */
            pthread_mutex_unlock(&stdio_usb_mutex);
            usb_serial_write(out, o);
            pthread_mutex_lock(&stdio_usb_mutex);
        }

        pthread_mutex_unlock(&stdio_usb_mutex);
        return (int)cnt;
    }
#endif

    (void) ptr;

    /* If fd is not stdout or stderr */
    ptr->_errno = EBADF;
    return -1;
}

/**
 * \internal
 * _read_r, read from a file.
 */
int _read_r(struct _reent *ptr, int fd, void *buf, size_t cnt)
{
    (void) ptr;
    (void) fd;
    (void) buf;
    (void) cnt;

    /* If fd is not stdin */
    ptr->_errno = EBADF;
    return -1;
}

#ifdef __cplusplus
}
#endif

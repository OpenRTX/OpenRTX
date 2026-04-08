/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <reent.h>
#include "filesystem/file_access.h"
#include "hwconfig.h"
#include "interfaces/usb_serial.h"

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
        /*
         * Best-effort lossy console output.
         *
         * This function always returns cnt regardless of how many bytes
         * usb_serial_write() actually sent.  This is intentional: the USB
         * serial port is a debug console, not a reliable byte stream.
         * Propagating partial counts would cause stdio to retry, which can
         * stall the caller when the ring buffer is full or the device is not
         * yet mounted.  Callers that require reliable delivery must not use
         * stdio for that purpose.
         *
         * Before usb_serial_init() completes, usb_mounted is false and
         * usb_serial_write() returns -1 immediately; output is silently
         * discarded until the USB device is enumerated.
         *
         * Translate \n -> \r\n so output renders correctly in serial
         * terminals (CDC ACM presents as a raw byte stream with no tty
         * processing).  The translation is done here in the stdio layer,
         * not in the driver, so the driver can be used for binary
         * protocols (TNC, NMEA passthrough) without corruption.
         */
        const char *p   = (const char *) buf;
        size_t      rem = cnt;
        while(rem > 0)
        {
            size_t chunk = 0;
            while(chunk < rem && p[chunk] != '\n')
                chunk++;

            if(chunk > 0)
                usb_serial_write(p, chunk);

            if(chunk < rem)
            {
                usb_serial_write("\r\n", 2);
                chunk++;
            }

            p   += chunk;
            rem -= chunk;
        }
        return cnt;
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

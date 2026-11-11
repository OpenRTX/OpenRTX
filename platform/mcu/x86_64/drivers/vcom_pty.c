/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "interfaces/vcom.h"

/*
 * The emulator's virtual serial port: a pseudo-terminal, whose device path
 * is printed at start-up for host software to open. Writes never wait:
 * with nobody reading, they fail once the terminal's buffer is full.
 */

static int ptyFd = -1;

int vcom_init()
{
    ptyFd = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (ptyFd < 0)
        return -errno;

    // The program that opens the terminal sets its line mode
    if ((grantpt(ptyFd) != 0) || (unlockpt(ptyFd) != 0)) {
        const int err = errno;

        close(ptyFd);
        ptyFd = -1;
        return -err;
    }

    printf("Virtual serial port: %s\n", ptsname(ptyFd));

    return 0;
}

ssize_t vcom_writeBlock(const void *buf, size_t len)
{
    const ssize_t ret = write(ptyFd, buf, len);

    return (ret < 0) ? -errno : ret;
}

ssize_t vcom_readBlock(void *buf, size_t len)
{
    const ssize_t ret = read(ptyFd, buf, len);

    if (ret >= 0)
        return ret;

    return ((errno == EAGAIN) || (errno == EIO)) ? 0 : -errno;
}

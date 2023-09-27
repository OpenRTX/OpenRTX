/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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

#define _XOPEN_SOURCE 600

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include "pty.h"

static int pty_init(const struct chardev *dev)
{
    int ptyFd = posix_openpt(O_RDWR);
    if(ptyFd < 0)
        return errno;

    int rc = grantpt(ptyFd);
    if (rc != 0)
        return errno;

    rc = unlockpt(ptyFd);
    if (rc != 0)
        return errno;

    printf("Successfully open pseudoTTY on %s\n", ptsname(ptyFd));

    *((int *)dev->data) = ptyFd;

    return 0;
}

static int pty_terminate(const struct chardev *dev)
{
    int ptyFd = *((int *)dev->data);
    if(ptyFd < 0)
        return -ENOENT;

    close(ptyFd);
    *((int *)dev->data) = -1;

    return 0;
}


static ssize_t pty_read(const struct chardev *dev, void *data, const size_t len)
{
    int ptyFd = *((int *)dev->data);

    if(ptyFd < 0)
        return -ENOENT;

    return read(ptyFd, data, len);
}

static ssize_t pty_write(const struct chardev *dev, const void *data,
                         const size_t len)
{
    int ptyFd = *((int *)dev->data);

    if(ptyFd < 0)
        return -ENOENT;

    return write(ptyFd, data, len);
}

static int pty_ioctl(const struct chardev *dev, const int cmd, const void *arg)
{
    (void) dev;
    (void) cmd;
    (void) arg;

    return 0;
}

const struct chardev_api pty_chardev_api =
{
    .init      = &pty_init,
    .terminate = &pty_terminate,
    .read      = &pty_read,
    .write     = &pty_write,
    .ioctl     = &pty_ioctl
};

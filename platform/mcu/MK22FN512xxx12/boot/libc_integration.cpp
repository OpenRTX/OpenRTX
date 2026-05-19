/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <reent.h>
#include "drivers/usb_vcom.h"
#include "filesystem/file_access.h"

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
    #ifdef ENABLE_STDIO
    if(fd == STDOUT_FILENO || fd == STDERR_FILENO)
    {
        return vcom_writeBlock(buf, cnt);
    }
    #else
    (void) ptr;
    (void) fd;
    (void) buf;
    (void) cnt;
    #endif

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
    int ret = -1;

    #ifdef ENABLE_STDIO
    if(fd == STDIN_FILENO)
    {
        for(;;)
        {
            ssize_t r = vcom_readBlock(buf, cnt);
            if((r < 0) || (r == ((ssize_t) cnt)))
            {
                ret = ((int) r);
                break;
            }
        }
    }
    else
    {
        /* If fd is not stdin */
        ptr->_errno = EBADF;
        ret = -1;
    }
    #else
    (void) ptr;
    (void) fd;
    (void) buf;
    (void) cnt;
    ptr->_errno = EBADF;
    #endif

    return ret;
}

#ifdef __cplusplus
}
#endif

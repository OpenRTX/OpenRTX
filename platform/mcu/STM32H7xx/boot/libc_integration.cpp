/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <reent.h>
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
    (void) ptr;
    (void) fd;
    (void) buf;
    (void) cnt;

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

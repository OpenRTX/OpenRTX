/***************************************************************************
 *   Copyright (C) 2020, 2021 by Silvano Seva IU2KWO                       *
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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <stdio.h>
#include <reent.h>
#include <usb_vcom.h>
#include <filesystem/file_access.h>

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
    if(fd == STDOUT_FILENO || fd == STDERR_FILENO)
    {
        return vcom_writeBlock(buf, cnt);
    }

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

    return ret;
}

#ifdef __cplusplus
}
#endif

/***************************************************************************
 *   Copyright (C) 2020 by Silvano Seva IU2KWO                             *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <reent.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "../drivers/usb_vcom.h"


void pthread_mutex_unlock(){}
void pthread_mutex_lock() {}
void pthread_mutex_destroy() {}
int pthread_setcancelstate(int state, int *oldstate)
{
    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

//
// C atexit support, for thread safety and code size optimizations
// ===============================================================

/**
 * Function called by atexit(), on_exit() and __cxa_atexit() to register
 * C functions/C++ destructors to be run at program termintion.
 * It is called in this way:
 * atexit():       __register_exitproc(__et_atexit, fn, 0,   0)
 * on_exit():      __register_exitproc(__et_onexit, fn, arg, 0)
 * __cxa_atexit(): __register_exitproc(__et_cxa,    fn, arg, d)
 * \param type to understand if the function was called by atexit, on_exit, ...
 * \param fn pointer to function to be called
 * \param arg 0 in case of atexit, function argument in case of on_exit,
 * "this" parameter for C++ destructors registered with __cxa_atexit
 * \param d __dso_handle used to selectively call C++ destructors of a shared
 * library loaded dynamically, unused since Miosix does not support shared libs
 * \return 0 on success
 */
int __register_exitproc(int type, void (*fn)(void), void *arg, void *d)
{
    return 0;
}

/**
 * Called by exit() to call functions registered through atexit()
 * \param code the exit code, for example with exit(1), code==1
 * \param d __dso_handle, see __register_exitproc
 */
void __call_exitprocs(int code, void *d) {}

/**
 * \internal
 * Required by C++ standard library.
 * See http://lists.debian.org/debian-gcc/2003/07/msg00057.html
 */
void *__dso_handle=(void*) &__dso_handle;




//
// C/C++ system calls, to support malloc, printf, fopen, etc.
// ==========================================================

/**
 * \internal
 * _exit, lock system in infinite loop until reboot
 */
void _exit(int status)
{
	for(;;) ;
}

/**
 * \internal
 * _sbrk_r, allocates memory dynamically
 */
void *_sbrk_r(struct _reent *ptr, ptrdiff_t incr)
{
    //This is the absolute start of the heap
    extern char _end asm("_end"); //defined in the linker script
    //This is the absolute end of the heap
    extern char _heap_end asm("_heap_end"); //defined in the linker script
    //This holds the current end of the heap (static)
    static char *curHeapEnd=NULL;
    //This holds the previous end of the heap
    char *prevHeapEnd;

    //Check if it's first time called
    if(curHeapEnd==NULL) curHeapEnd=&_end;

    prevHeapEnd=curHeapEnd;

    if((curHeapEnd+incr)>&_heap_end)
    {
        return (void*)(-1);
    }
    curHeapEnd+=incr;

    return (void*)(prevHeapEnd);
}

/**
 * \internal
 * __malloc_lock, called by malloc to ensure no context switch happens during
 * memory allocation. Since this environment is not a multithreaded one, this
 * function is left empty. Allocating memory inside interrupts is anyway
 * forbidden.
 */
void __malloc_lock() {}

/**
 * \internal
 * __malloc_unlock, called by malloc after performing operations on the heap
 */
void __malloc_unlock() {}

/**
 * \internal
 * __getreent(), return the reentrancy structure of the current thread.
 * Used by newlib to make the C standard library thread safe.
 * Not a multithreaded environment, we return global reentrancy data.
 */
struct _reent *__getreent()
{
    return _GLOBAL_REENT;
}




/**
 * \internal
 * _open_r, open a file. Actually unimpemented
 */
int _open_r(struct _reent *ptr, const char *name, int flags, int mode)
{
    return -1;
}

/**
 * \internal
 * _close_r, close a file. Actually unimpemented
 */
int _close_r(struct _reent *ptr, int fd)
{
    return -1;
}

/**
 * \internal
 * _write_r, write to a file.
 */
int _write_r(struct _reent *ptr, int fd, const void *buf, size_t cnt)
{
    if(fd == STDOUT_FILENO || fd == STDERR_FILENO)
    {
        vcom_writeBlock(buf, cnt);
        return cnt;
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
    if(fd == STDIN_FILENO)
    {
        for(;;)
        {
            ssize_t r = vcom_readBlock(buf, cnt);
            if((r < 0) || (r == cnt)) return r;
        }
    }
    else

    /* If fd is not stdin */
    ptr->_errno = EBADF;
    return -1;
}

int _read(int fd, void *buf, size_t cnt)
{
    return _read_r(__getreent(), fd, buf, cnt);
}

/**
 * \internal
 * _lseek_r, move file pointer. Actually unimpemented
 */
off_t _lseek_r(struct _reent *ptr, int fd, off_t pos, int whence)
{
    return -1;
}

off_t _lseek(int fd, off_t pos, int whence)
{
    return -1;
}

/**
 * \internal
 * _fstat_r, return file info. Actually unimpemented
 */
int _fstat_r(struct _reent *ptr, int fd, struct stat *pstat)
{
    return -1;
}

int _fstat(int fd, struct stat *pstat)
{
    return -1;
}

/**
 * \internal
 * _stat_r, collect data about a file. Actually unimpemented
 */
int _stat_r(struct _reent *ptr, const char *file, struct stat *pstat)
{
    return -1;
}

/**
 * \internal
 * isatty, returns 1 if fd is associated with a terminal.
 * Always return 1 because read and write are implemented only in 
 * terms of serial communication
 */
int _isatty_r(struct _reent *ptr, int fd)
{
    return 1;
}

int isatty(int fd)
{
    return 1;
}

int _isatty(int fd)
{
    return 1;
}

/**
 * \internal
 * _mkdir, create a directory. Actually unimpemented
 */
int mkdir(const char *path, mode_t mode)
{
    return -1;
}

/**
 * \internal
 * _link_r: create hardlinks. Actually unimpemented
 */
int _link_r(struct _reent *ptr, const char *f_old, const char *f_new)
{
    return -1;
}

/**
 * \internal
 * _unlink_r, remove a file. Actually unimpemented
 */
int _unlink_r(struct _reent *ptr, const char *file)
{
    return -1;
}

/**
 * \internal
 * _times_r, return elapsed time. Actually unimpemented
 */
clock_t _times_r(struct _reent *ptr, struct tms *tim)
{
    return -1;
}





/**
 * \internal
 * it looks like abort() calls _kill instead of exit, this implementation
 * calls _exit() so that calling abort() really terminates the program
 */
int _kill_r(struct _reent* ptr, int pid, int sig)
{
    if(pid == 0)
        _exit(1);
    else
        return -1;
}

int _kill(int pid, int sig)
{
    _kill_r(0, pid, sig);
}

/**
 * \internal
 * _getpid_r. Not a multiprocess system, return always 0
 */
int _getpid_r(struct _reent* ptr)
{
    return 0;
}

int _getpid()
{
    return 0;
}

/**
 * \internal
 * _wait_r, unimpemented because processes are not supported.
 */
int _wait_r(struct _reent *ptr, int *status)
{
    return -1;
}

int _fork_r(struct _reent *ptr)
{
    return -1;
}

#ifdef __cplusplus
}
#endif

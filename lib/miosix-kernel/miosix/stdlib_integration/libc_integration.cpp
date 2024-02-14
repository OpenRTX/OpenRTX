/***************************************************************************
 *   Copyright (C) 2008-2023 by Terraneo Federico                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
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

#include "libc_integration.h"
#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <dirent.h>
#include <reent.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/times.h>
//// Settings
#include "config/miosix_settings.h"
//// Filesystem
#include "filesystem/file_access.h"
//// Console
#include "kernel/logging.h"
//// kernel interface
#include "kernel/kernel.h"
#include "interfaces/bsp.h"
#include "interfaces/os_timer.h"

using namespace std;

namespace miosix {

// This holds the max heap usage since the program started.
// It is written by _sbrk_r and read by getMaxHeap()
static unsigned int maxHeapEnd=0;

unsigned int getMaxHeap()
{
    //If getMaxHeap() is called before the first _sbrk_r() maxHeapEnd is zero.
    extern char _end asm("_end"); //defined in the linker script
    if(maxHeapEnd==0) return reinterpret_cast<unsigned int>(&_end);
    return maxHeapEnd;
}

/**
 * \return the global C reentrancy structure
 */
static struct _reent *kernelNotStartedGetReent() { return _GLOBAL_REENT; }

/**
 * Pointer to a function that retrieves the correct reentrancy structure.
 * When the C reentrancy structure is requested before the kernel is started,
 * the default reentrancy structure shall be returned, while after the kernel
 * is started, the per-thread reentrancy structure needs to be returned to
 * avoid race conditions between threads.
 * The function pointer is needed to switch between the two behaviors as the
 * per-thread code would cause a circular dependency if called before the
 * kernel is started (getCurrentThread needs to allocate a thread with malloc
 * if called before the kernel istarted, and malloc needs the reentrancy
 * structure).
 */
static struct _reent *(*getReent)()=kernelNotStartedGetReent;

void setCReentrancyCallback(struct _reent *(*callback)()) { getReent=callback; }

} //namespace miosix

#ifdef __cplusplus
extern "C" {
#endif

//
// C atexit support, for thread safety and code size optimizations
// ===============================================================

// Prior to Miosix 1.58 atexit was effectively unimplemented, but its partial
// support in newlib used ~384bytes of RAM. Within the kernel it will always
// be unimplemented, so newlib has been patched not to waste RAM.
// The support library for Miosix processes will instead implement those stubs
// so as to support atexit in processes, as in that case it makes sense.

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
 * _exit, restarts the system
 */
void _exit(int n)
{
    miosix::reboot();
    //Never reach here
    for(;;) ; //Required to avoid a warning about noreturn functions
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
        //bad, heap overflow
        #ifdef __NO_EXCEPTIONS
        // When exceptions are disabled operator new would return 0, which would
        // cause undefined behaviour. So when exceptions are disabled, a heap
        // overflow causes a reboot.
        errorLog("\n***Heap overflow\n");
        _exit(1);
        #else //__NO_EXCEPTIONS
        return reinterpret_cast<void*>(-1);
        #endif //__NO_EXCEPTIONS
    }
    curHeapEnd+=incr;

    if(reinterpret_cast<unsigned int>(curHeapEnd) > miosix::maxHeapEnd)
        miosix::maxHeapEnd=reinterpret_cast<unsigned int>(curHeapEnd);
    
    return reinterpret_cast<void*>(prevHeapEnd);
}

void *sbrk(ptrdiff_t incr)
{
    return _sbrk_r(miosix::getReent(),incr);
}

/**
 * \internal
 * __malloc_lock, called by malloc to ensure no context switch happens during
 * memory allocation (the heap is global and shared between the threads, so
 * memory allocation should not be interrupted by a context switch)
 *
 *	WARNING:
 *	pauseKernel() does not stop interrupts, so interrupts may occur
 *	during memory allocation. So NEVER use malloc inside an interrupt!
 *	Also beware that some newlib functions, like printf, iprintf...
 *	do call malloc, so you must not use them inside an interrupt.
 */
void __malloc_lock()
{
    miosix::pauseKernel();
}

/**
 * \internal
 * __malloc_unlock, called by malloc after performing operations on the heap
 */
void __malloc_unlock()
{
    miosix::restartKernel();
}

/**
 * \internal
 * __getreent(), return the reentrancy structure of the current thread.
 * Used by newlib to make the C standard library thread safe
 */
struct _reent *__getreent()
{
    return miosix::getReent();
}




/**
 * \internal
 * _open_r, open a file
 */
int _open_r(struct _reent *ptr, const char *name, int flags, int mode)
{
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        int result=miosix::getFileDescriptorTable().open(name,flags,mode);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        ptr->_errno=ENOMEM;
        return -1;
    }
    #endif //__NO_EXCEPTIONS

    #else //WITH_FILESYSTEM
    ptr->_errno=ENFILE;
    return -1;
    #endif //WITH_FILESYSTEM
}

int open(const char *name, int flags, ...)
{
    int mode=0;
    if(flags & O_CREAT)
    {
        va_list arg;
        va_start(arg,flags);
        mode=va_arg(arg,int);
        va_end(arg);
    }
    return _open_r(miosix::getReent(),name,flags,mode);
}

/**
 * \internal
 * _close_r, close a file
 */
int _close_r(struct _reent *ptr, int fd)
{
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        int result=miosix::getFileDescriptorTable().close(fd);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        ptr->_errno=ENOMEM;
        return -1;
    }
    #endif //__NO_EXCEPTIONS

    #else //WITH_FILESYSTEM
    ptr->_errno=EBADF;
    return -1;
    #endif //WITH_FILESYSTEM
}

int close(int fd)
{
    return _close_r(miosix::getReent(),fd);
}

/**
 * \internal
 * _write_r, write to a file
 */
int _write_r(struct _reent *ptr, int fd, const void *buf, size_t cnt)
{    
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        int result=miosix::getFileDescriptorTable().write(fd,buf,cnt);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        ptr->_errno=ENOMEM;
        return -1;
    }
    #endif //__NO_EXCEPTIONS
    
    #else //WITH_FILESYSTEM
    if(fd==STDOUT_FILENO || fd==STDERR_FILENO)
    {
        int result=miosix::DefaultConsole::instance().getTerminal()->write(buf,cnt);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    } else {
        ptr->_errno=EBADF;
        return -1;
    }
    #endif //WITH_FILESYSTEM
}

int write(int fd, const void *buf, size_t cnt)
{
    return _write_r(miosix::getReent(),fd,buf,cnt);
}

/**
 * \internal
 * _read_r, read from a file
 */
int _read_r(struct _reent *ptr, int fd, void *buf, size_t cnt)
{
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        int result=miosix::getFileDescriptorTable().read(fd,buf,cnt);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        ptr->_errno=ENOMEM;
        return -1;
    }
    #endif //__NO_EXCEPTIONS
    
    #else //WITH_FILESYSTEM
    if(fd==STDIN_FILENO)
    {
        int result=miosix::DefaultConsole::instance().getTerminal()->read(buf,cnt);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    } else {
        ptr->_errno=EBADF;
        return -1;
    }
    #endif //WITH_FILESYSTEM
}

int read(int fd, void *buf, size_t cnt)
{
    return _read_r(miosix::getReent(),fd,buf,cnt);
}

/**
 * \internal
 * _lseek_r, move file pointer
 */
off_t _lseek_r(struct _reent *ptr, int fd, off_t pos, int whence)
{
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        off_t result=miosix::getFileDescriptorTable().lseek(fd,pos,whence);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        ptr->_errno=ENOMEM;
        return -1;
    }
    #endif //__NO_EXCEPTIONS
    
    #else //WITH_FILESYSTEM
    ptr->_errno=EBADF;
    return -1;
    #endif //WITH_FILESYSTEM
}

off_t lseek(int fd, off_t pos, int whence)
{
    return _lseek_r(miosix::getReent(),fd,pos,whence);
}

/**
 * \internal
 * _fstat_r, return file info
 */
int _fstat_r(struct _reent *ptr, int fd, struct stat *pstat)
{
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        int result=miosix::getFileDescriptorTable().fstat(fd,pstat);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        ptr->_errno=ENOMEM;
        return -1;
    }
    #endif //__NO_EXCEPTIONS
    
    #else //WITH_FILESYSTEM
    switch(fd)
    {
        case STDIN_FILENO:
        case STDOUT_FILENO:
        case STDERR_FILENO:
            memset(pstat,0,sizeof(struct stat));
            pstat->st_mode=S_IFCHR;//Character device
            pstat->st_blksize=0; //Defualt file buffer equals to BUFSIZ
            return 0;
        default:
            ptr->_errno=EBADF;
            return -1;
    }
    #endif //WITH_FILESYSTEM
}

int fstat(int fd, struct stat *pstat)
{
    return _fstat_r(miosix::getReent(),fd,pstat);
}

/**
 * \internal
 * _stat_r, collect data about a file
 */
int _stat_r(struct _reent *ptr, const char *file, struct stat *pstat)
{
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        int result=miosix::getFileDescriptorTable().stat(file,pstat);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        ptr->_errno=ENOMEM;
        return -1;
    }
    #endif //__NO_EXCEPTIONS
    
    #else //WITH_FILESYSTEM
    ptr->_errno=ENOENT;
    return -1;
    #endif //WITH_FILESYSTEM
}

int stat(const char *file, struct stat *pstat)
{
    return _stat_r(miosix::getReent(),file,pstat);
}

/**
 * \internal
 * isatty, returns 1 if fd is associated with a terminal
 */
int _isatty_r(struct _reent *ptr, int fd)
{
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        int result=miosix::getFileDescriptorTable().isatty(fd);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        ptr->_errno=ENOMEM;
        return -1;
    }
    #endif //__NO_EXCEPTIONS
    
    #else //WITH_FILESYSTEM
    switch(fd)
    {
        case STDIN_FILENO:
        case STDOUT_FILENO:
        case STDERR_FILENO:
            return 1;
        default:
            return 0;
    }
    #endif //WITH_FILESYSTEM
}

int isatty(int fd)
{
    return _isatty_r(miosix::getReent(),fd);
}

/**
 * \internal
 * _fntl_r, perform operations on a file descriptor
 */
int _fcntl_r(struct _reent *ptr, int fd, int cmd, int opt)
{
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        int result=miosix::getFileDescriptorTable().fcntl(fd,cmd,opt);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        ptr->_errno=ENOMEM;
        return -1;
    }
    #endif //__NO_EXCEPTIONS
    
    #else //WITH_FILESYSTEM
    ptr->_errno=ENOENT;
    return -1;
    #endif //WITH_FILESYSTEM
}

int fcntl(int fd, int cmd, ...)
{
    va_list arg;
    int result;
    struct _reent *r=miosix::getReent();
    switch(cmd)
    {
        case F_DUPFD:
        case F_SETFD:
        case F_SETFL:
            va_start(arg,cmd);
            result=_fcntl_r(r,fd,cmd,va_arg(arg,int));
            va_end(arg);
        default:
            result=_fcntl_r(r,fd,cmd,0);
    }
    return result;
}

/**
 * \internal
 * _ioctl_r, perform operations on a file descriptor
 */
int _ioctl_r(struct _reent *ptr, int fd, int cmd, void *arg)
{
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        int result=miosix::getFileDescriptorTable().ioctl(fd,cmd,arg);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        ptr->_errno=ENOMEM;
        return -1;
    }
    #endif //__NO_EXCEPTIONS
    
    #else //WITH_FILESYSTEM
    if(fd==STDIN_FILENO || fd==STDOUT_FILENO || fd==STDERR_FILENO)
    {
        int result=miosix::DefaultConsole::instance().getTerminal()->ioctl(cmd,arg);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    } else {
        ptr->_errno=ENOENT;
        return -1;
    }
    #endif //WITH_FILESYSTEM
}

int ioctl(int fd, int cmd, void *arg)
{
    return _ioctl_r(miosix::getReent(),fd,cmd,arg);
}

/**
 * \internal
 * _getcwd_r, return current directory
 */
char *_getcwd_r(struct _reent *ptr, char *buf, size_t size)
{
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        int result=miosix::getFileDescriptorTable().getcwd(buf,size);
        if(result>=0) return buf;
        ptr->_errno=-result;
        return NULL;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        ptr->_errno=ENOMEM;
        return NULL;
    }
    #endif //__NO_EXCEPTIONS
    
    #else //WITH_FILESYSTEM
    ptr->_errno=ENOENT;
    return NULL;
    #endif //WITH_FILESYSTEM
}

char *getcwd(char *buf, size_t size)
{
    return _getcwd_r(miosix::getReent(),buf,size);
}

/**
 * \internal
 * _chdir_r, change current directory
 */
int _chdir_r(struct _reent *ptr, const char *path)
{
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        int result=miosix::getFileDescriptorTable().chdir(path);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        ptr->_errno=ENOMEM;
        return -1;
    }
    #endif //__NO_EXCEPTIONS
    
    #else //WITH_FILESYSTEM
    ptr->_errno=ENOENT;
    return -1;
    #endif //WITH_FILESYSTEM
}

int chdir(const char *path)
{
    return _chdir_r(miosix::getReent(),path);
}

/**
 * \internal
 * _mkdir_r, create a directory
 */
int _mkdir_r(struct _reent *ptr, const char *path, int mode)
{
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        int result=miosix::getFileDescriptorTable().mkdir(path,mode);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        ptr->_errno=ENOMEM;
        return -1;
    }
    #endif //__NO_EXCEPTIONS
    
    #else //WITH_FILESYSTEM
    ptr->_errno=ENOENT;
    return -1;
    #endif //WITH_FILESYSTEM
}

int mkdir(const char *path, mode_t mode)
{
    return _mkdir_r(miosix::getReent(),path,mode);
}

/**
 * \internal
 * _rmdir_r, remove a directory if empty
 */
int _rmdir_r(struct _reent *ptr, const char *path)
{
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        int result=miosix::getFileDescriptorTable().rmdir(path);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        ptr->_errno=ENOMEM;
        return -1;
    }
    #endif //__NO_EXCEPTIONS
    
    #else //WITH_FILESYSTEM
    ptr->_errno=ENOENT;
    return -1;
    #endif //WITH_FILESYSTEM
}

int rmdir(const char *path)
{
    return _rmdir_r(miosix::getReent(),path);
}

/**
 * \internal
 * _link_r: create hardlinks
 */
int _link_r(struct _reent *ptr, const char *f_old, const char *f_new)
{
    ptr->_errno=ENOENT; //Unimplemented at the moment
    return -1;
}

int link(const char *f_old, const char *f_new)
{
    return _link_r(miosix::getReent(),f_old,f_new);
}

/**
 * \internal
 * _unlink_r, remove a file
 */
int _unlink_r(struct _reent *ptr, const char *file)
{
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        int result=miosix::getFileDescriptorTable().unlink(file);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        ptr->_errno=ENOMEM;
        return -1;
    }
    #endif //__NO_EXCEPTIONS
    
    #else //WITH_FILESYSTEM
    ptr->_errno=ENOENT;
    return -1;
    #endif //WITH_FILESYSTEM
}

int unlink(const char *file)
{
    return _unlink_r(miosix::getReent(),file);
}

/**
 * \internal
 * _rename_r, rename a file or directory
 */
int _rename_r(struct _reent *ptr, const char *f_old, const char *f_new)
{
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        int result=miosix::getFileDescriptorTable().rename(f_old,f_new);
        if(result>=0) return result;
        ptr->_errno=-result;
        return -1;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        ptr->_errno=ENOMEM;
        return -1;
    }
    #endif //__NO_EXCEPTIONS
    
    #else //WITH_FILESYSTEM
    ptr->_errno=ENOENT;
    return -1;
    #endif //WITH_FILESYSTEM
}

int rename(const char *f_old, const char *f_new)
{
    return _rename_r(miosix::getReent(),f_old,f_new);
}

/**
 * \internal
 * getdents, allows to list the content of a directory
 */
int getdents(unsigned int fd, struct dirent *dirp, unsigned int count)
{
    #ifdef WITH_FILESYSTEM

    #ifndef __NO_EXCEPTIONS
    try {
    #endif //__NO_EXCEPTIONS
        int result=miosix::getFileDescriptorTable().getdents(fd,dirp,count);
        if(result>=0) return result;
        miosix::getReent()->_errno=-result;
        return -1;
    #ifndef __NO_EXCEPTIONS
    } catch(exception& e) {
        miosix::getReent()->_errno=ENOMEM;
        return -1;
    }
    #endif //__NO_EXCEPTIONS
    
    #else //WITH_FILESYSTEM
    miosix::getReent()->_errno=ENOENT;
    return -1;
    #endif //WITH_FILESYSTEM
}




/*
 * Time API in Miosix
 * ==================
 *
 * CORE
 * - clock_gettime
 * - clock_nanosleep
 * - clock_settime
 * - clock_getres
 *
 * DERIVED, preferred
 * - C++11 chrono system|steady_clock -> clock_gettime
 * - C++11 sleep_for|until            -> clock_nanosleep
 *
 * DERIVED, for compatibility (replace with core functions when possible)
 * - sleep|usleep -> nanosleep        -> clock_nanosleep
 * - clock        -> times            -> clock_gettime
 * - time         -> gettimeofday     -> clock_gettime
 *
 * UNSUPPORTED
 * - timer_create -> ? 
 */

int clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    if(tp==nullptr) return -1;
    //TODO: support CLOCK_REALTIME
    miosix::ll2timespec(miosix::getTime(),tp);
    return 0;
}

int clock_settime(clockid_t clock_id, const struct timespec *tp)
{
    //TODO: support CLOCK_REALTIME
    return -1;
}

int clock_getres(clockid_t clock_id, struct timespec *res)
{
    if(res==nullptr) return -1;
    //TODO: support CLOCK_REALTIME

    //Integer division with round-to-nearest for better accuracy
    int resolution=2*miosix::nsPerSec/miosix::internal::osTimerGetFrequency();
    resolution=(resolution & 1) ? resolution/2+1 : resolution/2;

    res->tv_sec=0;
    res->tv_nsec=resolution;
    return 0;
}

int clock_nanosleep(clockid_t clock_id, int flags,
                    const struct timespec *req, struct timespec *rem)
{
    if(req==nullptr) return -1;
    //TODO: support CLOCK_REALTIME
    long long timeNs=miosix::timespec2ll(req);
    if(flags!=TIMER_ABSTIME) timeNs+=miosix::getTime();
    miosix::Thread::nanoSleepUntil(timeNs);
    return 0;
}

/**
 * \internal
 * _times_r, return elapsed time
 */
clock_t _times_r(struct _reent *ptr, struct tms *tim)
{
    struct timespec tp;
    //No CLOCK_PROCESS_CPUTIME_ID support, use CLOCK_MONOTONIC
    if(clock_gettime(CLOCK_MONOTONIC,&tp)) return static_cast<clock_t>(-1);
    constexpr int divFactor=1000000000/CLOCKS_PER_SEC;
    clock_t utime=tp.tv_sec*CLOCKS_PER_SEC + tp.tv_nsec/divFactor;
    
    //Actually, we should return tim.utime or -1 on failure, but clock_t is
    //unsigned, so if we return tim.utime and someone calls _times_r in an
    //unlucky moment where tim.utime is 0xffffffff it would be interpreted as -1
    //IMHO, the specifications are wrong since returning an unsigned leaves
    //no value left to return in case of errors. Thus 0 is returned if a valid
    //pointer is passed, and tim.utime if the pointer is null
    if(tim==nullptr) return utime;
    tim->tms_utime=utime;
    tim->tms_stime=0;
    tim->tms_cutime=0;
    tim->tms_cstime=0;
    return 0;
}

clock_t times(struct tms *tim)
{
    return _times_r(miosix::getReent(),tim);
}

int _gettimeofday_r(struct _reent *ptr, struct timeval *tv, void *tz)
{
    if(tv==nullptr || tz!=nullptr) return -1;
    struct timespec tp;
    if(clock_gettime(CLOCK_REALTIME,&tp)) return -1;
    tv->tv_sec=tp.tv_sec;
    tv->tv_usec=tp.tv_nsec/1000;
    return 0;
}

int gettimeofday(struct timeval *tv, void *tz)
{
    return _gettimeofday_r(miosix::getReent(),tv,tz);
}


int nanosleep(const struct timespec *req, struct timespec *rem)
{
    return clock_nanosleep(CLOCK_MONOTONIC,0,req,rem);
}




/**
 * \internal
 * it looks like abort() calls _kill instead of exit, this implementation
 * calls _exit() so that calling abort() really terminates the program
 */
int _kill_r(struct _reent* ptr, int pid, int sig)
{
    if(pid==0) _exit(1); //pid=1 means the only running process
    else return -1;
}

int kill(int pid, int sig)
{
    return _kill_r(miosix::getReent(),pid,sig);
}

/**
 * \internal
 * _getpid_r, there is only one process in Miosix (but multiple threads)
 */
int _getpid_r(struct _reent* ptr)
{
    return 0;
}

/**
 * \internal
 * getpid, there is only one process in Miosix (but multiple threads)
 */
int getpid()
{
    return _getpid_r(miosix::getReent());
}

/**
 * \internal
 * _wait_r, unimpemented because processes are not supported in Miosix
 */
int _wait_r(struct _reent *ptr, int *status)
{
    return -1;
}

int wait(int *status)
{
    return _wait_r(miosix::getReent(),status);
}

/**
 * \internal
 * _execve_r, unimpemented because processes are not supported in Miosix
 */
int _execve_r(struct _reent *ptr, const char *path, char *const argv[],
        char *const env[])
{
    return -1;
}

int execve(const char *path, char *const argv[], char *const env[])
{
    return _execve_r(miosix::getReent(),path,argv,env);
}

/**
 * \internal
 * _forkexecve_r, reserved for future use
 */
pid_t _forkexecve_r(struct _reent *ptr, const char *path, char *const argv[],
        char *const env[])
{
    return -1;
}

pid_t forkexecve(const char *path, char *const argv[], char *const env[])
{
    return _forkexecve_r(miosix::getReent(),path,argv,env);
}

#ifdef __cplusplus
}
#endif




//
// Check that newlib has been configured correctly
// ===============================================

#ifndef _REENT_SMALL
#error "_REENT_SMALL not defined"
#endif //_REENT_SMALL

#ifndef _POSIX_THREADS
#error "_POSIX_THREADS not defined"
#endif //_POSIX_THREADS

#ifndef __DYNAMIC_REENT__
#error "__DYNAMIC_REENT__ not defined"
#endif

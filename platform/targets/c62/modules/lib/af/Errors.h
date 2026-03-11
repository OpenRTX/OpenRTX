
#ifndef AUDIOSERVICE_ERRORS_H
#define AUDIOSERVICE_ERRORS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <sys/types.h>
#include <errno.h>

#define EPERM 1
#define ENOENT 2
#define ENOFILE ENOENT
#define ESRCH 3
#define EINTR 4
#define EIO 5
#define ENXIO 6
#define E2BIG 7
#define ENOEXEC 8
#define EBADF 9
#define ECHILD 10
#define EAGAIN 11
#define ENOMEM 12
#define EACCES 13
#define EFAULT 14
#define EBUSY 16
#define EEXIST 17
#define EXDEV 18
#define ENODEV 19
#define ENOTDIR 20
#define EISDIR 21
#define ENFILE 23
#define EMFILE 24
#define ENOTTY 25
#define EFBIG 27
#define ENOSPC 28
#define ESPIPE 29
#define EROFS 30
#define EMLINK 31
#define EPIPE 32
#define EDOM 33

#define EINVAL 22
#define ERANGE 34
#define STRUNCATE 80

/*
 * Error codes. 
 * All error codes are negative values.
 */
 
enum {
    OK                = 0,    // Everything's swell.
    NO_ERROR          = 0,    // No errors.
    
    UNKNOWN_ERROR       = 0x80000000,

    NO_MEMORY           = -ENOMEM,
    INVALID_OPERATION   = -ENOSYS,
    BAD_VALUE           = -EINVAL,
    BAD_TYPE            = 0x80000001,
    NAME_NOT_FOUND      = -ENOENT,
    PERMISSION_DENIED   = -EPERM,
    NO_INIT             = -ENODEV,
    ALREADY_EXISTS      = -EEXIST,
    DEAD_OBJECT         = -EPIPE,
    FAILED_TRANSACTION  = 0x80000002,
    JPARKS_BROKE_IT     = -EPIPE,
    BAD_INDEX           = -EOVERFLOW,
    NOT_ENOUGH_DATA     = -ENODATA,
    WOULD_BLOCK         = -EWOULDBLOCK, 
    TIMED_OUT           = -ETIMEDOUT,
    UNKNOWN_TRANSACTION = -EBADMSG,
};
    
// ---------------------------------------------------------------------------
    
#endif // AUDIOSERVICE_ERRORS_H

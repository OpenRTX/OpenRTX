/* Minimal <sys/dirent.h> for the Xuantie CSKY bare-metal newlib, whose own
 * sys/dirent.h is an "#error not supported" stub. Provides just enough
 * (struct dirent + opaque DIR) for Miosix's filesystem/file.h to compile in a
 * no-WITH_FILESYSTEM build (the directory API is declared but never used).
 * Placed first on the include path so the real <dirent.h> picks it up. */
#ifndef _CSKY_COMPAT_SYS_DIRENT_H
#define _CSKY_COMPAT_SYS_DIRENT_H

#include <sys/types.h>

struct dirent {
    ino_t          d_ino;
    unsigned char  d_type;
    char           d_name[256];
};

/* Directory-type values referenced by POSIX dirent users. */
#ifndef DT_UNKNOWN
#define DT_UNKNOWN 0
#define DT_REG     8
#define DT_DIR     4
#endif

typedef struct __dirstream DIR;

#endif /* _CSKY_COMPAT_SYS_DIRENT_H */

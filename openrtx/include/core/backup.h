#ifndef BACKUP_H
#define BACKUP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start a dump of the external flash memory content via xmodem transfer,
 * blocking function.
 */
void eflash_dump();

/**
 * Start a restore of the external flash memory content via xmodem transfer,
 * blocking function.
 */
void eflash_restore();

#ifdef __cplusplus
}
#endif


#endif /* BACKUP_H */

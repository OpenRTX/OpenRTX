/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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

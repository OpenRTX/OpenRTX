/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
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

#ifndef THREADS_H
#define THREADS_H

#include <stddef.h>

/**
 * Spawn all the threads for the various functionalities.
 */
void create_threads();

#ifdef __arm__

/**
 * Stack size for UI task, in bytes.
 */
#define UI_TASK_STKSIZE 1024

/**
 * Stack size for Keyboard task, in bytes.
 */
#define KBD_TASK_STKSIZE 512

/**
 * Stack size for state update task, in bytes.
 */
#define DEV_TASK_STKSIZE 512

/**
 * Stack size for baseband control task, in bytes.
 */
#define RTX_TASK_STKSIZE 512

#else /* __arm__ */

#define UI_TASK_STKSIZE 4096

/**
 * Stack size for Keyboard task, in bytes.
 */
#define KBD_TASK_STKSIZE 256

/**
 * Stack size for device update task, in bytes.
 */
#define DEV_TASK_STKSIZE 1024

/**
 * Stack size for baseband control task, in bytes.
 */
#define RTX_TASK_STKSIZE 1024

#endif /* __arm__ */

#endif /* THREADS_H */

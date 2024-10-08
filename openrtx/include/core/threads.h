/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

/**
 * Stack size for state update task, in bytes.
 */
#define DEV_TASK_STKSIZE 2048

/**
 * Stack size for UI management, in bytes.
 */
#define UI_TASK_STKSIZE 2048

/**
 * Stack size for baseband control task, in bytes.
 */
#define RTX_TASK_STKSIZE 512

/**
 * Stack size for codec2 task, in bytes.
 */
#define CODEC2_TASK_STKSIZE 16384

/**
 * Stack size for user functions task, in bytes
 */
#ifdef CONFIG_USER_FUNCTIONS
#define USER_FUNCTIONS_STKSIZE 512 + 128*CONFIG_USER_FUNCTIONS
#endif

#endif /* THREADS_H */

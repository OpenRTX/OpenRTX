/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
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

#ifndef PTY_H
#define PTY_H

#include <interfaces/chardev.h>


/**
 * Set up a linux PTY character device.
 * This macro instantiates and initializes the data structures for a PTY character
 * device.
 *
 * @param name: device name.
 */
#define CHARDEV_PTY_DEFINE(name)    \
static int data_##name;             \
struct chardev name =               \
{                                   \
    .config = NULL,                 \
    .data   = &data_##name,         \
    .api    = &pty_chardev_api      \
};

/**
 * Character device API definition for linux PTY.
 */
extern const struct chardev_api pty_chardev_api;

#endif

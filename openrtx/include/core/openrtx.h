/***************************************************************************
 *   Copyright (C) 2022 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef OPENRTX_H
#define OPENRTX_H

#include <stddef.h>

/**
 * Initialisation of all the OpenRTX components, to be called before the main
 * entrypoint.
 */
void openrtx_init();

/**
 * Entrypoint of the OpenRTX firmware.
 * This function returns only on linux emulator, when the main program terminates.
 * Return type is void* to make this function be used as a pthread thread body.
 */
void *openrtx_run();

#endif /* OPENRTX_H */

/***************************************************************************
 *   Copyright (C) 2020 by Silvano Seva IU2KWO and Niccolò Izzo IU2KIN     *
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

#ifndef DELAYS_H
#define DELAYS_H

/**
 * Function prototypes for microsecond and millisecond delays.
 * Their implementation is device-specific, thus it is placed inside the drivers
 * folder.
 */

/**
 * Exact microsecond delay.
 * @param useconds: delay value
 */
void delayUs(unsigned int useconds);

/**
 * Exact millisecond delay.
 * @param mseconds: delay value
 */
void delayMs(unsigned int mseconds);

#endif /* DELAYS_H */

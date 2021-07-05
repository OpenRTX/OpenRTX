/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
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

#ifndef ERROR_H
#define ERROR_H

#include <interfaces/graphics.h>
#include <stdbool.h>

/**
 * Initialise error log, always call this function before logging errors.
 */
void errorLog_init();

/**
 * Log an error message, can be called inside an IRQ.
 * @param string: error message to be logged.
 */
void IRQerrorLog(const char *string);

/**
 * Get a pointer to the error message contained in the log. If log is empty a
 * NULL pointer is returned.
 * @return error message or NULL if empty.
 */
const char *errorLog_getMessage();

/**
 * Print the error message on screen, if present.
 * @param start: coordinates specifying the starting point for the print.
 */
void errorLog_printMessage(const point_t start);

/**
 * Check whether error log is empty or not.
 * @return true if log contains a message, false otherwise.
 */
bool errorLog_notEmpty();

/**
 * Clear the error log content.
 */
void errorLog_clear();

#endif

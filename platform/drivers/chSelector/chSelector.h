/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Caleb Jamison                            *
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

#ifndef CH_SELECTOR_H
#define CH_SELECTOR_H

/**
 * Low-level driver for correct handling of channel selector knobs connected to
 * a quadrature encoder.
 * This header file only provides the API for driver initialisation and shutdown,
 * while the readout of current encoder position is provided by target-specific
 * sources by implementating platform_getChSelector().
 */

/**
 * Initialise channel selector driver.
 */
void chSelector_init();

/**
 * Terminate channel selector driver.
 */
void chSelector_terminate();

#endif /* CH_SELECTOR_H */

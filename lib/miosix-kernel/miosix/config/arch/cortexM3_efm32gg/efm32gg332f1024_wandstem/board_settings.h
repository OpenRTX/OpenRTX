/***************************************************************************
 *   Copyright (C) 2015-2021 by Terraneo Federico                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#pragma once

/**
 * \internal
 * Versioning for board_settings.h for out of git tree projects
 */
#define BOARD_SETTINGS_VERSION 300

/**
 * Select hardware revision (10=1.0, 11=1.1, ...).
 * Different versions of the board were built, with minor differences in GPIO
 * usage. Default is currently the latest one, which is v1.4
 */
#define WANDSTEM_HW_REV 14

/**
 * Disable FLOPSYNCVHT support in the board support package.
 * Note that for best clock skew correction you should avoid entering the
 * deep sleep state if you disable FLOSPYNCVHT
 */
// #define DISABLE_FLOPSYNCVHT

namespace miosix {

/**
 * \addtogroup Settings
 * \{
 */

/// Size of stack for main().
const unsigned int MAIN_STACK_SIZE=4096;

/// Serial port
const unsigned int defaultSerial=0;
const unsigned int defaultSerialSpeed=115200;

/**
 * \}
 */

} //namespace miosix

/***************************************************************************
 *   Copyright (C) 2010-2021 by Terraneo Federico                          *
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

namespace miosix {

/**
 * \addtogroup Settings
 * \{
 */

/// Serial port baudrate
const unsigned int SERIAL_PORT_SPEED=115200;

/// Uncomment to enable USART1 as well. This is only possible if WITH_DEVFS is
/// defined in miosix_settings.h The device will appear as /dev/auxtty.
//#define AUX_SERIAL "auxtty"

/// Aux serial port baudrate
const unsigned int AUX_SERIAL_SPEED=9600;

/// Size of stack for main().
/// The C standard library is stack-heavy (iprintf requires 1.5KB) and the
/// LPC2138 has 32KB of RAM so there is room for a big 4K stack.
const unsigned int MAIN_STACK_SIZE=4*1024;

/// \internal Clock frequency of hardware timer, hardware specific data
const unsigned int TIMER_CLOCK=14745600;

/// \def WITH_RTC
/// Uncomment to enable support for RTC. Time-related functions depend on it.
/// By default it is defined (RTC is active)
#define WITH_RTC

/// \def WAKEUP_DELAY
/// Uncomment if you want that to resume after shutdown, the user must hold down
/// the button for two seconds. Comment if you want instant resume.
/// By default it is not defined (no wakeup delay)
//#define WAKEUP_DELAY

/**
 * \}
 */

} //namespace miosix

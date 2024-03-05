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

namespace miosix {

/**
 * \addtogroup Settings
 * \{
 */

/// Select oscillator speed at startup. Currently supported configurations are
/// 4, 8, 12MHz, all using RCFAST
constexpr unsigned int bootClock=12000000;

/// If this is NOT defined, start32kHzOscillator() starts the 32kHz crystal
/// oscillator, so you need a quarts crystal attached to the proper pins.
/// If this IS defined, start32kHzOscillator() falls back to the internal RC osc
//#define USE_RC_32K_OSCILLATOR

/// Use AST as os_timer instead of TC1. This requires a 32kHz crystal to be
/// connected to the board, reduces timing resolution to only 16kHz and makes
/// context switches much slower but the os easily keeps time across deep sleeps
//#define WITH_RTC_AS_OS_TIMER

/// Size of stack for main().
/// The C standard library is stack-heavy (iprintf requires 1KB) but the
/// atsam4lc2aa only has 32KB of RAM so there is room for a big 4K stack.
const unsigned int MAIN_STACK_SIZE=4*1024;

/// Serial port
const unsigned int defaultSerial=2;
const unsigned int defaultSerialSpeed=115200;

/**
 * \}
 */

} //namespace miosix

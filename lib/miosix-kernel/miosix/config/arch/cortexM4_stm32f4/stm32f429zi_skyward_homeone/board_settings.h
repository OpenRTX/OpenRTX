/***************************************************************************
 *   Copyright (C) 2014-2021 by Terraneo Federico                          *
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

/// Size of stack for main().
/// The C standard library is stack-heavy (iprintf requires 1KB) but the
/// STM32F407VG only has 192KB of RAM so there is room for a big 4K stack.
const unsigned int MAIN_STACK_SIZE=4*1024;

/// Serial port
const unsigned int defaultSerial=1;
const unsigned int defaultSerialSpeed=19200;
const bool defaultSerialFlowctrl=false;
#define SERIAL_1_DMA
//#define SERIAL_2_DMA //Serial 2 can't be used (GPIO conflict), so no DMA
//#define SERIAL_3_DMA //Serial 3 can't be used (GPIO conflict), so no DMA

//SD card driver
static const unsigned char sdVoltage=30; //Board powered @ 3.0V
#ifdef __ENABLE_XRAM
//Reduce SD clock to ~4.8MHz
#define OVERRIDE_SD_CLOCK_DIVIDER_MAX 8
#endif //__ENABLE_XRAM
//#define SD_ONE_BIT_DATABUS //This board supports 4 bit databus to SD card

/**
 * \}
 */

} //namespace miosix

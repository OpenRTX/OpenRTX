/***************************************************************************
 *   Copyright (C) 2012-2021 by Terraneo Federico                          *
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

/// Serial port (USART3 PB10=TX, PB11=RX)
const unsigned int defaultSerial=3;
const unsigned int defaultSerialSpeed=19200;
const bool defaultSerialFlowctrl=false;
// Aux serial port (hardcoded USART2 PA2=TX, PA3=RX).
// Uncomment AUX_SERIAL to enable. The device will appear as /dev/auxtty.
//#define AUX_SERIAL "auxtty"
const unsigned int auxSerialSpeed=9600;
const bool auxSerialFlowctrl=false;
//#define SERIAL_1_DMA //Serial 1 is not used, so not enabling DMA
//#define SERIAL_2_DMA //Serial 2 DMA conflicts with I2S driver in the examples
#define SERIAL_3_DMA

//SD card driver
static const unsigned char sdVoltage=30; //Board powered @ 3.0V
#define SD_ONE_BIT_DATABUS //Can't use 4 bit databus due to pin conflicts

/**
 * \}
 */

} //namespace miosix

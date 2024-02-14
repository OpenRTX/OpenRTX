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
/// The C standard library is stack-heavy (iprintf requires 1KB)
/// Application requires more than the usual 4KB stack, increasing to 5KB.
const unsigned int MAIN_STACK_SIZE=5*1024;

const unsigned int defaultSerial=1;
const unsigned int defaultSerialSpeed=115200;
const bool defaultSerialFlowctrl=false;
// Uncomment AUX_SERIAL to enable. The device will appear as /dev/auxtty.
#define AUX_SERIAL "auxtty"
const unsigned int auxSerial=3;
const unsigned int auxSerialSpeed=230400;
const bool auxSerialFlowctrl=false;
#define SERIAL_1_DMA
//#define SERIAL_2_DMA
#define SERIAL_3_DMA

//SD card driver
static const unsigned char sdVoltage=33; //Board powered @ 3.3V
// #define SD_ONE_BIT_DATABUS //Commented to use 4 bit databus

/**
 * \}
 */

} //namespace miosix

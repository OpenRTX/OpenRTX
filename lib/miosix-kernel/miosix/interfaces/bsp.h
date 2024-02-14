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

namespace miosix {

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file bsp.h
 * This file contains architecture specific board support package.
 * It must at least provide these four functions:
 *
 * IRQbspInit(), to initialize the board to a known state early in the boot
 * process (before the kernel is started, and when interrupts are disabled)
 *
 * bspInit2(), to perform the remaining part of the initialization, once the
 * kernel is started
 *
 * shutdown(), for system shutdown. This function is called in case main()
 * returns, and is available to be called by user code.
 *
 * reboot(), a function that can be called to reboot the system under normal
 * (non error) conditions. It should sync and unmount the filesystem, and
 * perform a reboot. This function is available for user code.
 *
 * Other than this, the board support package might contain other functions,
 * classes, macros etc. to support peripherals and or board hardware.
 */

/**
 * \internal
 * Initializes the I/O pins, and put system peripherals to a known state.<br>
 * Must be called before starting kernel. Interrupts must be disabled.<br>
 * This function is used by the kernel and should not be called by user code.
 */
void IRQbspInit();

/**
 * \internal
 * Performs the part of initialization that must be done after the kernel is
 * started.<br>
 * This function is used by the kernel and should not be called by user code.
 */
void bspInit2();

/**
 * This function disables filesystem (if enabled), serial port (if enabled) and
 * shuts down the system, usually by putting the procesor in a deep sleep
 * state.<br>
 * The action to start a new boot is system-specific, can be for example a
 * reset, powercycle or a special GPIO configured to wakeup the processor from
 * deep sleep.<br>
 * This function does not return.<br>
 * WARNING: close all files before using this function, since it unmounts the
 * filesystem.<br>
 */
void shutdown();

/**
 * The difference between this function and miosix_private::IRQsystemReboot()
 * is that this function disables filesystem (if enabled), serial port
 * (if enabled) while miosix_private::system_reboot() does not do all these
 * things. miosix_private::IRQsystemReboot() is designed to reboot the system
 * when an unrecoverable error occurs, and is used primarily in kernel code,
 * reboot() is designed to reboot the system in normal conditions.<br>
 * This function does not return.<br>
 * WARNING: close all files before using this function, since it unmounts the
 * filesystem.
 */
void reboot();

/**
 * \}
 */

} //namespace miosix

/*
 * Since the architecture specific board support package can declare other
 * functions and macros, include this header.
 */
// #include "interfaces-impl/bsp_impl.h"

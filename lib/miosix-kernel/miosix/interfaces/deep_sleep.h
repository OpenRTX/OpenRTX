/***************************************************************************
 *   Copyright (C) 2018-2021 by Terraneo Federico, Daniele Marsella        *
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
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file deep_sleep.h
 * This file contains required functions to implement automatic deep sleep state
 * switch.
 * 
 * This solution on supported hardware allows to power off also the peripherals
 * when the system is in idle state and doesn't require any peripheral action.
 * 
 * NOTE: this interface is meant to be used only by the kernel and not by user
 * code. When automatic deep sleep is enabled, it is transparent to applications.
 */

namespace miosix {

/**
 * \internal
 * Initialize the required component to support the deep sleep functionalities.
 */
void IRQdeepSleepInit();

/** 
 * \internal
 * Put in deep sleep the board until the next wakeup schedule.
 * \param abstime : selected absolute time to wake up from deep sleep state.
 * This blocking function shall return when abstime is reached.
 * \return true if the deep sleep operation was performed succesfully, and the
 * function has returned at the prescribed time (within its tolerance).
 * This function may immediately return false if some condition is not met and
 * going in deep sleep was not possible, for example the requested wakeup time
 * is too close considering the overhead of going in deep sleep.
 */
bool IRQdeepSleep(long long abstime);

/**
 * \internal
 * Put in deep sleep the board without a wakeup time.
 * This may happen because of waiting for some event that can happen also
 * in deep sleep.
 * \return true if the deep sleep operation was performed succesfully.
 * This function may immediately return false if some condition is not met and
 * going in deep sleep was not possible.
 */
bool IRQdeepSleep();

} //namespace miosix

/**
 * \}
 */

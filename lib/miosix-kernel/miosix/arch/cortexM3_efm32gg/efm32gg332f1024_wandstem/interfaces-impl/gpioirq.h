/***************************************************************************
 *   Copyright (C) 2016 by Terraneo Federico                               *
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

#include <functional>
#include <interfaces/gpio.h>

namespace miosix {

/**
 * Constants used to select which edges should cause an interrupt
 */
enum class GpioIrqEdge
{
    RISING,
    FALLING,
    BOTH
};

/**
 * Register an interrupt to be called when an edge occurs on a specific GPIO
 * Due to hardware limitations, it is possible to register up to 16 interrupts,
 * and no two with the same pin number. So, if an interrupt is set on
 * PA0, it is not possible to register an interrupt on PB0, PC0, ...
 * 
 * Note that this function just configures the interrupt, but it does not enable
 * it. To enable it, you have to call enableGpioIrq().
 * 
 * \param pin GPIO pin on which the interrupt is requested
 * \param edge allows to set whether the interrupt should be fired on the
 * rising, falling or both edges
 * \param callback a callback function that that will be called. Note that the
 * callback is called in interrupt context, so it is subject to the usual
 * restrictions that apply to interrupt context, such that only IRQ prefixed
 * functions can be called, etc. It is recomended to just wake a task and keep
 * the function short. Note that it is also possible to call the scheduler to
 * cause a context switch.
 * \throws exception if the resources for this interrupt are occupied
 */
void registerGpioIrq(GpioPin pin, GpioIrqEdge edge, std::function<void ()> callback);

/**
 * Enable the interrupt on the specified pin
 * \param pin GPIO pin on which the interrupt is requested
 * \throws runtime_error if the pin has not been registered
 */
void enableGpioIrq(GpioPin pin);

/**
 * Disable the interrupt on the specified pin
 * \param pin GPIO pin on which the interrupt is requested
 * \throws runtime_error if the pin has not been registered
 */
void disableGpioIrq(GpioPin pin);

/**
 * Enable the interrupt on the specified pin. Callable with interrupts disabled
 * \param pin GPIO pin on which the interrupt is requested
 * \return false if the pin has not been registered
 */
bool IRQenableGpioIrq(GpioPin pin);

/**
 * Disable the interrupt on the specified pin. Callable with interrupts disabled
 * \param pin GPIO pin on which the interrupt is requested
 * \return false if the pin has not been registered
 */
bool IRQdisableGpioIrq(GpioPin pin);

/**
 * Unregister an interrupt, also freeing the resources for other to register a
 * new one.
 * \param pin pint to unregister
 */
void unregisterGpioIrq(GpioPin pin);

} //namespace miosix

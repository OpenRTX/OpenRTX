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
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file gpio.h
 * The interface to gpios provided by Miosix is in the form of templates,
 * therefore this file can only include gpio_impl.h with the architecture
 * dependand code.
 *
 * The interface should be as follows:
 * First a class Mode containing an enum Mode_ needs to be defined. Its minimum
 * implementation is this:
 * \code
 * class Mode
 * {
 * public:
 *     enum Mode_
 *     {
 *         INPUT,
 *         OUTPUT
 *     };
 * private:
 *     Mode(); //Just a wrapper class, disallow creating instances
 * };
 * \endcode
 *
 * This class should define the possible configurations of a gpio pin.
 * The minimum required is INPUT and OUTPUT, but this can be extended to other
 * options to reflect the hardware capabilities of gpios. For example, if
 * gpios can be set as input with pull up, it is possible to add INPUT_PULL_UP
 * to the enum.
 *
 * Then a template Gpio class should be provided, with at least the following
 * member functions:
 * \code
 * template<unsigned int P, unsigned char N>
 * class Gpio
 * {
 * public:
 *     static void mode(Mode::Mode_ m);
 *     static void high();
 *     static void low();
 *     static int value();
 *     unsigned int getPort() const;
 *     unsigned char getNumber() const;
 * private:
 *     Gpio();//Only static member functions, disallow creating instances
 * };
 * \endcode
 *
 * mode() should take a Mode::Mode_ enum and set the mode of the gpio
 * (input, output or other architecture specific)
 *
 * high() should set a gpio configured as output to high logic level
 *
 * low() should set a gpio configured as output to low logic level
 *
 * value() should return either 1 or 0 to refect the state of a gpio configured
 * as input
 * 
 * getPort() should return the gpio port
 * 
 * getNumber() should return the gpio pin number
 *
 * Lastly, a number of constants must be provided to be passed as first template
 * parameter of the Gpio class, usually identifying the gpio port, while the
 * second template parameter should be used to specify a gpio pin within a port.
 *
 * The intended use is this:
 * considering an architecture with two ports, PORTA and PORTB each with 8 pins.
 * The header gpio_impl.h should provide two constants, for example named
 * GPIOA_BASE and GPIOB_BASE.
 *
 * The user can declare the hardware mapping between gpios and what is connected
 * to them, usually in an header file. If for example PORTA.0 is connected to
 * a button while PORTB.4 to a led, the header file might contain:
 *
 * \code
 * typedef Gpio<GPIOA_BASE,0> button;
 * typedef Gpio<GPIOB_BASE,4> led;
 * \endcode
 *
 * This allows the rest of the code to be written in terms of leds and buttons,
 * without a reference to which pin they are connected to, something that might
 * change.
 *
 * A simple code using these gpios could be:
 * \code
 * void blinkUntilButtonPressed()
 * {
 *     led::mode(Mode::OUTPUT);
 *     button::mode(Mode::INPUT);
 *     for(;;)
 *     {
 *         if(button::value()==1) break;
 *         led::high();
 *         Thread::sleep(250);
 *         led::low();
 *         Thread::sleep(250);
 *     }
 * }
 * \endcode
 *
 */

/**
 * \}
 */

#include "interfaces-impl/gpio_impl.h"

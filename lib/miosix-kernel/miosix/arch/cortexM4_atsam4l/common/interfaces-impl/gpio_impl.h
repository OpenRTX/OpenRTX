/***************************************************************************
 *   Copyright (C) 2020 by Terraneo Federico                               *
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

#include "interfaces/arch_registers.h"

//The ATSAM header files do not provide a #define for each GPIO, rather a single
//array of structs. Thus, the port names map to indices to that array.
//To be able to access GPIOs the Miosix way, we need to add those.
//They are put outside of the miosix namespace for compatibility with other BSPs
const unsigned int GPIOA_BASE = GPIO_ADDR;
const unsigned int GPIOB_BASE = GPIO_ADDR + 1*0x200;
const unsigned int GPIOC_BASE = GPIO_ADDR + 2*0x200;

namespace miosix {

/*
 * A note for the user: writing this driver has been harder than expected as
 * the ATSAM4L documentation is quite a bit incpmplete and inconsistent, here's
 * a quick list of the dark corners:
 * - Figure 23-2 shows a GPIO_ODMER register, but there's no mention of it
 *   anywhere else in the entire document. The header file has this register
 *   and a comment alludes to being an open drain feature. I tried implementing
 *   it in the driver and tested it on PA0, but it doesn't seem to work.
 * - Documentation for pullup/pulldown mentions that setting PUER=PDER=1 enables
 *   a "buskeeper" feature, which of course isn't documented anywhere else.
 * - There are supposed to be 4 levels of drive strength, configured using two
 *   bits ODCR0 and ODCR1. However, the documentation of how many mA does pins
 *   drive only exist for ODCR0. Does ODCR1 even exist and do something?
 * - The header file has more completely undocumented registers, such as LOCK
 *   and UNLOCK registers...
 * - The datasheet lists a Schmitt trigger feature implying it's an optional
 *   feature for inputs, but it looks like it *must* be enabled for the GPIO to
 *   operate as an input
 */

/**
 * GPIO mode (INPUT, OUTPUT, ...)
 * \code pin::mode(Mode::INPUT);\endcode
 */
enum class Mode : unsigned char
{
    OFF                    = 0b00000, ///Disconnected
    INPUT                  = 0b10000, ///Floating Input
    INPUT_PULL_UP          = 0b10100, ///Pullup   Input
    INPUT_PULL_DOWN        = 0b11000, ///Pulldown Input
    OUTPUT                 = 0b10001, ///Push Pull Output
    ALTERNATE              = 0b10010, ///Alternate function

    //These may not not work with all peripherals, as section 23.6.2.1 says that
    //some peripherals may override pullup/down/schmitt trigger control
    ALTERNATE_PULL_UP      = 0b10110, ///Alternate function Pullup
    ALTERNATE_PULL_DOWN    = 0b11010, ///Alternate function Pulldown
};

/**
 * Base class to implement non template-dependent functions that, if inlined,
 * would significantly increase code size
 */
class GpioBase
{
protected:
    static void modeImpl(GpioPort *p, unsigned char n, Mode m);
    static void afImpl(GpioPort *p, unsigned char n, char af);
    static void strengthImpl(GpioPort *p, unsigned char n, unsigned char s);
    static void slewImpl(GpioPort *p, unsigned char n, bool s);
};

/**
 * This class allows to easiliy pass a Gpio as a parameter to a function.
 * Accessing a GPIO through this class is slower than with just the Gpio,
 * but is a convenient alternative in some cases. Also, an instance of this
 * class occupies a few bytes of memory, unlike the Gpio class.
 */
class GpioPin : private GpioBase
{
public:
    /**
     * \internal
     * Constructor
     * \param p GPIOA_BASE, GPIOB_BASE, ...
     * \param n which pin (0 to 31)
     */
    GpioPin(unsigned int p, unsigned char n)
        : p(reinterpret_cast<GpioPort*>(p)), n(n) {}

    /**
     * Set the GPIO to the desired mode (INPUT, OUTPUT, ...)
     * \param m enum Mode
     */
    void mode(Mode m)
    {
        modeImpl(p,n,m);
    }

    /**
     * Set the GPIO drive strength
     * \param strength GPIO drive strength, from 0 to 3, with 0 being the lowest
     * and 3 being the highest. 
     */
    void driveStrength(unsigned char s)
    {
        strengthImpl(p,n,s);
    }

    /**
     * Set the GPIO slew rate slew rate control
     * \param s true to enable slew rate control false to turn it off
     */
    void slewRateControl(bool s)
    {
        slewImpl(p,n,s);
    }

    /**
     * Select which of the many alternate functions is to be connected with the
     * GPIO pin.
     * \param af alternate function, you can pass either a number in the range
     * 0 to 7, or a character in the range 'a' to 'h' or 'A' to 'H'. 0 is the
     * same as 'a' or 'A', and so on.
     */
    void alternateFunction(char af)
    {
        afImpl(p,n,af);
    }

    /**
     * Set the pin to 1, if it is an output
     */
    void high()
    {
        p->GPIO_OVRS=1<<n;
    }

    /**
     * Set the pin to 0, if it is an output
     */
    void low()
    {
        p->GPIO_OVRC=1<<n;
    }
    
    /**
     * Toggle pin, if it is an output
     */
    void toggle()
    {
        p->GPIO_OVRT=1<<n;
    }

    /**
     * Allows to read the pin status
     * \return 0 or 1
     */
    int value()
    {
        return (p->GPIO_PVR & 1<<n) ? 1 : 0;
    }

    /**
     * \return the pin port. One of the constants PORTA_BASE, PORTB_BASE, ...
     */
    unsigned int getPort() const { return reinterpret_cast<unsigned int>(p); }

    /**
     * \return the pin number, from 0 to 31
     */
    unsigned char getNumber() const { return n; }

private:
    GpioPort *p;     //Pointer to the port
    unsigned char n; //Number of the GPIO within the port
};

/**
 * Gpio template class
 * \param P GPIOA_BASE, GPIOB_BASE, ...
 * \param N which pin (0 to 31)
 * The intended use is to make a typedef to this class with a meaningful name.
 * \code
 * typedef Gpio<PORTA_BASE,0> green_led;
 * green_led::mode(Mode::OUTPUT);
 * green_led::high();//Turn on LED
 * \endcode
 */
template<unsigned int P, unsigned char N>
class Gpio : private GpioBase
{
public:
    Gpio() = delete; //Disallow creating instances

    /**
     * Set the GPIO to the desired mode (INPUT, OUTPUT, ...)
     * \param m enum Mode
     */
    static void mode(Mode m)
    {
        modeImpl(reinterpret_cast<GpioPort*>(P),N,m);
    }

    /**
     * Set the GPIO drive strength
     * \param s GPIO drive strength, from 0 to 3, with 0 being the lowest and
     * 3 being the highest. 
     */
    static void driveStrength(unsigned char s)
    {
        strengthImpl(reinterpret_cast<GpioPort*>(P),N,s);
    }

    /**
     * Set the GPIO slew rate slew rate control
     * \param s true to enable slew rate control false to turn it off
     */
    static void slewRateControl(bool s)
    {
        slewImpl(reinterpret_cast<GpioPort*>(P),N,s);
    }

    /**
     * Select which of the many alternate functions is to be connected with the
     * GPIO pin.
     * \param af alternate function, you can pass either a number in the range
     * 0 to 7, or a character in the range 'a' to 'h' or 'A' to 'H'. 0 is the
     * same as 'a' or 'A', and so on.
     */
    static void alternateFunction(char af)
    {
        afImpl(reinterpret_cast<GpioPort*>(P),N,af);
    }

    /**
     * Set the pin to 1, if it is an output
     */
    static void high()
    {
        reinterpret_cast<GpioPort*>(P)->GPIO_OVRS=1<<N;
    }

    /**
     * Set the pin to 0, if it is an output
     */
    static void low()
    {
        reinterpret_cast<GpioPort*>(P)->GPIO_OVRC=1<<N;
    }
    
    /**
     * Toggle pin, if it is an output
     */
    static void toggle()
    {
        reinterpret_cast<GpioPort*>(P)->GPIO_OVRT=1<<N;
    }

    /**
     * Allows to read the pin status
     * \return 0 or 1
     */
    static int value()
    {
        return ((reinterpret_cast<GpioPort*>(P)->GPIO_PVR & 1<<N) ? 1 : 0);
    }

    /**
     * \return this Gpio converted as a GpioPin class 
     */
    static GpioPin getPin()
    {
        return GpioPin(P,N);
    }

    /**
     * \return the pin port. One of the constants PORTA_BASE, PORTB_BASE, ...
     */
    unsigned int getPort() const { return P; }
    
    /**
     * \return the pin number, from 0 to 31
     */
    unsigned char getNumber() const { return N; }
};

} //namespace miosix

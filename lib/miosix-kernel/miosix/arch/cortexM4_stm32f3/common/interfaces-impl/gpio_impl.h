/***************************************************************************
 *   Copyright (C) 2009, 2010, 2011, 2012 by Terraneo Federico             *
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

/*
 * Versions:
 * 1.0 First release
 * 1.1 Made Mode, Gpio and GpioBase contructor private to explicitly disallow
 *     creating instances of these classes.
 * 1.2 Fixed a bug
 * 1.3 Applied patch by Lee Richmond (http://pastebin.com/f7ae1a65f). Now
 *     mode() is inlined too.
 * 1.4 Adapted to stm32f4
 * 1.5 Added GpioPin for easily passing a Gpio as a parameter to a function
 */

#ifndef GPIO_IMPL_H
#define	GPIO_IMPL_H

#include "interfaces/arch_registers.h"

namespace miosix {

/**
 * This class just encapsulates the Mode_ enum so that the enum names don't
 * clobber the global namespace.
 */
class Mode
{
public:
    /**
     * GPIO mode (INPUT, OUTPUT, ...)
     * \code pin::mode(Mode::INPUT);\endcode
     */
    enum Mode_
    {
        INPUT              =  0, ///Floating Input       (MODE=00 TYPE=0 PUP=00)
        INPUT_PULL_UP      =  1, ///Pullup   Input       (MODE=00 TYPE=0 PUP=01)
        INPUT_PULL_DOWN    =  2, ///Pulldown Input       (MODE=00 TYPE=0 PUP=10)
        INPUT_ANALOG       = 24, ///Analog   Input       (MODE=11 TYPE=0 PUP=00)
        OUTPUT             =  8, ///Push Pull  Output    (MODE=01 TYPE=0 PUP=00)
        OPEN_DRAIN         = 12, ///Open Drain Output    (MODE=01 TYPE=1 PUP=00)
        ALTERNATE          = 16, ///Alternate function   (MODE=10 TYPE=0 PUP=00)
        ALTERNATE_OD       = 20, ///Alternate Open Drain (MODE=10 TYPE=1 PUP=00)
    };
private:
    Mode(); //Just a wrapper class, disallow creating instances
};

/**
 * This class just encapsulates the Speed_ enum so that the enum names don't
 * clobber the global namespace.
 */
class Speed
{
public:
    /**
     * GPIO speed
     * \code pin::speed(Speed::_100MHz);\endcode
     */
    enum Speed_
    {
        _2MHz   = 0x0,
        _10MHz  = 0x1,
        _50MHz  = 0x3,
    };
private:
    Speed(); //Just a wrapper class, disallow creating instances
};

/**
 * Base class to implement non template-dependent functions that, if inlined,
 * would significantly increase code size
 */
class GpioBase
{
protected:
	static void modeImpl(unsigned int p, unsigned char n, Mode::Mode_ m);
    static void afImpl(unsigned int p, unsigned char n, unsigned char af);
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
     * Constructor
     * \param p GPIOA_BASE, GPIOB_BASE, ... as #define'd in stm32f4xx.h
     * \param n which pin (0 to 15)
     */
    GpioPin(unsigned int p, unsigned char n)
        : p(reinterpret_cast<GPIO_TypeDef*>(p)), n(n) {}
    
    /**
     * Set the GPIO to the desired mode (INPUT, OUTPUT, ...)
     * \param m enum Mode_
     */
    void mode(Mode::Mode_ m)
    {
        modeImpl(reinterpret_cast<unsigned int>(p),n,m);
    }
    
    /**
     * Set the GPIO speed
     * \param s speed value
     */
    void speed(Speed::Speed_ s)
    {
        p->OSPEEDR &= ~(3<<(n*2));
        p->OSPEEDR |= s<<(n*2);
    }
    
    /**
     * Select which of the many alternate functions is to be connected with the
     * GPIO pin.
     * \param af alternate function number, from 0 to 15
     */
    void alternateFunction(unsigned char af)
    {
        afImpl(reinterpret_cast<unsigned int>(p),n,af);
    }

    /**
     * Set the pin to 1, if it is an output
     */
    void high()
    {
        p->BSRR= 1<<n;
    }

    /**
     * Set the pin to 0, if it is an output
     */
    void low()
    {
        p->BSRR=1<<(n+16);
    }

    /**
     * Allows to read the pin status
     * \return 0 or 1
     */
    int value()
    {
        return (p->IDR & (1<<n)) ? 1 : 0;
    }

    /**
     * \return the pin port. One of the constants PORTA_BASE, PORTB_BASE, ...
     */
    unsigned int getPort() const { return reinterpret_cast<unsigned int>(p); }

    /**
     * \return the pin number, from 0 to 15
     */
    unsigned char getNumber() const { return n; }
    
private:
    GPIO_TypeDef *p; //Pointer to the port
    unsigned char n; //Number of the GPIO within the port
};

/**
 * Gpio template class
 * \param P GPIOA_BASE, GPIOB_BASE, ... as #define'd in stm32f4xx.h
 * \param N which pin (0 to 15)
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
    /**
     * Set the GPIO to the desired mode (INPUT, OUTPUT, ...)
     * \param m enum Mode_
     */
    static void mode(Mode::Mode_ m)
    {
        modeImpl(P,N,m);
    }
    
    /**
     * Set the GPIO speed
     * \param s speed value
     */
    static void speed(Speed::Speed_ s)
    {
        reinterpret_cast<GPIO_TypeDef*>(P)->OSPEEDR &= ~(3<<(N*2));
        reinterpret_cast<GPIO_TypeDef*>(P)->OSPEEDR |= s<<(N*2);
    }
    
    /**
     * Select which of the many alternate functions is to be connected with the
     * GPIO pin.
     * \param af alternate function number, from 0 to 15
     */
    static void alternateFunction(unsigned char af)
    {
        afImpl(P,N,af);
    }

    /**
     * Set the pin to 1, if it is an output
     */
    static void high()
    {
        reinterpret_cast<GPIO_TypeDef*>(P)->BSRR= 1<<N;
    }

    /**
     * Set the pin to 0, if it is an output
     */
    static void low()
    {
        reinterpret_cast<GPIO_TypeDef*>(P)->BSRR= 1<<(N+16);
    }

    /**
     * Allows to read the pin status
     * \return 0 or 1
     */
    static int value()
    {
        return ((reinterpret_cast<GPIO_TypeDef*>(P)->IDR & 1<<N)? 1 : 0);
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
     * \return the pin number, from 0 to 15
     */
    unsigned char getNumber() const { return N; }

private:
    Gpio();//Only static member functions, disallow creating instances
};

} //namespace miosix

#endif	//GPIO_IMPL_H

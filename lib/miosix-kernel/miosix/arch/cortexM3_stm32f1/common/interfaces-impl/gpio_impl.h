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
 * 1.4 Added GpioPin for easily passing a Gpio as a parameter to a function
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
        INPUT              = 0x4, ///Floating Input             (CNF=01 MODE=00)
        INPUT_PULL_UP_DOWN = 0x8, ///Pullup/Pulldown Input      (CNF=10 MODE=00)
        INPUT_ANALOG       = 0x0, ///Analog Input               (CNF=00 MODE=00)
        OUTPUT             = 0x3, ///Push Pull  50MHz Output    (CNF=00 MODE=11)
        OUTPUT_10MHz       = 0x1, ///Push Pull  10MHz Output    (CNF=00 MODE=01)
        OUTPUT_2MHz        = 0x2, ///Push Pull   2MHz Output    (CNF=00 MODE=10)
        OPEN_DRAIN         = 0x7, ///Open Drain 50MHz Output    (CNF=01 MODE=11)
        OPEN_DRAIN_10MHz   = 0x5, ///Open Drain 10MHz Output    (CNF=01 MODE=01)
        OPEN_DRAIN_2MHz    = 0x6, ///Open Drain  2MHz Output    (CNF=01 MODE=10)
        ALTERNATE          = 0xb, ///Alternate function 50MHz   (CNF=10 MODE=11)
        ALTERNATE_10MHz    = 0x9, ///Alternate function 10MHz   (CNF=10 MODE=01)
        ALTERNATE_2MHz     = 0xa, ///Alternate function  2MHz   (CNF=10 MODE=10)
        ALTERNATE_OD       = 0xf, ///Alternate Open Drain 50MHz (CNF=11 MODE=11)
        ALTERNATE_OD_10MHz = 0xd, ///Alternate Open Drain 10MHz (CNF=11 MODE=01)
        ALTERNATE_OD_2MHz  = 0xe  ///Alternate Open Drain  2MHz (CNF=11 MODE=10)
    };
private:
    Mode(); //Just a wrapper class, disallow creating instances
};

template<unsigned int P, unsigned char N, bool = N >= 8>
struct GpioMode
{
    inline static void mode(Mode::Mode_ m)
    {
        reinterpret_cast<GPIO_TypeDef*>(P)->CRH &= ~(0xf<<((N-8)*4));
        reinterpret_cast<GPIO_TypeDef*>(P)->CRH |= m<<((N-8)*4);
    }
};

template<unsigned int P, unsigned char N>
struct GpioMode<P, N, false>
{
    inline static void mode(Mode::Mode_ m)
    {
        reinterpret_cast<GPIO_TypeDef*>(P)->CRL &= ~(0xf<<(N*4));
        reinterpret_cast<GPIO_TypeDef*>(P)->CRL |= m<<(N*4);
    }
};

/**
 * This class allows to easiliy pass a Gpio as a parameter to a function.
 * Accessing a GPIO through this class is slower than with just the Gpio,
 * but is a convenient alternative in some cases. Also, an instance of this
 * class occupies a few bytes of memory, unlike the Gpio class.
 */
class GpioPin
{
public:
    /**
     * Constructor
     * \param p GPIOA_BASE, GPIOB_BASE, ... as #define'd in stm32f10x.h
     * \param n which pin (0 to 15)
     */
    GpioPin(unsigned int p, unsigned char n)
        : p(reinterpret_cast<GPIO_TypeDef*>(p)), n(n) {}
        
    /**
     * Set the GPIO to the desired mode (INPUT, OUTPUT, ...)
     * \param m enum Mode_
     */
    void mode(Mode::Mode_ m);

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
        p->BRR= 1<<n;
    }

    /**
     * Allows to read the pin status
     * \return 0 or 1
     */
    int value()
    {
        return (p->IDR & 1<<n) ? 1 : 0;
    }

    /**
     * Set pullup on pin, if its mode is Mode::INPUT_PULL_UP_DOWN
     */
    void pullup()
    {
        high();//When in input pullup/pulldown mode ODR=choose pullup/pulldown
    }

    /**
     * Set pulldown on pin, if its mode is Mode::INPUT_PULL_UP_DOWN
     */
    void pulldown()
    {
        low();//When in input pullup/pulldown mode ODR=choose pullup/pulldown
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
 * \param P GPIOA_BASE, GPIOB_BASE, ... as #define'd in stm32f10x.h
 * \param N which pin (0 to 15)
 * The intended use is to make a typedef to this class with a meaningful name.
 * \code
 * typedef Gpio<PORTA_BASE,0> green_led;
 * green_led::mode(Mode::OUTPUT);
 * green_led::high();//Turn on LED
 * \endcode
 */
template<unsigned int P, unsigned char N>
class Gpio
{
public:
    /**
     * Set the GPIO to the desired mode (INPUT, OUTPUT, ...)
     * \param m enum Mode_
     */
    static void mode(Mode::Mode_ m)
    {
        GpioMode<P, N>::mode(m);
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
        reinterpret_cast<GPIO_TypeDef*>(P)->BRR= 1<<N;
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
     * Set pullup on pin, if its mode is Mode::INPUT_PULL_UP_DOWN
     */
    static void pullup()
    {
        high();//When in input pullup/pulldown mode ODR=choose pullup/pulldown
    }

    /**
     * Set pulldown on pin, if its mode is Mode::INPUT_PULL_UP_DOWN
     */
    static void pulldown()
    {
        low();//When in input pullup/pulldown mode ODR=choose pullup/pulldown
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

/***************************************************************************
 *   Copyright (C) 2015 by Terraneo Federico                               *
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

#ifndef GPIO_IMPL_H
#define	GPIO_IMPL_H

#include "interfaces/arch_registers.h"

//The EFM32 header files do not provide a pointer to struct for each GPIO,
//rather a single pointer to a GPIO struct is provided with an array of
//other structs, one for each port. Thus, the port names map to indices to that
//array. They are put outside of the miosix namespace for compatibility with
//other BSPs
const unsigned int GPIOA_BASE=0;
const unsigned int GPIOB_BASE=1;
const unsigned int GPIOC_BASE=2;
const unsigned int GPIOD_BASE=3;
const unsigned int GPIOE_BASE=4;
const unsigned int GPIOF_BASE=5;

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
        DISABLED                     = 0x10, ///<Disabled digital input, analog input on selected pins
        DISABLED_PULL_UP             = 0x20, ///<Disabled input with pullup
        INPUT                        = 0x11, ///<Floating input
        INPUT_FILTER                 = 0x21, ///<Floating input with glitch filter
        INPUT_PULL_DOWN              = 0x12, ///<Pulldown input
        INPUT_PULL_UP                = 0x22, ///<Pullup   input
        INPUT_PULL_DOWN_FILTER       = 0x13, ///<Pulldown input with glitch filter
        INPUT_PULL_UP_FILTER         = 0x23, ///<Pullup   input with glitch filter
        OUTPUT                       =  0x4, ///<Push pull   output
        OUTPUT_LOW                   = 0x14, ///<Combined operation: set pin to output and start with a low value
        OUTPUT_HIGH                  = 0x24, ///<Combined operation: set pin to output and start with a high value
        OUTPUT_ALT                   =  0x5, ///<Push pull   output with alternate drive strength
        OUTPUT_ALT_LOW               = 0x15, ///<Combined operation: set pin to output with alternate drive strength and start with a low value
        OUTPUT_ALT_HIGH              = 0x25, ///<Combined operation: set pin to output with alternate drive strength and start with a high value
        OUTPUT_OS                    =  0x6, ///<Open source output
        OUTPUS_OS_PULL_DOWN          =  0x7, ///<Open source output with pulldown
        OUTPUT_OD                    =  0x8, ///<Open drain  output
        OUTPUT_OD_FILTER             =  0x9, ///<Open drain  output with glitch filter
        OUTPUT_OD_PULL_UP            =  0xa, ///<Open drain  output with pullup
        OUTPUT_OD_PULL_UP_FILTER     =  0xb, ///<Open drain  output with pullup and glitch filter
        OUTPUT_OD_ALT                =  0xc, ///<Open drain  output with alternate drive strength
        OUTPUT_OD_ALT_FILTER         =  0xd, ///<Open drain  output with alternate drive strength and glitch filter
        OUTPUT_OD_ALT_PULL_UP        =  0xe, ///<Open drain  output with alternate drive strength and pullup
        OUTPUT_OD_ALT_PULL_UP_FILTER =  0xf, ///<Open drain  output with alternate drive strength, pullup and glitch filter
    };
private:
    Mode(); //Just a wrapper class, disallow creating instances
};

/**
 * This class just encapsulates the Dst enum so that the enum names don't
 * clobber the global namespace.
 */
class DriveStrength
{
public:
    /**
     * Drive strength for GPIO set in ALT mode
     */
    enum Dst
    {
        HIGH=2,     ///<20mA max
        STANDARD=0, ///< 6mA max
        LOW=3,      ///< 1mA max
        LOWEST=1    ///< 0.1mA max
    };
private:
    DriveStrength(); //Just a wrapper class, disallow creating instances
};

/**
 * GPIOs configured as output have a default drive strength og 6mA, but every
 * port also has an alternate drive strength setting. GPIOs configured as
 * output ALT inherit the drive strength configured with this function.
 * \param p GPIOA_BASE, GPIOB_BASE, ...
 * \param dst drive strength
 */
inline void setPortAlternateDriveStrength(unsigned int p, DriveStrength::Dst dst)
{
    GPIO->P[p].CTRL=dst;
}

/**
 * This class allows to easiliy pass a Gpio as a parameter to a function.
 * Accessing a GPIO through this class is slower than with just the Gpio,
 * but is a convenient alternative in some cases. Also, an instance of this
 * class occupies a few bytes of memory, unlike the Gpio class.
 * 
 * To instantiate classes of this type, use Gpio<P,N>::getPin()
 * \code
 * typedef Gpio<PORTA_BASE,0> led;
 * GpioPin ledPin=led::getPin();
 * \endcode
 */
class GpioPin
{
public:
    /**
     * \internal
     * Constructor. Don't instantiate classes through this constructor,
     * rather caller Gpio<P,N>::getPin().
     * \param port port struct
     * \param n which pin (0 to 15)
     */
    GpioPin(GPIO_P_TypeDef *port, unsigned char n) : port(port), n(n) {}
    
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
        port->DOUTSET=1<<n;
    }

    /**
     * Set the pin to 0, if it is an output
     */
    void low()
    {
        port->DOUTCLR=1<<n;
    }
    
    /**
     * Toggle pin, if it is an output
     */
    void toggle()
    {
        port->DOUTTGL=1<<n;
    }

    /**
     * Allows to read the pin status
     * \return 0 or 1
     */
    int value()
    {
        return ((port->DIN & 1<<n)? 1 : 0);
    }
    
    /**
     * \return the pin port. One of the constants PORTA_BASE, PORTB_BASE, ...
     */
    unsigned int getPort() const { return port-GPIO->P; }
    
    /**
     * \return the pin number, from 0 to 15
     */
    unsigned char getNumber() const { return n; }
    
private:
    GPIO_P_TypeDef *port; ///<Pointer to the port
    unsigned char n;      ///<Number of the GPIO within the port
};

namespace detail {

/**
 * \internal
 * Base class to implement non template-dependent functions that, if inlined,
 * would significantly increase code size
 */
struct ModeBase
{
    /**
     * \internal
     * Set mode to a GPIO whose number is within 0 and 7
     * \param port port struct
     * \param n which pin (0 to 15)
     * \param m enum Mode_
     */
    static void modeImplL(GPIO_P_TypeDef *port, unsigned char n, Mode::Mode_ m);
    
    /**
     * \internal
     * Set mode to a GPIO whose number is within 8 and 15
     * \param port port struct
     * \param n which pin (0 to 15)
     * \param m enum Mode_
     */
    static void modeImplH(GPIO_P_TypeDef *port, unsigned char n, Mode::Mode_ m);
};

/**
 * \internal
 * Forwarding class to call modeImplL or modeImplH depending on pin number
 * resolving which function to call at compile time
 */
template<unsigned int P, unsigned char N, bool= N>=8>
struct ModeFwd : private ModeBase
{
    inline static void mode(Mode::Mode_ m)
    {
        ModeBase::modeImplH(&GPIO->P[P],N,m);
    }
};

template<unsigned int P, unsigned char N>
struct ModeFwd<P,N,false>
{
    inline static void mode(Mode::Mode_ m)
    {
        ModeBase::modeImplL(&GPIO->P[P],N,m);
    }
};

} //namespace detail

/**
 * Gpio template class
 * \param P GPIOA_BASE, GPIOB_BASE, ...
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
        detail::ModeFwd<P,N>::mode(m);
    }

    /**
     * Set the pin to 1, if it is an output
     */
    static void high()
    {
        GPIO->P[P].DOUTSET=1<<N;
    }

    /**
     * Set the pin to 0, if it is an output
     */
    static void low()
    {
        GPIO->P[P].DOUTCLR=1<<N;
    }
    
    /**
     * Toggle pin, if it is an output
     */
    static void toggle()
    {
        GPIO->P[P].DOUTTGL=1<<N;
    }

    /**
     * Allows to read the pin status
     * \return 0 or 1
     */
    static int value()
    {
        return ((GPIO->P[P].DIN & 1<<N)? 1 : 0);
    }
    
    /**
     * \return this Gpio converted as a GpioPin class 
     */
    static GpioPin getPin()
    {
        return GpioPin(&GPIO->P[P],N);
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

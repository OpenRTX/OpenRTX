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

#include "gpio_impl.h"

namespace miosix {

void GpioPin::mode(Mode::Mode_ m)
{
    //Can't avoid the if as we've done with Gpio<P,N>, as n is not a
    //template parameter in GpioPin
    if(n>=8) detail::ModeBase::modeImplH(port,n,m);
    else detail::ModeBase::modeImplL(port,n,m);
}

namespace detail {

void ModeBase::modeImplL(GPIO_P_TypeDef *port, unsigned char n, Mode::Mode_ m)
{
    const unsigned char o=n*4;
    
    //First, transition to disabled state
    port->MODEL &= ~(0xf<<o);
    
    //Then, set port out bit as they have side effects in some modes
    if(m & 0x10) port->DOUTCLR=1<<n;
    else if(m & 0x20) port->DOUTSET=1<<n;
    
    //Last, configure port to new value
    port->MODEL |= (m & 0xf)<<o;
}

void ModeBase::modeImplH(GPIO_P_TypeDef* port, unsigned char n, Mode::Mode_ m)
{
    const unsigned char o=(n-8)*4;
    
    //First, transition to disabled state
    port->MODEH &= ~(0xf<<o);
    
    //Then, set port out bit as they have side effects in some modes
    if(m & 0x10) port->DOUTCLR=1<<n;
    else if(m & 0x20) port->DOUTSET=1<<n;
    
    //Last, configure port to new value
    port->MODEH |= (m & 0xf)<<o;
}

} //namespace detail

} //namespace miosix

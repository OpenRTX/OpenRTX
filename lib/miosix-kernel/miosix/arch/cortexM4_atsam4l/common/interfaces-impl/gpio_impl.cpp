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

#include "gpio_impl.h"

namespace miosix {

void GpioBase::modeImpl(GpioPort *p, unsigned char n, Mode m)
{
    auto mm = static_cast<unsigned char>(m);

    const unsigned char OUTPUT       = 0b00001;
    const unsigned char ALTERNATE    = 0b00010;
    const unsigned char PULLUP       = 0b00100;
    const unsigned char PULLDOWN     = 0b01000;
    const unsigned char SCHMITT_TRIG = 0b10000; //Required to function as input

    switch(mm & (PULLUP | PULLDOWN))
    {
        case PULLUP:
            p->GPIO_PDERC=1<<n;
            p->GPIO_PUERS=1<<n;
            break;
        case PULLDOWN:
            p->GPIO_PUERC=1<<n;
            p->GPIO_PDERS=1<<n;
            break;
        default: //Floating
            p->GPIO_PUERC=1<<n;
            p->GPIO_PDERC=1<<n;
            break;
    }

    if(mm & SCHMITT_TRIG) p->GPIO_STERS=1<<n;
    else                  p->GPIO_STERC=1<<n;

    switch(mm & (OUTPUT | ALTERNATE))
    {
        case OUTPUT:
            p->GPIO_ODERS=1<<n;
            p->GPIO_GPERS=1<<n;
            break;
        case ALTERNATE:
            p->GPIO_GPERC=1<<n;
            break;
        default: //Input
            p->GPIO_ODERC=1<<n;
            p->GPIO_GPERS=1<<n;
            break;
    }
}

void GpioBase::afImpl(GpioPort *p, unsigned char n, char af)
{
    //'a' is 01100001, while 'A' is 01000001, so to map the 'a'..'h' / 'A'..'H'
    //range to 0..7 when considering only the lowest 3 bits subtract one.
    //The behavior for anything that's not in the 0..7 / 'a'..'h' / 'A'..'H' is
    //don't care for efficiency
    if(af>7) af--;

    if(af & 0b001) p->GPIO_PMR0S=1<<n;
    else           p->GPIO_PMR0C=1<<n;
    if(af & 0b010) p->GPIO_PMR1S=1<<n;
    else           p->GPIO_PMR1C=1<<n;
    if(af & 0b100) p->GPIO_PMR2S=1<<n;
    else           p->GPIO_PMR2C=1<<n;
}

void GpioBase::strengthImpl(GpioPort *p, unsigned char n, unsigned char s)
{
    if(s & 0b01) p->GPIO_ODCR0S=1<<n;
    else         p->GPIO_ODCR0C=1<<n;
    if(s & 0b10) p->GPIO_ODCR1S=1<<n;
    else         p->GPIO_ODCR1C=1<<n;
}

void GpioBase::slewImpl(GpioPort *p, unsigned char n, bool s)
{
    if(s) p->GPIO_OSRR0S=1<<n;
    else  p->GPIO_OSRR0C=1<<n;
}

} //namespace miosix

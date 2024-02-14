/***************************************************************************
 *   Copyright (C) 2016 by Silvano Seva                                    *
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

#ifndef HWMAPPING_H
#define	HWMAPPING_H

#include "interfaces/gpio.h"

namespace miosix {

namespace gpio {
    
    typedef Gpio<GPIOC_BASE,10> gpio0;
    typedef Gpio<GPIOA_BASE,15> gpio1;
    typedef Gpio<GPIOA_BASE,14> gpio2;    
    typedef Gpio<GPIOB_BASE,12> gpio3;
    
    typedef Gpio<GPIOC_BASE,0> ai0;
    typedef Gpio<GPIOC_BASE,1> ai1;
    typedef Gpio<GPIOC_BASE,2> ai2;
    typedef Gpio<GPIOC_BASE,3> ai3;
    typedef Gpio<GPIOC_BASE,4> ai4;
    typedef Gpio<GPIOC_BASE,5> ai5;
    typedef Gpio<GPIOB_BASE,0> ai6;
    typedef Gpio<GPIOB_BASE,1> ai7;
}        

namespace display {
       
    typedef Gpio<GPIOB_BASE,4> lcd_e;
    typedef Gpio<GPIOC_BASE,13> lcd_rs;
    typedef Gpio<GPIOA_BASE,7> lcd_d4;
    typedef Gpio<GPIOB_BASE,3> lcd_d5;
    typedef Gpio<GPIOC_BASE,12> lcd_d6;
    typedef Gpio<GPIOC_BASE,11> lcd_d7;    
}

namespace buttons {
    
    typedef Gpio<GPIOA_BASE,0> btn1;
    typedef Gpio<GPIOA_BASE,1> btn2;
    typedef Gpio<GPIOA_BASE,2> btn3;
    typedef Gpio<GPIOA_BASE,3> btn4;
    typedef Gpio<GPIOA_BASE,4> btn5;
    typedef Gpio<GPIOA_BASE,5> btn6;
    typedef Gpio<GPIOA_BASE,6> btn7;
}

namespace spi {
    
    typedef Gpio<GPIOB_BASE,13> sck;
    typedef Gpio<GPIOB_BASE,14> miso;
    typedef Gpio<GPIOB_BASE,15> mosi;
}

namespace i2c {
    //Is I2C2
    typedef Gpio<GPIOB_BASE,10> scl;
    typedef Gpio<GPIOB_BASE,11> sda;
}

namespace valves {
    
    typedef Gpio<GPIOC_BASE,6> valv1;
    typedef Gpio<GPIOC_BASE,7> valv2;
    typedef Gpio<GPIOC_BASE,8> valv3;
    typedef Gpio<GPIOC_BASE,9> valv4;
    typedef Gpio<GPIOB_BASE,6> valv5;
    typedef Gpio<GPIOB_BASE,7> valv6;
    typedef Gpio<GPIOB_BASE,8> valv7;
    typedef Gpio<GPIOB_BASE,9> valv8;    
}

// typedef Gpio<GPIOA_BASE,8> 
typedef Gpio<GPIOA_BASE,9> tx;
typedef Gpio<GPIOA_BASE,10> rx;
// typedef Gpio<GPIOA_BASE,11>
// typedef Gpio<GPIOA_BASE,12>
// typedef Gpio<GPIOA_BASE,13>
// typedef Gpio<GPIOB_BASE,2>
typedef Gpio<GPIOB_BASE,5> powerSw;
    
} //namespace miosix

#endif //HWMAPPING_H

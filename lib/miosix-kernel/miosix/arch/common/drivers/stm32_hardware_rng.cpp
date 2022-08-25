/***************************************************************************
 *   Copyright (C) 2013 by Terraneo Federico                               *
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

#include <stdexcept>
#include "interfaces/delays.h"
#include "stm32_hardware_rng.h"

using namespace std;

namespace miosix {

//
// class HardwareRng
//

HardwareRng& HardwareRng::instance()
{
    static HardwareRng singleton;
    return singleton;
}

unsigned int HardwareRng::get()
{
    miosix::Lock<miosix::FastMutex> l(mutex);
    PeripheralEnable pe;
    return getImpl();
}

void HardwareRng::get(void* buf, unsigned int size)
{
    unsigned char *buffer=reinterpret_cast<unsigned char*>(buf);
    Lock<FastMutex> l(mutex);
    PeripheralEnable pe;
    union Cast
    {
        unsigned int theInt;
        unsigned char theChar[4];
    };
    
    if(reinterpret_cast<unsigned int>(buffer) & 0x3)
    {
        Cast cast;
        cast.theInt=getImpl();
        int i=0;
        while(reinterpret_cast<unsigned int>(buffer) & 0x3)
        {
            if(size--==0) return; //May happen if buffer is small and unaligned
            *buffer++=cast.theChar[i++];
        }
    }
    
    unsigned int *aligned=reinterpret_cast<unsigned int*>(buffer);
    for(unsigned int i=0;i<size/4;i++) aligned[i]=getImpl();
        
    if(size & 0x3)
    {
        buffer+=(size/4)*4;
        Cast cast;
        cast.theInt=getImpl();
        int i=0;
        while(size & 0x3)
        {
            size--;
            *buffer++=cast.theChar[i++];
        }
    }
}

unsigned int HardwareRng::getImpl()
{
    #ifndef __NO_EXCEPTIONS
    for(int i=0;i<16;i++) //Try up to a reasonable # of times, then throw
    #else
    for(;;) //Can't return an error, keep retrying
    #endif
    {
        int timeout=1000000;
        unsigned int sr;
        while(--timeout>0) { sr=RNG->SR; if(sr & RNG_SR_DRDY) break; }
        if((sr & RNG_SR_SECS) || (sr & RNG_SR_CECS) || timeout<=0)
        {
            RNG->CR=0;
            delayUs(1);
            RNG->CR=RNG_CR_RNGEN;
            continue;
        }
        unsigned int result=RNG->DR;
        if(result==old) continue;
        old=result;
        return result;
    }
    #ifndef __NO_EXCEPTIONS
    throw runtime_error("RNG Fault detected");
    #endif //__NO_EXCEPTIONS
}

} //namespace miosix

/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012 by Terraneo Federico                   *
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

#include "interfaces/delays.h"

namespace miosix {

void delayMs(unsigned int mseconds)
{
    #ifndef __CODE_IN_XRAM

    #ifdef SYSCLK_FREQ_120MHz
    register const unsigned int count=29998;
    #else
    #warning "Delays are uncalibrated for this clock frequency"    
    #endif
    
    for(unsigned int i=0;i<mseconds;i++)
    {
        // This delay has been calibrated to take 1 millisecond
        // It is written in assembler to be independent on compiler optimization
        asm volatile("   mov   r1, #0     \n"
                     "1: cmp   r1, %0     \n"
                     "   itt   lo         \n"
                     "   addlo r1, r1, #1 \n"
                     "   blo   1b  \n"::"r"(count):"r1");
    }

    #else //__CODE_IN_XRAM
    #error "No delays"    
    #endif //__CODE_IN_XRAM
}

void delayUs(unsigned int useconds)
{
    #ifndef __CODE_IN_XRAM

    // This delay has been calibrated to take x microseconds
    // It is written in assembler to be independent on compiler optimization
    asm volatile("   mov   r1, #30    \n"
                 "   mul   r2, %0, r1 \n"
                 "   mov   r1, #0     \n"
                 "1: cmp   r1, r2     \n"
                 "   itt   lo         \n"
                 "   addlo r1, r1, #1 \n"
                 "   blo   1b  \n"::"r"(useconds):"r1","r2");

    #else //__CODE_IN_XRAM
    #error "No delays"
    #endif //__CODE_IN_XRAM
}

} //namespace miosix

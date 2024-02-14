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

#include "interfaces/delays.h"
#include "interfaces/arch_registers.h"

namespace miosix {

void delayMs(unsigned int mseconds)
{
    //This platform supports dynamic frequency scaling, two values: 120/26MHz
    //Note: delays were never tested agains an oscilloscope when the frequency
    //is 26MHz, so may not be accurate!
    register const unsigned int count=SystemCoreClock==120000000 ? 29999 : 6499;
    
    for(unsigned int i=0;i<mseconds;i++)
    {
        // This delay has been calibrated to take 1 millisecond
        // It is written in assembler to be independent on compiler optimization
        asm volatile("           mov   r1, #0     \n"
                     "___loop_m: cmp   r1, %0     \n"
                     "           itt   lo         \n"
                     "           addlo r1, r1, #1 \n"
                     "           blo   ___loop_m  \n"::"r"(count):"r1");
    }
}

void delayUs(unsigned int useconds)
{
    //This platform supports dynamic frequency scaling, two values: 120/26MHz
    //Note: delays were never tested agains an oscilloscope when the frequency
    //is 26MHz, so may not be accurate!
    if(SystemCoreClock==120000000)
    {
        // This delay has been calibrated to take x microseconds
        // It is written in assembler to be independent on compiler optimization
        asm volatile("           mov   r1, #30    \n"
                     "           mul   r2, %0, r1 \n"
                     "           mov   r1, #0     \n"
                     "___loop_u: cmp   r1, r2     \n"
                     "           itt   lo         \n"
                     "           addlo r1, r1, #1 \n"
                     "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2");
    } else {
        // This delay NEEDS CALIBRATION
        // It is written in assembler to be independent on compiler optimization
        asm volatile("           mov   r1, #6     \n"
                     "           mul   r2, %0, r1 \n"
                     "           mov   r1, #0     \n"
                     "___loop_k: nop              \n"
                     "           cmp   r1, r2     \n"
                     "           itt   lo         \n"
                     "           addlo r1, r1, #1 \n"
                     "           blo   ___loop_k  \n"::"r"(useconds):"r1","r2");
    }
}

} //namespace miosix

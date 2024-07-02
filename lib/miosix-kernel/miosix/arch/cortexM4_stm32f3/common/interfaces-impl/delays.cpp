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
    #ifdef SYSCLK_FREQ_72MHz
    #warning delayMs has not been calibrated yet with 72MHz clock
    register const unsigned int count=5350;
    #elif SYSCLK_FREQ_56MHz
    register const unsigned int count=5350;
    #elif SYSCLK_FREQ_48MHz
    register const unsigned int count=5350;
    #elif SYSCLK_FREQ_36MHz
    register const unsigned int count=4010;
    #elif SYSCLK_FREQ_24MHz
    register const unsigned int count=4010;
    #else // 8MHz clock
    register const unsigned int count=2000;
    #endif

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
    // This delay has been calibrated to take x microseconds
    // It is written in assembler to be independent on compiler optimization
    #ifdef SYSCLK_FREQ_72MHz
    #warning delayUs has not been calibrated yet with 72MHz clock
    asm volatile("           mov   r2, #166   \n"//Preloop, constant delay
                 "           mov   r1, #0     \n"
                 "__loop_u2: cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   __loop_u2  \n"
                 "           mov   r1, #5     \n"//Actual loop
                 "           mul   r2, %0, r1 \n"
                 "           mov   r1, #0     \n"
                 "___loop_u: nop              \n"
                 "           nop              \n"
                 "           cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2");
    #elif SYSCLK_FREQ_56MHz
    asm volatile("           mov   r2, #166   \n"//Preloop, constant delay
                 "           mov   r1, #0     \n"
                 "__loop_u2: cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   __loop_u2  \n"
                 "           mov   r1, #5     \n"//Actual loop
                 "           mul   r2, %0, r1 \n"
                 "           mov   r1, #0     \n"
                 "___loop_u: nop              \n"
                 "           nop              \n"
                 "           cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2");
    #elif SYSCLK_FREQ_48MHz
    asm volatile("           mov   r2, #166   \n"//Preloop, constant delay
                 "           mov   r1, #0     \n"
                 "__loop_u2: cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   __loop_u2  \n"
                 "           mov   r1, #5     \n"//Actual loop
                 "           mul   r2, %0, r1 \n"
                 "           mov   r1, #0     \n"
                 "___loop_u: nop              \n"
                 "           nop              \n"
                 "           cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2");
    #elif SYSCLK_FREQ_36MHz
    asm volatile("           mov   r1, #4     \n"
                 "           mul   r2, %0, r1 \n"
                 "           mov   r1, #0     \n"
                 "___loop_u: nop              \n"
                 "           nop              \n"
                 "           cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2");
    #elif SYSCLK_FREQ_24MHz
    asm volatile("           mov   r1, #4     \n"
                 "           mul   r2, %0, r1 \n"
                 "           mov   r1, #0     \n"
                 "           nop              \n"
                 "___loop_u: cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2");
    #else   // 8MHz clock
    asm volatile("           mov   r1, #2     \n"
                 "           mul   r2, %0, r1 \n"
                 "           mov   r1, #0     \n"
                 "___loop_u: cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2");
    #endif
}

} //namespace miosix

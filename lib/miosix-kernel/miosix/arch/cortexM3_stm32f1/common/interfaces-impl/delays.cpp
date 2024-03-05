/***************************************************************************
 *   Copyright (C) 2010 by Terraneo Federico                               *
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
    #ifdef SYSCLK_FREQ_72MHz
    register const unsigned int count=12000;
    #elif SYSCLK_FREQ_56MHz
    register const unsigned int count=11200;
    #elif SYSCLK_FREQ_48MHz
    register const unsigned int count=9600;
    #elif SYSCLK_FREQ_36MHz
    register const unsigned int count=7200;
    #elif SYSCLK_FREQ_24MHz
    register const unsigned int count=6000;
    #else
    register const unsigned int count=2016;
    #endif
    #else //__CODE_IN_XRAM
    #ifdef SYSCLK_FREQ_72MHz
    register const unsigned int count=1265;
    #elif SYSCLK_FREQ_56MHz
    register const unsigned int count=981;
    #elif SYSCLK_FREQ_48MHz
    register const unsigned int count=841;
    #elif SYSCLK_FREQ_36MHz
    register const unsigned int count=628;
    #elif SYSCLK_FREQ_24MHz
    register const unsigned int count=417;
    #else
    register const unsigned int count=103;
    #endif
    #endif //__CODE_IN_XRAM
    for(unsigned int i=0;i<mseconds;i++)
    {
        // This delay has been calibrated to take 1 millisecond
        // It is written in assembler to be independent on compiler optimizations
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
    // It is written in assembler to be independent on compiler optimizations
    #ifndef __CODE_IN_XRAM
    #ifdef SYSCLK_FREQ_72MHz
    asm volatile("           mov   r2, #14    \n"//Preloop, constant delay
                 "           mov   r1, #0     \n"
                 "__loop_u2: cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   __loop_u2  \n"
                 "           mov   r1, #8     \n"//Same delay as with 56MHz
                 "           mul   r2, %0, r1 \n"//No idea why, but works and
                 "           mov   r1, #0     \n"//is precise...
                 "___loop_u: nop              \n"
                 "           cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2");
    #elif SYSCLK_FREQ_56MHz
    asm volatile("           mov   r2, #10    \n"//Preloop, constant delay
                 "           mov   r1, #0     \n"
                 "__loop_u2: cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   __loop_u2  \n"
                 "           mov   r1, #8     \n"//Actual loop
                 "           mul   r2, %0, r1 \n"
                 "           mov   r1, #0     \n"
                 "___loop_u: nop              \n"
                 "           cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2");
    #elif SYSCLK_FREQ_48MHz
    asm volatile("           mov   r2, #6     \n"//Preloop, constant delay
                 "           mov   r1, #0     \n"
                 "__loop_u2: cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   __loop_u2  \n"
                 "           mov   r1, #7     \n"//Actual loop
                 "           mul   r2, %0, r1 \n"
                 "           mov   r1, #0     \n"
                 "___loop_u: nop              \n"
                 "           cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2");
    #elif SYSCLK_FREQ_36MHz
    asm volatile("           mov   r1, #5     \n"
                 "           mul   r2, %0, r1 \n"
                 "           mov   r1, #0     \n"
                 "___loop_u: nop              \n"
                 "           nop              \n"
                 "           cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2");
    #elif SYSCLK_FREQ_24MHz
    asm volatile("           mov   r1, #5     \n"
                 "           mul   r2, %0, r1 \n"
                 "           mov   r1, #0     \n"
                 "___loop_u: nop              \n"
                 "           cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2");
    #else
    asm volatile("           mov   r1, #2     \n"
                 "           mul   r2, %0, r1 \n"
                 "           mov   r1, #0     \n"
                 "___loop_u: cmp   r1, r2     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1","r2");
    #endif
    #else //__CODE_IN_XRAM
    #ifdef SYSCLK_FREQ_72MHz
    //These are not as precise as the ones with code running from FLASH
    asm volatile("           mov   r1, #0     \n"
                 "___loop_u: nop              \n"
                 "           cmp   r1, %0     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1");
    #elif SYSCLK_FREQ_56MHz
    asm volatile("           mov   r1, #0     \n"
                 "___loop_u: cmp   r1, %0     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1");
    #elif SYSCLK_FREQ_48MHz
    asm volatile("           lsr   %0, %0, 1  \n"
                 "           mov   r1, #0     \n"
                 "___loop_u: nop              \n"
                 "           nop              \n"
                 "           nop              \n"
                 "           cmp   r1, %0     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1");
    #elif SYSCLK_FREQ_36MHz
    asm volatile("           lsr   %0, %0, 1  \n"
                 "           mov   r1, #0     \n"
                 "___loop_u: nop              \n"
                 "           nop              \n"
                 "           cmp   r1, %0     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1");
    #elif SYSCLK_FREQ_24MHz
    asm volatile("           lsr   %0, %0, 1  \n"
                 "           mov   r1, #0     \n"
                 "___loop_u: cmp   r1, %0     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1");
    #else
    //This is extremely imprecise
    asm volatile("           lsr   %0, %0, 4  \n"
                 "           mov   r1, #0     \n"
                 "___loop_u: nop              \n"
                 "           cmp   r1, %0     \n"
                 "           itt   lo         \n"
                 "           addlo r1, r1, #1 \n"
                 "           blo   ___loop_u  \n"::"r"(useconds):"r1");
    #endif
    #endif //__CODE_IN_XRAM
}

} //namespace miosix


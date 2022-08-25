/***************************************************************************
 *   Copyright (C) 2013, 2014, 2015 by Terraneo Federico                   *
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

#ifndef ATOMIC_OPS_IMPL_H
#define	ATOMIC_OPS_IMPL_H

namespace miosix {

inline int atomicSwap(volatile int *p, int v)
{
    //This is the only atomic operation in the ARM7 assembler
    register int result;
    asm volatile("swp %0, %1, [%2]" : "=&r"(result) : "r"(v),"r"(p) : "memory");
    return result;
}

inline void atomicAdd(volatile int *p, int incr)
{
    register int a,b; //Temporaries used by ASM code
    asm volatile("    mrs   %0,     cpsr         \n"
                 "    tst   %0,     #0x80        \n"
                 "    orreq %1,     %0,    #0x80 \n"
                 "    msreq cpsr_c, %1           \n"
                 "    ldr   %1,     [%3]         \n"
                 "    add   %1,     %1,    %2    \n"
                 "    str   %1,     [%3]         \n"
                 "    tst   %0,     #0x80        \n"
                 "    msreq cpsr_c, %0           \n"
                 : "=&r"(a),"=&r"(b)
                 : "r"(incr),"r"(p)
                 : "cc","memory");
}

inline int atomicAddExchange(volatile int *p, int incr)
{
    register int a; //Temporaries used by ASM code
    register int result;
    asm volatile("    mrs   %0,     cpsr         \n"
                 "    tst   %0,     #0x80        \n"
                 "    orreq %1,     %0,    #0x80 \n"
                 "    msreq cpsr_c, %1           \n"
                 "    ldr   %1,     [%3]         \n"
                 "    add   %2,     %2,    %1    \n"
                 "    str   %2,     [%3]         \n"
                 "    tst   %0,     #0x80        \n"
                 "    msreq cpsr_c, %0           \n"
                 : "=&r"(a),"=&r"(result),"+&r"(incr)//Incr is read and clobbered
                 : "r"(p)
                 : "cc","memory");
    return result;
}

inline int atomicCompareAndSwap(volatile int *p, int prev, int next)
{
    register int a; //Temporaries used by ASM code
    register int result;
    asm volatile("    mrs   %0,     cpsr         \n"
                 "    tst   %0,     #0x80        \n"
                 "    orreq %1,     %0,    #0x80 \n"
                 "    msreq cpsr_c, %1           \n"
                 "    ldr   %1,     [%2]         \n"
                 "    cmp   %1,     %3           \n"
                 "    streq %4,     [%2]         \n"
                 "    tst   %0,     #0x80        \n"
                 "    msreq cpsr_c, %0           \n"
                 : "=&r"(a),"=&r"(result)
                 : "r"(p),"r"(prev),"r"(next)
                 : "cc","memory");
    return result;
}

inline void *atomicFetchAndIncrement(void * const volatile * p, int offset,
        int incr)
{
    register int a,b; //Temporaries used by ASM code
    register void *result;
    asm volatile("    mrs   %0,     cpsr             \n"
                 "    tst   %0,     #0x80            \n"
                 "    orreq %1,     %0,    #0x80     \n"
                 "    msreq cpsr_c, %1               \n"
                 "    ldr   %2,     [%3]             \n"
                 "    ldr   %1,     [%2, %4, asl #2] \n"
                 "    add   %1,     %1,    %5        \n"
                 "    str   %1,     [%2, %4, asl #2] \n"
                 "    tst   %0,     #0x80            \n"
                 "    msreq cpsr_c, %0               \n"
                 : "=&r"(a),"=&r"(b),"=&r"(result)
                 : "r"(p),"r"(offset),"r"(incr)
                 : "cc","memory");
    return result;
}

} //namespace miosix

#endif //ATOMIC_OPS_IMPL_H

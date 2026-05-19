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

#include "interfaces/arch_registers.h"

namespace miosix {

inline int atomicSwap(volatile int *p, int v)
{
    int result;
    volatile uint32_t *ptr=reinterpret_cast<volatile uint32_t*>(p);
    do {
        result=__LDREXW(ptr);
    } while(__STREXW(v,ptr));
    asm volatile("":::"memory");
    return result;
}

inline void atomicAdd(volatile int *p, int incr)
{
    int value;
    volatile uint32_t *ptr=reinterpret_cast<volatile uint32_t*>(p);
    do {
        value=__LDREXW(ptr);
    } while(__STREXW(value+incr,ptr));
    asm volatile("":::"memory");
}

inline int atomicAddExchange(volatile int *p, int incr)
{
    int result;
    volatile uint32_t *ptr=reinterpret_cast<volatile uint32_t*>(p);
    do {
        result=__LDREXW(ptr);
    } while(__STREXW(result+incr,ptr));
    asm volatile("":::"memory");
    return result;
}

inline int atomicCompareAndSwap(volatile int *p, int prev, int next)
{
    int result;
    volatile uint32_t *ptr=reinterpret_cast<volatile uint32_t*>(p);
    do {
        result=__LDREXW(ptr);
        if(result!=prev)
        {
            __CLREX();
            return result;
        }
    } while(__STREXW(next,ptr));
    asm volatile("":::"memory");
    return result;
}

inline void *atomicFetchAndIncrement(void * const volatile * p, int offset,
        int incr)
{
    void *result;
    volatile uint32_t *rcp;
    int rc;
    do {
        for(;;)
        {
            result=*p;
            if(result==0) return 0;
            rcp=reinterpret_cast<uint32_t*>(result)+offset;
            rc=__LDREXW(rcp);
            asm volatile("":::"memory");
            if(result==*p) break;
            __CLREX();
        }
    } while(__STREXW(rc+incr,rcp));
    asm volatile("":::"memory");
    return result;
}

} //namespace miosix

#endif //ATOMIC_OPS_IMPL_H

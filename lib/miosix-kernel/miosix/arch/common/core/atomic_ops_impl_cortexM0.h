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

#ifndef ATOMIC_OPS_IMPL_M0_H
#define	ATOMIC_OPS_IMPL_M0_H

/**
 * Cortex M0/M0+ architectures does not support __LDREXW, __STREXW and __CLREX
 * instructions, so we have to redefine the atomic operations using functions
 * that disable the interrupts.
 * 
 * TODO: actually this implementation is not very efficient
 * 
 */

#include "interfaces/arch_registers.h"
#include <kernel/kernel.h>

namespace miosix {


inline int atomicSwap(volatile int *p, int v)
{
    InterruptDisableLock dLock;
    
    int result = *p;
    *p = v;
    return result;
}

inline void atomicAdd(volatile int *p, int incr)
{
    InterruptDisableLock dLock;
    
    *p += incr;
}

inline int atomicAddExchange(volatile int *p, int incr)
{
    InterruptDisableLock dLock;
    
    int result = *p;
    *p += incr;
    return result;
}

inline int atomicCompareAndSwap(volatile int *p, int prev, int next)
{
    InterruptDisableLock dLock;
    
    int result = *p;
    if(*p == prev) *p = next;
    return result;
}

inline void *atomicFetchAndIncrement(void * const volatile * p, int offset,
        int incr)
{
    InterruptDisableLock dLock;
    
    volatile uint32_t *pt;
    
    void *result = *p;
    if(result == 0) return 0;
    pt = reinterpret_cast<uint32_t*>(result) + offset;
    *pt += incr;
    return result;
}

} //namespace miosix

#endif //ATOMIC_OPS_IMPL_M0_H

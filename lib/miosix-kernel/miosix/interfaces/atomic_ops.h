/***************************************************************************
 *   Copyright (C) 2013-2021 by Terraneo Federico                          *
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

#pragma once

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file atomic_ops.h
 * This file contains various atomic operations useful for implementing
 * lock-free algorithms.
 * 
 * For architectures without hardware support for these operations, they are
 * emulated by disabling interrupts. Note that these functions should be safe
 * to be called also with interrupts disabled, so implementations that disable
 * interrupts should be careful not to accidentally re-enable them if these
 * functions are called with interupts disabled.
 */

namespace miosix {

/**
 * Store a value in one memory location, and atomically read back the
 * previously stored value. Performs atomically the following operation:
 * 
 * \code
 * inline int atomicSwap(volatile int *p, int v)
 * {
 *      int result=*p;
 *      *p=v;
 *      return result;
 * }
 * \endcode
 * 
 * \param p pointer to memory location where the atomic swap will take place
 * \param v new value to be stored in *p
 * \return the previous value of *p
 */
inline int atomicSwap(volatile int *p, int v);

/**
 * Atomically read the content of a memory location, add a number to the loaded
 * value, and store the result. Performs atomically the following operation:
 * 
 * \code
 * inline void atomicAdd(volatile int *p, int incr)
 * {
 *      *p+=incr;
 * }
 * \endcode
 * 
 * \param p pointer to memory location where the atomic add will take place
 * \param incr value to be added to *p
 */
inline void atomicAdd(volatile int *p, int incr);

/**
 * Atomically read the content of a memory location, add a number to the loaded
 * value, store the result and return the previous value stored.
 * Performs atomically the following operation:
 * 
 * \code
 * inline int atomicAddExchange(volatile int *p, int incr)
 * {
 *      int result=*p;
 *      *p+=incr;
 *      return result;
 * }
 * \endcode
 * 
 * \param pointer to memory location where the atomic add will take place
 * \param incr value to be added to *p
 * \return the previous value of *p
 */
inline int atomicAddExchange(volatile int *p, int incr);

/**
 * Atomically read the value of a memory location, and store a new value in it
 * if it matches a given value. Also, return the previously stored value.
 * Performs atomically the following operation:
 * 
 * \code
 * inline int atomicCompareAndSwap(volatile int *p, int prev, int next)
 * {
 *      int result=*p;
 *      if(*p==prev) *p=next;
 *      return result;
 * }
 * \endcode
 * 
 * \param p pointer to the memory location to compare and swap
 * \param prev value to be compared against the content of *p
 * \param next value to be stored in *p if *p==prev
 * \return the value actually read from *p
 */
inline int atomicCompareAndSwap(volatile int *p, int prev, int next);

/**
 * An implementation of atomicFetchAndIncrement, as described in
 * http://www.drdobbs.com/atomic-reference-counting-pointers/184401888
 * Atomically read a pointer stored in one memory loaction, and add
 * a constant to a memory loaction placed at a given offset from the
 * pointer. Performs atomically the following operation:
 * 
 * \code
 * void *atomicFetchAndIncrement(void * const volatile *p, int offset, int incr)
 * {
 *      int *result=*p;
 *      if(result==0) return 0;
 *      *(result+offset)+=incr;
 *      return result;
 * }
 * \endcode
 * 
 * \param p pointer to a const volatile pointer to object. While p is not
 * subject to thread contention, *p is.
 * \param offset the memory location to increment is **p+offset*sizeof(int) 
 * \param incr value to be added to **p+offset*sizeof(int) 
 * \return *p
 */
inline void *atomicFetchAndIncrement(void * const volatile * p, int offset,
        int incr);

} //namespace miosix

/**
 * \}
 */

#ifdef _ARCH_ARM7_LPC2000
#include "core/atomic_ops_impl_arm7.h"
#elif defined(_ARCH_CORTEXM3_STM32F1) || defined(_ARCH_CORTEXM3_STM32F2) \
   || defined(_ARCH_CORTEXM4_STM32F4) || defined(_ARCH_CORTEXM3_STM32L1) \
   || defined(_ARCH_CORTEXM7_STM32F7) || defined(_ARCH_CORTEXM7_STM32H7) \
   || defined(_ARCH_CORTEXM3_EFM32GG) || defined(_ARCH_CORTEXM4_STM32F3) \
   || defined(_ARCH_CORTEXM4_STM32L4) || defined(_ARCH_CORTEXM4_ATSAM4L)
#include "core/atomic_ops_impl_cortexMx.h"
#elif defined(_ARCH_CORTEXM0_STM32F0)
#include "core/atomic_ops_impl_cortexM0.h"
#else
#error "No atomic ops for this architecture"
#endif

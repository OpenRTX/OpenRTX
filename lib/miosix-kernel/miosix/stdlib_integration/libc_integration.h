/***************************************************************************
 *   Copyright (C) 2008-2023 by Terraneo Federico                          *
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

#ifndef LIBC_INTEGRATION_H
#define	LIBC_INTEGRATION_H

#include <reent.h>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>

#ifndef COMPILING_MIOSIX
#error "This is header is private, it can't be used outside Miosix itself."
#error "If your code depends on a private header, it IS broken."
#endif //COMPILING_MIOSIX

namespace miosix {

/**
 * \internal
 * \return the heap high watermark. Note that the returned value is a memory
 * address and not a size in bytes. This is just an implementation detail, what
 * you'd want to call is most likely MemoryProfiling::getAbsoluteFreeHeap().
 */
unsigned int getMaxHeap();

/**
 * \internal
 * Used by the kernel during the boot process to switch the C standard library
 * from using a single global reentrancy structure to using per-thread
 * reentrancy structures
 * \param callback a function that return the per-thread reentrancy structure
 */
void setCReentrancyCallback(struct _reent *(*callback)());

static constexpr int nsPerSec = 1000000000;

/**
 * Convert from timespec to the Miosix representation of time
 * \param tp input timespec, must not be nullptr and be a valid pointer
 * \return Miosix nanoseconds
 */
inline long long timespec2ll(const struct timespec *tp)
{
    //NOTE: the cast is required to prevent overflow with older versions
    //of the Miosix compiler where tv_sec is int and not long long
    return static_cast<long long>(tp->tv_sec) * nsPerSec + tp->tv_nsec;
}

/**
 * Convert from the Miosix representation of time to a timespec
 * \param ns input Miosix nanoseconds
 * \param tp output timespec, must not be nullptr and be a valid pointer
 */
inline void ll2timespec(long long ns, struct timespec *tp)
{
    #ifdef __ARM_EABI__
    // Despite there being a single intrinsic, __aeabi_ldivmod, that computes
    // both the result of the / and % operator, GCC 9.2.0 isn't smart enough and
    // calls the intrinsic twice. This asm implementation takes ~188 cycles
    // instead of ~316 by calling it once. Sadly I had to use asm, as the
    // calling convention of the intrinsic appears to be nonstandard.
    // NOTE: actually a and b, by being 64 bit numbers, occupy register pairs
    register long long a asm("r0") = ns;
    register long long b asm("r2") = nsPerSec;
    // NOTE: clobbering lr to mark function not leaf due to the bl
    asm volatile("bl	__aeabi_ldivmod" : "+r"(a), "+r"(b) :: "lr");
    tp->tv_sec = a;
    tp->tv_nsec = static_cast<long>(b);
    #else //__ARM_EABI__
    #warning Warning POSIX time API not optimized for this platform
    tp->tv_sec = ns / nsPerSec;
    tp->tv_nsec = static_cast<long>(ns % nsPerSec);
    #endif //__ARM_EABI__
}

} //namespace miosix

#endif //LIBC_INTEGRATION_H

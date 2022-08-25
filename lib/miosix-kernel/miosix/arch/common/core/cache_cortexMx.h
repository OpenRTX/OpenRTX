/***************************************************************************
 *   Copyright (C) 2018 by Terraneo Federico                               *
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

#ifndef CACHE_CORTEX_MX_H
#define CACHE_CORTEX_MX_H

/*
 * README: Essentials about how cache support is implemented.
 * 
 * Caches in the Cortex M7 are transparent to software, except when using
 * the DMA. As the DMA reads and writes directly to memory, explicit management
 * is required. The default cache policy is write-back, but this has been deemed
 * unsuitable for Miosix, so for the time being only write-through is supported.
 * 
 * The IRQconfigureCache() configures the cache as write-through and enables it.
 * It should be called early at boot, in stage_1_boot.cpp
 * 
 * When writing DMA drivers, before passing a buffer to the DMA for it to be
 * written to a peripheral, call markBufferBeforeDmaWrite().
 * After a DMA read from a peripheral to a memory buffer has completed,
 * call markBufferAfterDmaRead(). These take care of keeping the DMA operations
 * in sync with the cache. These become no-ops for other architectures, so you
 * can freely put the in any driver.
 */

/*
 * Some more info about caches. Why not supporting write-back?
 * When writing data from memory to a peripheral using DMA, things are easy
 * also with write-back. You just flush (clean) the relevant cache lines, and
 * the DMA has access to the correct values. So it looks like it's ok.
 * When instead the DMA has to write to a memory location things become
 * complicated. Assume that a buffer not aligned to a cache line is passed to
 * a DMA read routine. After that a context switch happens and another thread
 * writes to a memory location that is on the same cache line as the (beginning
 * or end of) the buffer passed to the DMA. At the same time the DMA is writing
 * to the buffer.
 * At the end the situation looks like this, where the thread has written to
 * location X in the cache, while the DMA has written Y to the buffer.
 * <-- outside buffer --x-- buffer -->
 * +----------------------------------+
 * |   X                |             | cache
 * +----------------------------------+
 * |                    |YYYYYYYYYYYYY| memory
 * +----------------------------------+
 * What are you suppose to do? If you flush (clean) the cache line, X will be
 * committed to memory, but the Y data written by the DMA will be lost. If you
 * invalidate the cache, Y is preserved, but X is lost.
 * If you're just thinking that the problem can be solved by making sure buffers
 * are aligned to the cache line (and their size is a multiple of the cache
 * line), well, there's a problem.
 * Miosix is a real-time OS, and for performance and safety, most drivers are
 * zero copy. Applications routinely pass to DMA drivers such as the SDIO large
 * buffers (think 100+KB). Of course allocating an aligned buffer inside the
 * DMA driver as large as the user-passed buffer and copying the data there
 * isn't just slow, it wouldn't be safe, as you risk to exceed the free heap
 * memory or fragment the heap. Allocating a small buffer and splitting the
 * large DMA transfer in many small ones where the user passed buffer is copyied
 * one chunk at a time would be feasible, but even slower, and even more so
 * considering that some peripherals such as SD cards are optimized for large
 * sequential writes, not for small chunks.
 * But what if we make sure all buffers passed to DMA drivers are aligned?
 * That is a joke, as it the burden of doing so is unmaintainable. Buffers
 * passed to DMA memory are everywhere, in the C/C++ standard library
 * (think the buffer used for IO formatting inside printf/fprintf), and
 * everywherein application code. Something like
 * char s[128]="Hello world";
 * puts(s);
 * may cause s to be passed to a DMA driver. We would spend our lives chasing
 * unaligned buffers, and the risk of this causing difficult to reproduce memory
 * corruptions is too high. For this reason, for the time being, Miosix only
 * supports write-through caching on the Cortex-M7.
 * 
 * A note about performance. Using the testsuite benchmark, when caches are
 * disabled the STM32F746 @ 216MHz is less than half the speed of the
 * STM32F407 @ 168MHz. By enabling the ICACHE things get better, but it is
 * still slower, and achieves a speedup of 1.53 only when both ICACHE and
 * DCACHE are enabled. The speedup also includes the gains due to the faster
 * clock frequency. So if you want speed you have to use caches.
 */

#include "interfaces/arch_registers.h"

namespace miosix {

/**
 * To be called in stage_1_boot.cpp to configure caches.
 * Only call this function if the board has caches.
 * \param xramBase base address of external memory, if present, otherwise nullptr
 * \param xramSize size of external memory, if present, otherwise 0
 */
void IRQconfigureCache(const unsigned int *xramBase=nullptr, unsigned int xramSize=0);

/**
 * Call this function to mark a buffer before starting a DMA transfer where
 * the DMA will read the buffer.
 * \param buffer buffer
 * \param size buffer size
 */
inline void markBufferBeforeDmaWrite(const void *buffer, int size)
{
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT==1)
    // You may think that since the cache is configured as write-through,
    // there's nothing to do before the DMA can read a memory buffer just
    // written by the CPU, right? Wrong! Other than the cache, there's the
    // write buffer to worry about. My hypothesis is that once a memory region
    // is marked as cacheable, the write buffer becomes more lax in
    // automatically flushing as soon as possible. In the stm32 serial port
    // driver writing just a few characters causes garbage to be printed if
    // this __DSB() is removed. Apparently, the characters remian in the write
    // buffer.
    __DSB();
#endif
}

/**
 * Call this function after having completed a DMA transfer where the DMA has
 * written to the buffer.
 * \param buffer buffer
 * \param size buffer size
 */
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT==1)
void markBufferAfterDmaRead(void *buffer, int size);
#else
inline void markBufferAfterDmaRead(void *buffer, int size) {}
#endif

} //namespace miosix

#endif //CACHE_CORTEX_MX_H

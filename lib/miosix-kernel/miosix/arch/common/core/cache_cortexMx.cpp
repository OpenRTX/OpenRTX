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

#include "cache_cortexMx.h"
#include "mpu_cortexMx.h"
#include <utility>

using namespace std;

namespace miosix {

static const unsigned int cacheLine=32; //Cortex-M7 cache line size

/**
 * Using the MPU, configure a region of the memory space as
 * - write-through cacheable
 * - non-shareable
 * - readable/writable/executable only by privileged code (for compatibility
 *   with the way processes use the MPU)
 * \param region MPU region. Note that region 6 and 7 are used by processes, and
 * should be avoided here
 * \param base base address, aligned to a 32Byte cache line
 * \param size size, must be at least 32 and a power of 2, or it is rounded to
 * the next power of 2
 */
static void IRQconfigureCacheability(unsigned int region, unsigned int base,
                                     unsigned int size)
{
    // NOTE: The ARM documentation is unclear about the effect of the shareable
    // bit on a single core architecture. Experimental evidence on an STM32F476
    // shows that setting it in IRQconfigureCache for the internal RAM region
    // causes the boot to fail.
    // For this reason, all regions are marked as not shareable
    MPU->RBAR=(base & (~(cacheLine-1))) | MPU_RBAR_VALID_Msk | region;
    MPU->RASR=1<<MPU_RASR_AP_Pos //Privileged: RW, unprivileged: no access
               | MPU_RASR_C_Msk  //Cacheable, write through
               | 1               //Enable bit
               | sizeToMpu(size)<<1;
}

void IRQconfigureCache(const unsigned int *xramBase, unsigned int xramSize)
{
    IRQconfigureCacheability(0,0x00000000,0x20000000);
    IRQconfigureCacheability(1,0x20000000,0x20000000);
    if(xramSize)
        IRQconfigureCacheability(2,reinterpret_cast<unsigned int>(xramBase),xramSize);
    IRQenableMPUatBoot();
    
    SCB_EnableICache();
    SCB_EnableDCache();
}

#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT==1)
/**
 * Align a generic buffer to another one that contains the first one, but the
 * start size is aligned to a cache line
 */
static pair<uint32_t*, int32_t> alignBuffer(void *buffer, int size)
{
    auto bufferAddr=reinterpret_cast<unsigned int>(buffer);
    
    auto base=bufferAddr & (~(cacheLine-1));
    size+=bufferAddr-base;
    
    return make_pair(reinterpret_cast<uint32_t*>(base),size);
}

void markBufferAfterDmaRead(void *buffer, int size)
{
    //Since the current cache policy is write-through, we just invalidate the
    //cache lines corresponding to the buffer. No need to flush (clean) the cache.
    auto result=alignBuffer(buffer,size);
    SCB_InvalidateDCache_by_Addr(result.first,result.second);
}
#endif

} //namespace miosix

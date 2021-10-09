/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or functions from this file, or you compile this file   *
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

#include <memory_profiling.h>

#ifdef _MIOSIX

#include <miosix.h>

/*
 * Provide a C-callable wrapper for the corresponding miosix functions.
 */

unsigned int getStackSize()
{
    return miosix::MemoryProfiling::getStackSize();
}

unsigned int getAbsoluteFreeStack()
{
    return miosix::MemoryProfiling::getAbsoluteFreeStack();
}

unsigned int getCurrentFreeStack()
{
    return miosix::MemoryProfiling::getCurrentFreeStack();
}

unsigned int getHeapSize()
{
    return miosix::MemoryProfiling::getHeapSize();
}

unsigned int getAbsoluteFreeHeap()
{
    return miosix::MemoryProfiling::getAbsoluteFreeHeap();
}

unsigned int getCurrentFreeHeap()
{
    return miosix::MemoryProfiling::getCurrentFreeHeap();
}

#else

/*
 * No memory profiling is possible on x86/64 machines, thus all the functions
 * return 0.
 */

unsigned int getStackSize()
{
    return 0;
}

unsigned int getAbsoluteFreeStack()
{
    return 0;
}

unsigned int getCurrentFreeStack()
{
    return 0;
}

unsigned int getHeapSize()
{
    return 0;
}

unsigned int getAbsoluteFreeHeap()
{
    return 0;
}

unsigned int getCurrentFreeHeap()
{
    return 0;
}

#endif

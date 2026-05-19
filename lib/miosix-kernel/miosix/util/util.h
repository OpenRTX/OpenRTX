/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013 by Terraneo Federico *
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

/***********************************************************************
* util.h Part of the Miosix Embedded OS.
* A collection of "utilities".
************************************************************************/

#ifndef UTIL_H
#define UTIL_H

namespace miosix {

/**
 * \addtogroup Util
 * \{
 */

/**
 * This class allows to gather memory statistics useful when developing
 * embedded code.
 */
class MemoryProfiling
{
public:

    /**
     * Prints a summary of the information that can be gathered from this class.
     */
    static void print();

    /**
     * \return stack size of current thread.
     */
    static unsigned int getStackSize();

    /**
     * \return absolute free stack of current thread.<br>
     * Absolute free stack is the minimum free stack since the thread was
     * created.
     */
    static unsigned int getAbsoluteFreeStack();

    /**
     * \return current free stack  of current thread.<br>
     * Current free stack is the free stack at the moment when the this
     * function is called.
     */
    static unsigned int getCurrentFreeStack();

    /**
     * \return heap size which is defined in the linker script.<br>The heap is
     * shared among all threads, therefore this function returns the same value
     * regardless which thread is called in.
     */
    static unsigned int getHeapSize();

    /**
     * \return absolute (not current) free heap.<br>
     * Absolute free heap is the minimum free heap since the program started.
     * <br>The heap is shared among all threads, therefore this function returns
     * the same value regardless which thread is called in.
     */
    static unsigned int getAbsoluteFreeHeap();

    /**
     * \return current free heap.<br>
     * Current free heap is the free heap at the moment when the this
     * function is called.<br>
     * The heap is shared among all threads, therefore this function returns
     * the same value regardless which thread is called in.
     */
    static unsigned int getCurrentFreeHeap();

private:
    //All member functions static, disallow creating instances
    MemoryProfiling();
};

/**
 * Dump a memory area in this format
 * 0x00000000 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 | ................
 * \param start pointer to beginning of memory block to dump
 * \param len length of memory block to dump
 */
void memDump(const void *start, int len);

/**
 * \}
 */

} //namespace miosix

#endif //UTIL_H

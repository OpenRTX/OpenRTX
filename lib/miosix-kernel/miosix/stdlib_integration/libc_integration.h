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

#ifndef LIBC_INTEGRATION_H
#define	LIBC_INTEGRATION_H

#include <reent.h>
#include <cstdlib>
#include <cstring>

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

} //namespace miosix

#endif //LIBC_INTEGRATION_H

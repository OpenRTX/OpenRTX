/***************************************************************************
 *   Copyright (C) 2012-2021 by Terraneo Federico                          *
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

namespace miosix {

/**
 * \addtogroup Settings
 * \{
 */

/// \internal size of vector to store registers during ctx switch
/// ((10+16)*4=104Bytes). Only sp, r4-r11, EXC_RETURN and s16-s31 are saved
/// here, since r0-r3,r12,lr,pc,xPSR, old sp and s0-s15,fpscr are saved by
/// hardware on the process stack on Cortex M4F CPUs. EXC_RETURN, or the lr, 
/// value to use to return from the exception is necessary to know if the
/// thread has used fp regs, as an extension specific to Cortex-M4F CPUs.
const unsigned char CTXSAVE_SIZE=10+16;

/// \internal some architectures save part of the context on their stack.
/// ((8+17)*4=100Bytes). This constant is used to increase the stack size by
/// the size of context save frame. If zero, this architecture does not save
/// anything on stack during context save. Size is in bytes, not words.
///  8 registers=r0-r3,r12,lr,pc,xPSR
/// 17 registers=s0-s15,fpscr
/// MUST be divisible by 4.
const unsigned int CTXSAVE_ON_STACK=(8+17)*4;

/// \internal stack alignment for this specific architecture
const unsigned int CTXSAVE_STACK_ALIGNMENT=8;

/**
 * \}
 */

} //namespace miosix

/***************************************************************************
 *   Copyright (C) 2011-2021 by Terraneo Federico                          *
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

#ifdef _ARCH_ARM7_LPC2000
#include "core/endianness_impl_arm7.h"
#elif defined(_ARCH_CORTEXM0_STM32F0) || defined(_ARCH_CORTEXM3_STM32F1) \
   || defined(_ARCH_CORTEXM4_STM32F4) || defined(_ARCH_CORTEXM3_STM32F2) \
   || defined(_ARCH_CORTEXM3_STM32L1) || defined(_ARCH_CORTEXM7_STM32F7) \
   || defined(_ARCH_CORTEXM7_STM32H7) || defined(_ARCH_CORTEXM3_EFM32GG) \
   || defined(_ARCH_CORTEXM4_STM32F3) || defined(_ARCH_CORTEXM4_STM32L4) \
   || defined(_ARCH_CORTEXM4_ATSAM4L)
#include "core/endianness_impl_cortexMx.h"
#else
#error "No endianness code for this architecture"
#endif

/**
 * \addtogroup Interfaces
 * \{
 */

/**
 * \file endianness.h
 * This file contains optimized functions to convert data from the system's
 * endianness to little or big endian, as well as to perform byte swapping.
 */

// Implementation of these functions is in endianness_impl.h

#ifdef __cplusplus
#define __MIOSIX_INLINE inline
#else //__cplusplus
#define __MIOSIX_INLINE static inline
#endif //__cplusplus

/**
 * \fn inline unsigned short swapBytes16(unsigned short x)
 * \param x an short int
 * \return the same short with its bytes swapped
 */
__MIOSIX_INLINE unsigned short swapBytes16(unsigned short x);

/**
 * \fn inline unsigned int swapBytes32(unsigned int x)
 * \param x an int
 * \return the same int with its bytes swapped
 */
__MIOSIX_INLINE unsigned int swapBytes32(unsigned int x);

/**
 * \fn inline unsigned long long swapBytes64(unsigned long long x)
 * \param x a long long
 * \return the same long long with its bytes swapped
 */
__MIOSIX_INLINE unsigned long long swapBytes64(unsigned long long x);

#undef __MIOSIX_INLINE

/**
 * \def toLittleEndian16(x)
 * Convert a (signed or unsigned) short int from the system representation
 * to little endian
 * \param x value to convert
 * \return value converted to little endian
 *
 * \def toLittleEndian32(x)
 * Convert a (signed or unsigned) int from the system representation
 * to little endian
 * \param x value to convert
 * \return value converted to little endian
 *
 * \def toLittleEndian64(x)
 * Convert a (signed or unsigned) long long from the system representation
 * to little endian
 * \param x value to convert
 * \return value converted to little endian
 *
 * \def toBigEndian16(x)
 * Convert a (signed or unsigned) short int from the system representation
 * to big endian
 * \param x value to convert
 * \return value converted to big endian
 *
 * \def toBigEndian32(x)
 * Convert a (signed or unsigned) int from the system representation
 * to big endian
 * \param x value to convert
 * \return value converted to big endian
 *
 * \def toBigEndian64(x)
 * Convert a (signed or unsigned) long long from the system representation
 * to big endian
 * \param x value to convert
 * \return value converted to big endian
 */

#ifdef MIOSIX_LITTLE_ENDIAN
#define toLittleEndian16(x)   (x)
#define toLittleEndian32(x)   (x)
#define toLittleEndian64(x)   (x)
#define fromLittleEndian16(x) (x)
#define fromLittleEndian32(x) (x)
#define fromLittleEndian64(x) (x)
#define toBigEndian16(x)      swapBytes16(x)
#define toBigEndian32(x)      swapBytes32(x)
#define toBigEndian64(x)      swapBytes64(x)
#define fromBigEndian16(x)    swapBytes16(x)
#define fromBigEndian32(x)    swapBytes32(x)
#define fromBigEndian64(x)    swapBytes64(x)
#elif defined(MIOSIX_BIG_ENDIAN)
#define toLittleEndian16(x)   swapBytes16(x)
#define toLittleEndian32(x)   swapBytes32(x)
#define toLittleEndian64(x)   swapBytes64(x)
#define fromLittleEndian16(x) swapBytes16(x)
#define fromLittleEndian32(x) swapBytes32(x)
#define fromLittleEndian64(x) swapBytes64(x)
#define toBigEndian16(x)      (x)
#define toBigEndian32(x)      (x)
#define toBigEndian64(x)      (x)
#define fromBigEndian16(x)    (x)
#define fromBigEndian32(x)    (x)
#define fromBigEndian64(x)    (x)
#else
#error "endianness_impl.h does not define endianness"
#endif

/**
 * \}
 */

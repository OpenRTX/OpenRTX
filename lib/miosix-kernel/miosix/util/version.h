/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012, 2013, 2014 by Terraneo Federico       *
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

#ifndef VERSION_H
#define	VERSION_H

namespace miosix {

/**
 * \addtogroup Util
 * \{
 */

/**
 * Allows to know the version of the kernel at runtime.
 * \return a string with the kernel version.
 * The format is "Miosix vX.XX (board, builddate, compiler)" where
 * vX.XX is the kernel version number, like "v2.0"
 * board is the board name, like "stm32f103ze_stm3210e-eval"
 * builddate is the date the kernel was built, like "Oct 30 2011 00:58:10"
 * compiler is the compiler version, like "gcc 4.7.3"
 */
const char *getMiosixVersion();

/**
 * \}
 */

} //namespace miosix

#endif	/* VERSION_H */

/***************************************************************************
 *   Copyright (C) 2008 by Terraneo Federico                               *
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
* interrupts.h Part of the Miosix Embedded OS.
* Implementation of "Generic" interrupts, not related to a particular
* hardware driver.
************************************************************************/

#ifndef INTERRUPTS_H
#define INTERRUPTS_H

void default_IRQ_Routine()      __attribute__ ((interrupt("IRQ")));
extern "C" void FIQ_Routine()   __attribute__ ((interrupt("FIQ")));
extern "C" void UNDEF_Routine() __attribute__ ((interrupt("UNDEF")));
extern "C" void DABT_Routine()  __attribute__ ((interrupt("DABT")));
extern "C" void PABT_Routine()  __attribute__ ((interrupt("PABT")));

#endif //INTERRUPTS_H

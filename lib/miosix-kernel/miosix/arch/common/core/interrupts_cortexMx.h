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

#ifndef INTERRUPTS_H
#define	INTERRUPTS_H

/**
 * Called when an unexpected interrupt occurs.
 * It is called by stage_1_boot.cpp for all weak interrupts not defined.
 */
void unexpectedInterrupt();

/**
 * Possible kind of faults that the Cortex-M3 can report.
 * They are used to print debug information if a process causes a fault
 */
enum FaultType
{
    MP=1,          //Process attempted data access outside its memory
    MP_NOADDR=2,   //Process attempted data access outside its memory (missing addr)
    MP_XN=3,       //Process attempted code access outside its memory
    UF_DIVZERO=4,  //Process attempted to divide by zero
    UF_UNALIGNED=5,//Process attempted unaligned memory access
    UF_COPROC=6,   //Process attempted a coprocessor access
    UF_EXCRET=7,   //Process attempted an exception return
    UF_EPSR=8,     //Process attempted to access the EPSR
    UF_UNDEF=9,    //Process attempted to execute an invalid instruction
    UF_UNEXP=10,   //Unexpected usage fault
    HARDFAULT=11,  //Hardfault (for example process executed a BKPT instruction)
    BF=12,         //Busfault
    BF_NOADDR=13   //Busfault (missing addr)
};

#endif	//INTERRUPTS_H

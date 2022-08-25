/***************************************************************************
 *   Copyright (C) 2010 by Terraneo Federico                               *
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

#include "error.h"
#include "kernel.h"
#include "interfaces/portability.h"
#include "interfaces/bsp.h"
#include "logging.h"

namespace miosix {

void errorHandler(Error e)
{
    // Here we must be careful since this function can be called within an
    // interrupt routine, and disabling interrupts within an interrupt
    // routine must be avoided.
    bool interrupts=areInterruptsEnabled();
    if(interrupts) disableInterrupts();

    //Recoverable errors: None
    
    //Unrecoverable errors
    switch(e)
    {
        
        case OUT_OF_MEMORY:
            IRQerrorLog("\r\n***Out of memory\r\n");
            break;
        case STACK_OVERFLOW:
            IRQerrorLog("\r\n***Stack overflow\r\n");
            break;
        case UNEXPECTED:
            IRQerrorLog("\r\n***Unexpected error\r\n");
            break;
        case PAUSE_KERNEL_NESTING:
            IRQerrorLog("\r\n***Pause kernel nesting\r\n");
            break;
        case DISABLE_INTERRUPTS_NESTING:
            IRQerrorLog("\r\n***Disable interrupt nesting\r\n");
            break;
        case MUTEX_DEADLOCK:
            IRQerrorLog("\r\n***Deadlock\r\n");
            break;
        case NESTING_OVERFLOW:
            IRQerrorLog("\r\n***Nesting overflow\r\n"); 
            break;
        case INTERRUPTS_ENABLED_AT_BOOT:
            IRQerrorLog("\r\n***Interrupts enabled at boot\r\n");
            break;
        default:
            break;
    }
    miosix_private::IRQsystemReboot();

    //if(interrupts) enableInterrupts(); // Not needed since no recoverable errors
}

} //namespace miosix

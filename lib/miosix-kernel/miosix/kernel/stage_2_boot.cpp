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

/* *****************************************************
Miosix boot system
Stage 2 boot process
This code will initialize system peripherals, and will
start the kernel and filesystem.
***************************************************** */

#include <cstdio>
#include <stdexcept>
// Low level hardware functionalities
#include "interfaces/bsp.h"
#include "interfaces/os_timer.h"
#include "interfaces/deep_sleep.h"
// Miosix kernel
#include "kernel.h"
#include "filesystem/file_access.h"
#include "error.h"
#include "logging.h"
// settings for miosix
#include "config/miosix_settings.h"
#include "util/util.h"
#include "util/version.h"

using namespace std;

///<\internal Entry point for application code.
int main(int argc, char *argv[]);

namespace miosix {

/**
 * \internal
 * Calls C++ global constructors
 * \param start first function pointer to call
 * \param end one past the last function pointer to call
 */
static void callConstructors(unsigned long *start, unsigned long *end)
{
    for(unsigned long *i=start; i<end; i++)
    {
        void (*funcptr)();
        funcptr=reinterpret_cast<void (*)()>(*i);
        funcptr();
    }
}

void *mainLoader(void *argv)
{
    //If reaches here kernel is started, print Ok
    bootlog("Ok\n%s\n",getMiosixVersion());

    //Starting part of bsp that must be started after kernel
    bspInit2();

    //Initialize application C++ global constructors (called after boot)
    extern unsigned long __preinit_array_start asm("__preinit_array_start");
    extern unsigned long __preinit_array_end asm("__preinit_array_end");
    extern unsigned long __init_array_start asm("__init_array_start");
    extern unsigned long __init_array_end asm("__init_array_end");
    extern unsigned long _ctor_start asm("_ctor_start");
    extern unsigned long _ctor_end asm("_ctor_end");
    callConstructors(&__preinit_array_start, &__preinit_array_end);
    callConstructors(&__init_array_start, &__init_array_end);
    callConstructors(&_ctor_start, &_ctor_end);
    
    bootlog("OS Timer freq = %d Hz\n", internal::osTimerGetFrequency());
    bootlog("Available heap %d out of %d Bytes\n",
            MemoryProfiling::getCurrentFreeHeap(),
            MemoryProfiling::getHeapSize());
    
    //Run application code
    #ifdef __NO_EXCEPTIONS
    main(0,NULL);
    #else //__NO_EXCEPTIONS
    try {
        main(0,NULL);
    } catch(std::exception& e) {
        errorLog("***An exception propagated through a thread\n");
        errorLog("what():%s\n",e.what());
    } catch(...) {
        errorLog("***An exception propagated through a thread\n");
    }
    #endif //__NO_EXCEPTIONS
    
    //If main returns, shutdown
    shutdown();
    return 0;
}

} //namespace miosix

extern "C" void _init()
{
    using namespace miosix;

    //Initialize kernel C++ global constructors (called before boot)
    extern unsigned long __miosix_init_array_start asm("__miosix_init_array_start");
    extern unsigned long __miosix_init_array_end asm("__miosix_init_array_end");
    callConstructors(&__miosix_init_array_start, &__miosix_init_array_end);

    if(areInterruptsEnabled()) errorHandler(INTERRUPTS_ENABLED_AT_BOOT);
    IRQbspInit();
    internal::IRQosTimerInit();
    #ifdef WITH_DEEP_SLEEP
    IRQdeepSleepInit();
    #endif // WITH_DEEP_SLEEP
    //After IRQbspInit() serial port is initialized, so we can use IRQbootlog
    IRQbootlog("Starting Kernel... ");
    startKernel();
    //Never reach here (unless startKernel fails)
}

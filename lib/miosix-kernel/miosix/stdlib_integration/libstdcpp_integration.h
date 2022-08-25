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

#ifndef LIBSTDCPP_INTEGRATION_H
#define	LIBSTDCPP_INTEGRATION_H

//Can't yet make this header private, as it's included by kernel.h
//#ifndef COMPILING_MIOSIX
//#error "This is header is private, it can't be used outside Miosix itself."
//#error "If your code depends on a private header, it IS broken."
//#endif //COMPILING_MIOSIX

#include "kernel/error.h"

namespace __cxxabiv1
{
struct __cxa_exception; //A forward declaration of this one is enough

/*
 * This struct was taken from libsupc++/unwind-cxx.h Unfortunately that file
 * is not deployed in the gcc installation so we can't just #include it.
 * It is required on a per-thread basis to make C++ exceptions thread safe.
 */
struct __cxa_eh_globals
{
    __cxa_exception *caughtExceptions;
    unsigned int uncaughtExceptions;
    //Should be __ARM_EABI_UNWINDER__ but that's only usable inside gcc
    #ifdef __ARM_EABI__
    __cxa_exception* propagatingExceptions;
    #endif //__ARM_EABI__
};

} //namespace __cxxabiv1

namespace miosix {

//Forward declaration of a class to hide accessors to CppReentrancyData
class CppReentrancyAccessor;

/**
 * \internal
 * This is a wrapper class that contains all per-thread data required to make
 * C++ exceptions thread safe, irrespective of the ABI.
 */
class CppReentrancyData
{
public:
    /**
     * Constructor, initializes the exception related data to their default
     * value
     */
    CppReentrancyData()
    {
        eh_globals.caughtExceptions=0;
        eh_globals.uncaughtExceptions=0;
        #ifdef __ARM_EABI__
        eh_globals.propagatingExceptions=0;
        #else //__ARM_EABI__
        sjlj_ptr=0;
        #endif //__ARM_EABI__
    }

    /**
     * Destructor, checks that no memory is leaked (should never happen)
     */
    ~CppReentrancyData()
    {
        if(eh_globals.caughtExceptions!=0) errorHandler(UNEXPECTED);
    }

private:
    CppReentrancyData(const CppReentrancyData&);
    CppReentrancyData& operator=(const CppReentrancyData&);
    
    /// With the arm eabi unwinder, this is the only data structure required
    /// to make C++ exceptions threadsafe.
    __cxxabiv1::__cxa_eh_globals eh_globals;
    #ifndef __ARM_EABI__
    /// On other no arm architectures, it looks like gcc requires also this
    /// pointer to make the unwinder thread safe
    void *sjlj_ptr;
    #endif //__ARM_EABI__

    friend class CppReentrancyAccessor;
};

} //namespace miosix

#endif //LIBSTDCPP_INTEGRATION_H

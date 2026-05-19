/***************************************************************************
 *   Copyright (C) 2017 by Terraneo Federico                               *
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

#include <tuple>

namespace miosix {

/**
 * \internal
 * Template-independent code of unmember is here to reduce code size.
 * Do not call this function directly
 * \param mixedField either the function pointer for nonvirtual functions,
 * or the vtable offset (with 1 added to disambiguate it from the previous case)
 * for virtual functions
 * \param thisOffset the offset to add to this for multiple/virtual inheritance
 * \param o the object pointer that has to be passed as this
 * \retun the ordinary function pointer and void * to call it with
 */
std::tuple<void (*)(void*), void*> unmemberLogic(unsigned long mixedField,
    long thisOffset, unsigned long *o) noexcept;

/**
 * This function performs the forbidden cast of C++: casting from a member
 * function pointer of any class and a class pointer to an ordinary function
 * pointer and an opaque void * parameter to call it with. It allows to call
 * any member function of any class (that takes no parameters and returns void)
 * in an uniform and efficient way.
 * 
 * The code is not portable, as the underlying representation of member function
 * pointers is not specified by the standard. According to
 * https://www.codeproject.com/Articles/7150/Member-Function-Pointers-and-the-Fastest-Possible
 * there are multiple implementations.
 * This code has been tested with the GCC and the LLVM compilers, in both
 * X86, X86-64 and ARM32.
 * 
 * \param mfn a member function pointer of any class that takes no parameters
 * and returns void
 * \param object an object on which the member function has to be called
 * \return the ordinary function pointer and void * to call it with
 */
template<typename T>
std::tuple<void (*)(void*), void*> unmember(void (T::*mfn)(), T *object) noexcept
{
    //This code only works with GCC/LLVM
    #if !defined(__clang__) && !defined(__GNUC__)
    #error Unknown member function pointer layout
    #endif

    //A union is used to "inspect" the internals of the member function pointer
    union {
        void (T::*mfn)();
        struct {
            unsigned long mixedField;
            long thisOffset;
        };
    } unpack;
    unpack.mfn=mfn;

    //Unsigned long is used as its size is 4 on 32bit systems and 8 in 64bit
    unsigned long *o=reinterpret_cast<unsigned long*>(object);

    return unmemberLogic(unpack.mixedField,unpack.thisOffset,o);
}

} //namespace miosix

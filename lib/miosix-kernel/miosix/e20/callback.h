/***************************************************************************
 *   Copyright (C) 2012-2023 by Terraneo Federico                          *
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

#include <stdint.h>

namespace miosix {

/**
 * This class is to extract from Callback code that
 * does not depend on the template parameter N. 
 */
class CallbackBase
{
protected:
    /**
     * Possible operations performed by TypeDependentOperation::operation()
     */
    enum Op
    {
        CALL,
        ASSIGN,
        DESTROY
    };
    /**
     * This class is part of the any idiom used by Callback.
     */
    template<typename T>
    class TypeDependentOperation
    {
    public:
        /**
         * Perform the type-dependent operations
         * \param a storage for the any object, stores the function object
         * \param b storage for the source object for the copy constructor
         * \param op operation
         */
        static void operation(int32_t *a, const int32_t *b, Op op)
        {
            T *o1=reinterpret_cast<T*>(a);
            const T *o2=reinterpret_cast<const T*>(b);
            switch(op)
            {
                case CALL:
                    (*o1)();
                    break;
                case ASSIGN:
                    //This used to be simply *o1=*o2 when we were using
                    //tr1/functional, but in C++11 the type returned by bind
                    //due to having a move constructor doesn't like being
                    //assigned, only copy construction works so we have to
                    //use placement new
                    new (o1) T(*o2);
                    break;
                case DESTROY:
                    o1->~T();
                    break;
            }
        }
    };
};

/**
 * A Callback works just like an std::function, but has some additional
 * <b>limitations</b>. First, it can only accept function objects that take void
 * as a parameter and return void, and second if the size of the
 * implementation-defined type returned by bind is larger than N a
 * compile-time error is generated. Also, calling an empty Callback does
 * nothing, while doing the same on a function results in an exception
 * being thrown.
 * 
 * The reason why one would want to use this class is because, other than the
 * limitations, this class also offers a guarantee: it will never allocate
 * data on the heap. It is not just a matter of code speed: in Miosix calling
 * new/delete/malloc/free from an interrupt routine produces undefined
 * behaviour, so this class enables binding function calls form an interrupt
 * safely.
 * 
 * \param N the size in bytes that an instance of this class reserves to
 * store the function objects. If the line starting with 'typedef char check1'
 * starts failing it means it is time to increase this number. The size
 * of an instance of this object is N+sizeof(void (*)()), but with N rounded
 * by excess to four byte boundaries.
 */
template<unsigned N>
class Callback : private CallbackBase
{
public:
    /**
     * Default constructor. Produces an empty callback.
     */
    Callback() : operation(nullptr) {}

    /**
     * Constructor. Not explicit by design.
     * \param functor function object a copy of which is stored internally
     */
    template<typename T>
    Callback(T functor) : operation(nullptr)
    {
        *this=functor;
    }
    
    /**
     * Copy constructor
     * \param rhs object to copy
     */
    Callback(const Callback& rhs)
    {
        operation=rhs.operation;
        if(operation) operation(any,rhs.any,ASSIGN);
    }
    
    /**
     * Operator =
     * \param rhs object to copy
     * \return *this
     */
    Callback& operator= (const Callback& rhs);

    /**
     * Assignment operation, assigns a function object to this callback.
     * \param funtor function object a copy of which is stored internally
     */
    template<typename T>
    Callback& operator= (T functor);

    /**
     * Removes any function object stored in this class
     */
    void clear()
    {
        if(operation) operation(any,nullptr,DESTROY);
        operation=nullptr;
    }

    /**
     * Call the callback, or do nothing if no callback is set
     */
    void operator() ()
    {
        if(operation) operation(any,nullptr,CALL);
    }
    
    /**
     * Call the callback, generating undefined behaviour if no callback is set
     */
    void call()
    {
        operation(any,nullptr,CALL);
    }
    
    /**
     * \return true if the object contains a callback
     */
    explicit operator bool() const
    {
        return operation!=nullptr;
    }

    /**
     * Destructor
     */
    ~Callback()
    {
        if(operation) operation(any,nullptr,DESTROY);
    }

private:
    /// This declaration is done like that to save memory. On 32 bit systems
    /// the size of a pointer is 4 bytes, but the strictest alignment is 8
    /// which is that of double and long long. Using an array of doubles would
    /// have guaranteed alignment but the array size would have been a multiple
    /// of 8 bytes, and by summing the 4 bytes of the operation pointer there
    /// would have been 4 bytes of slack space left unused when declaring arrays
    /// of callbacks. Therefore any is declared as an array of ints but aligned
    /// to 8 bytes. This allows i.e. declaring Callback<20> with 20 bytes of
    /// useful storage and 4 bytes of pointer, despite 20 is not a multiple of 8
    int32_t any[(N+3)/4] __attribute__((aligned(8)));
    void (*operation)(int32_t *a, const int32_t *b, Op op);
};

template<unsigned N>
Callback<N>& Callback<N>::operator= (const Callback<N>& rhs)
{
    if(this==&rhs) return *this; //Handle assignmento to self
    if(operation) operation(any,nullptr,DESTROY);
    operation=rhs.operation;
    if(operation) operation(any,rhs.any,ASSIGN);
    return *this;
}

template<unsigned N>
template<typename T>
Callback<N>& Callback<N>::operator= (T functor)
{
    //If an error is reported about this line an attempt to store a too large
    //object is made. Increase N.
    static_assert(sizeof(any)>=sizeof(T),"");
    
    //This should not fail unless something has a stricter alignment than double
    static_assert(__alignof__(any)>=__alignof__(T),"");

    if(operation) operation(any,nullptr,DESTROY);

    new (reinterpret_cast<T*>(any)) T(functor);
    operation=TypeDependentOperation<T>::operation;
    return *this;
}

} //namespace miosix

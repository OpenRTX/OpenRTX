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

#include "unmember.h"
#include <cstdio>

using namespace std;

namespace miosix {

tuple<void (*)(void*), void*> unmemberLogic(unsigned long mixedField,
    long thisOffset, unsigned long *o) noexcept
{
    //ARM stores the "virtual function or not?" bit in bit #0 of thisOffset and
    //multiplies the offset by 2 to make room for it. Intel stores it in bit #0
    //of mixedField
    
    #ifdef __ARM_EABI__
    //With multiple or virtual inheritance we need to add an offset to this.
    o+=(thisOffset & 0xfffffffe)/2/sizeof(long);
    
    //Then we have two cases, we can have a member function pointer to a
    //virtual or nonvirtual function. For virtual functions mixedField is the
    //offset in the vtable where to find the function pointer.
    //For nonvirtual functions it is already the desired function pointer
    void (*result)(void*);
    if(thisOffset & 1)
    {
        //Pointer to virtual function. Dereference the object to get to the
        //virtual table, and then get the function pointer at the correct offset
        unsigned long *vtbl=reinterpret_cast<unsigned long *>(*o);
        result=reinterpret_cast<void (*)(void*)>(vtbl[mixedField/sizeof(long)]);
    } else {
        //Pointer to non virtual function
        result=reinterpret_cast<void (*)(void*)>(mixedField);
    }

    #elif defined(__i386) || defined(__x86_64__)
    //With multiple or virtual inheritance we need to add an offset to this.
    o+=thisOffset/sizeof(long);

    //Then we have two cases, we can have a member function pointer to a
    //virtual or nonvirtual function. For virtual functions mixedField is the
    //offset in the vtable where to find the function pointer.
    //For nonvirtual functions it is already the desired function pointer
    void (*result)(void*);
    if(mixedField & 1)
    {
        //Pointer to virtual function. Dereference the object to get to the
        //virtual table, and then get the function pointer at the correct offset
        unsigned long *vtbl=reinterpret_cast<unsigned long *>(*o);
        result=reinterpret_cast<void (*)(void*)>(vtbl[(mixedField-1)/sizeof(long)]);
    } else {
        //Pointer to non virtual function
        result=reinterpret_cast<void (*)(void*)>(mixedField);
    }
    #else
    #error The layout of virtual function pointer is unknown for this arch
    #endif

    return make_tuple(result,reinterpret_cast<void*>(o));
}

} //namespace miosix

//Testsuite for member function pointer implementation. Compile with:
// g++ -std=c++11 -O2 -DTEST_ALGORITHM -o test unmember.cpp; ./test
#ifdef TEST_ALGORITHM

#include <cstdio>

using namespace std;
using namespace miosix;

class Base
{
public:
            void m1() { printf("Base m1 %d %p\n",y,this); }
    virtual void m2() { printf("Base m2 %d %p\n",y,this); }
    virtual void m3() { printf("Base m3 %d %p\n",y,this); }
    int y=1234;
};

class Base2
{
public:
            void m4() { printf("Base2 m4 %d %p\n",x,this); }
    virtual void m5() { printf("Base2 m5 %d %p\n",x,this); }
    int x=5678;
};

class Derived : public Base
{
public:
    virtual void m3() { printf("Derived m3 %d %p\n",y,this); }
};

class Derived2 : public Base, public Base2 {};

class A                      { public: int a=1; };
class B : virtual public A   { public: int b=2; };
class C : virtual public A   { public: int c=3; void mf() { printf("%d\n",c); } };
class D : public B, public C { public: int d=4; };

void call(tuple<void (*)(void*), void*> a) { (*get<0>(a))(get<1>(a)); }

int main()
{
    Base b;
    Derived d;
    Derived2 d2;
    D dd;
    call(unmember(&Base::m1,&b));
    call(unmember(&Base::m2,&b));
    call(unmember(&Base::m3,&b));
    call(unmember(&Derived::m3,&d));
    call(unmember<Derived2>(&Derived2::m4,&d2));
    call(unmember<Derived2>(&Derived2::m5,&d2));
    call(unmember<D>(&D::mf,&dd));
}

#endif //TEST_ALGORITHM

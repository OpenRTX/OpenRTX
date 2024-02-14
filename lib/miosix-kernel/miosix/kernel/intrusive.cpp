/***************************************************************************
 *   Copyright (C) 2023 by Terraneo Federico                               *
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

#ifndef TEST_ALGORITHM

#include "intrusive.h"

#else //TEST_ALGORITHM

#include <iostream>
#include <cassert>

// Unused stubs as the test code only tests IntrusiveList
inline int atomicSwap(volatile int*, int) { return 0; }
void *atomicFetchAndIncrement(void *const volatile*, int, int) { return nullptr; }

//C++ glassbox testing trick
#define private public
#define protected public
#include "intrusive.h"
#undef private
#undef public

using namespace std;
using namespace miosix;

#endif //TEST_ALGORITHM

namespace miosix {

//
// class IntrusiveListBase
//

void IntrusiveListBase::push_back(IntrusiveListItem *item)
{
    #ifdef INTRUSIVE_LIST_ERROR_CHECK
    if((head!=nullptr) ^ (tail!=nullptr)) fail();
    if(!empty() && head==tail && (head->prev || head->next)) fail();
    if(item->prev!=nullptr || item->next!=nullptr) fail();
    #endif //INTRUSIVE_LIST_ERROR_CHECK
    if(empty()) head=item;
    else {
        item->prev=tail;
        tail->next=item;
    }
    tail=item;
}

void IntrusiveListBase::pop_back()
{
    #ifdef INTRUSIVE_LIST_ERROR_CHECK
    if(head==nullptr || tail==nullptr) fail();
    if(!empty() && head==tail && (head->prev || head->next)) fail();
    #endif //INTRUSIVE_LIST_ERROR_CHECK
    IntrusiveListItem *removedItem=tail;
    tail=removedItem->prev;
    if(tail!=nullptr)
    {
        tail->next=nullptr;
        removedItem->prev=nullptr;
    } else head=nullptr;
}

void IntrusiveListBase::push_front(IntrusiveListItem *item)
{
    #ifdef INTRUSIVE_LIST_ERROR_CHECK
    if((head!=nullptr) ^ (tail!=nullptr)) fail();
    if(!empty() && head==tail && (head->prev || head->next)) fail();
    if(item->prev!=nullptr || item->next!=nullptr) fail();
    #endif //INTRUSIVE_LIST_ERROR_CHECK
    if(empty()) tail=item;
    else {
        head->prev=item;
        item->next=head;
    }
    head=item;
}

void IntrusiveListBase::pop_front()
{
    #ifdef INTRUSIVE_LIST_ERROR_CHECK
    if(head==nullptr || tail==nullptr) fail();
    if(!empty() && head==tail && (head->prev || head->next)) fail();
    #endif //INTRUSIVE_LIST_ERROR_CHECK
    IntrusiveListItem *removedItem=head;
    head=removedItem->next;
    if(head!=nullptr)
    {
        head->prev=nullptr;
        removedItem->next=nullptr;
    } else tail=nullptr;
}

void IntrusiveListBase::insert(IntrusiveListItem *cur, IntrusiveListItem *item)
{
    #ifdef INTRUSIVE_LIST_ERROR_CHECK
    if((head!=nullptr) ^ (tail!=nullptr)) fail();
    if(!empty() && head==tail && (head->prev || head->next)) fail();
    if(cur!=nullptr)
    {
        if(cur->prev==nullptr && cur!=head) fail();
        if(cur->next==nullptr && cur!=tail) fail();
    }
    if(item->prev!=nullptr || item->next!=nullptr) fail();
    #endif //INTRUSIVE_LIST_ERROR_CHECK
    item->next=cur;
    if(cur!=nullptr)
    {
        item->prev=cur->prev;
        cur->prev=item;
    } else {
        item->prev=tail;
        tail=item;
    }
    if(item->prev!=nullptr) item->prev->next=item;
    else head=item;
}

IntrusiveListItem *IntrusiveListBase::erase(IntrusiveListItem *cur)
{
    #ifdef INTRUSIVE_LIST_ERROR_CHECK
    if(head==nullptr || tail==nullptr) fail();
    if(!empty() && head==tail && (head->prev || head->next)) fail();
    if(cur==nullptr) fail();
    if(cur->prev==nullptr && cur!=head) fail();
    if(cur->next==nullptr && cur!=tail) fail();
    #endif //INTRUSIVE_LIST_ERROR_CHECK
    if(cur->prev!=nullptr) cur->prev->next=cur->next;
    else head=cur->next;
    if(cur->next!=nullptr) cur->next->prev=cur->prev;
    else tail=cur->prev;
    auto result=cur->next;
    cur->prev=nullptr;
    cur->next=nullptr;
    return result;
}

#ifdef INTRUSIVE_LIST_ERROR_CHECK
#warning "INTRUSIVE_LIST_ERROR_CHECK should not be enabled in release builds"
void IntrusiveListBase::fail()
{
    #ifndef TEST_ALGORITHM
    errorHandler(UNEXPECTED);
    #else //TEST_ALGORITHM
    assert(false);
    #endif //TEST_ALGORITHM
}
#endif //INTRUSIVE_LIST_ERROR_CHECK

} //namespace miosix

//Testsuite for IntrusiveList. Compile with:
//g++ -DTEST_ALGORITHM -DINTRUSIVE_LIST_ERROR_CHECK -fsanitize=address -m32
//    -std=c++14 -Wall -O2 -o test intrusive.cpp; ./test
#ifdef TEST_ALGORITHM

void emptyCheck(IntrusiveListItem& x)
{
    //Glass box check
    assert(x.next==nullptr); assert(x.prev==nullptr);
}

void emptyCheck(IntrusiveList<IntrusiveListItem>& list)
{
    //Black box check
    assert(list.empty());
    assert(list.begin()==list.end());
    //Glass box check
    assert(list.head==nullptr);
    assert(list.tail==nullptr);
}

void oneItemCheck(IntrusiveList<IntrusiveListItem>& list, IntrusiveListItem& a)
{
    IntrusiveList<IntrusiveListItem>::iterator it;
    //Black box check
    assert(list.empty()==false);
    assert(list.front()==&a);
    assert(list.back()==&a);
    assert(list.begin()!=list.end());
    assert(*list.begin()==&a);
    assert(++list.begin()==list.end());
    it=list.begin(); it++; assert(it==list.end());
    assert(--list.end()==list.begin());
    it=list.end(); it--; assert(it==list.begin());
    //Glass box check
    assert(list.head==&a);
    assert(list.tail==&a);
    assert(a.prev==nullptr);
    assert(a.next==nullptr);
}

void twoItemCheck(IntrusiveList<IntrusiveListItem>& list, IntrusiveListItem& a,
                  IntrusiveListItem& b)
{
    IntrusiveList<IntrusiveListItem>::iterator it;
    //Black box check
    assert(list.empty()==false);
    assert(list.front()==&a);
    assert(list.back()==&b);
    assert(list.begin()!=list.end());
    it=list.begin();
    assert(*it++==&a);
    assert(*it++==&b);
    assert(it==list.end());
    it=list.begin();
    assert(*it==&a);
    ++it;
    assert(*it==&b);
    ++it;
    assert(it==list.end());
    it=list.end();
    it--;
    assert(*it==&b);
    it--;
    assert(*it==&a);
    assert(it==list.begin());
    it=list.end();
    assert(*--it==&b);
    assert(*--it==&a);
    assert(it==list.begin());
    //Glass box check
    assert(list.head==&a);
    assert(list.tail==&b);
    assert(a.prev==nullptr);
    assert(a.next==&b);
    assert(b.prev==&a);
    assert(b.next==nullptr);
}

void threeItemCheck(IntrusiveList<IntrusiveListItem>& list, IntrusiveListItem& a,
                  IntrusiveListItem& b, IntrusiveListItem& c)
{
    IntrusiveList<IntrusiveListItem>::iterator it;
    //Black box check
    assert(list.empty()==false);
    assert(list.front()==&a);
    assert(list.back()==&c);
    assert(list.begin()!=list.end());
    it=list.begin();
    assert(*it++==&a);
    assert(*it++==&b);
    assert(*it++==&c);
    assert(it==list.end());
    it=list.begin();
    assert(*it==&a);
    ++it;
    assert(*it==&b);
    ++it;
    assert(*it==&c);
    ++it;
    assert(it==list.end());
    it=list.end();
    it--;
    assert(*it==&c);
    it--;
    assert(*it==&b);
    it--;
    assert(*it==&a);
    assert(it==list.begin());
    it=list.end();
    assert(*--it==&c);
    assert(*--it==&b);
    assert(*--it==&a);
    assert(it==list.begin());
    //Glass box check
    assert(list.head==&a);
    assert(list.tail==&c);
    assert(a.prev==nullptr);
    assert(a.next==&b);
    assert(b.prev==&a);
    assert(b.next==&c);
    assert(c.prev==&b);
    assert(c.next==nullptr);
}

int main()
{
    IntrusiveListItem a,b,c;
    IntrusiveList<IntrusiveListItem> list;
    emptyCheck(a);
    emptyCheck(b);
    emptyCheck(c);
    emptyCheck(list);

    //
    // Testing push_back / pop_back
    //
    list.push_back(&a);
    oneItemCheck(list,a);
    list.push_back(&b);
    twoItemCheck(list,a,b);
    list.pop_back();
    oneItemCheck(list,a);
    emptyCheck(b);
    list.pop_back();
    emptyCheck(list);
    emptyCheck(a);

    //
    // Testing push_front / pop_front
    //
    list.push_front(&a);
    oneItemCheck(list,a);
    list.push_front(&b);
    twoItemCheck(list,b,a);
    list.pop_front();
    oneItemCheck(list,a);
    emptyCheck(b);
    list.pop_front();
    emptyCheck(list);
    emptyCheck(a);

    //
    // Testing insert / erase
    //
    list.insert(list.end(),&a);
    oneItemCheck(list,a);
    list.insert(list.end(),&b);
    twoItemCheck(list,a,b);
    list.erase(++list.begin()); //Erase second item first
    oneItemCheck(list,a);
    emptyCheck(b);
    list.erase(list.begin());   //Erase only item
    emptyCheck(list);
    emptyCheck(a);
    list.insert(list.begin(),&a);
    oneItemCheck(list,a);
    list.insert(list.begin(),&b);
    twoItemCheck(list,b,a);
    list.erase(list.begin());   //Erase first item first
    oneItemCheck(list,a);
    emptyCheck(b);
    list.erase(list.begin());   //Erase only item
    emptyCheck(list);
    emptyCheck(a);
    list.insert(list.end(),&a);
    oneItemCheck(list,a);
    list.insert(list.end(),&c);
    twoItemCheck(list,a,c);
    list.insert(++list.begin(),&b); //Insert in the middle
    threeItemCheck(list,a,b,c);
    list.erase(++list.begin());     //Erase in the middle
    twoItemCheck(list,a,c);
    emptyCheck(b);
    list.erase(list.begin());
    oneItemCheck(list,c);
    emptyCheck(a);
    list.erase(list.begin());
    emptyCheck(list);
    emptyCheck(c);

    //
    // Testing removeFast
    //
    assert(list.removeFast(&a)==false); //Not present, list empty
    emptyCheck(list);
    emptyCheck(a);
    list.push_front(&a);
    assert(list.removeFast(&b)==false); //Not present, list not empty
    oneItemCheck(list,a);
    emptyCheck(b);
    assert(list.removeFast(&a)==true); //Present, only element
    emptyCheck(list);
    emptyCheck(a);
    list.push_front(&c);
    list.push_front(&b);
    list.push_front(&a);
    assert(list.removeFast(&a)==true); //Present, at list head
    twoItemCheck(list,b,c);
    emptyCheck(a);
    list.push_front(&a);
    assert(list.removeFast(&b)==true); //Present, at in the middle
    twoItemCheck(list,a,c);
    emptyCheck(b);
    assert(list.removeFast(&c)==true); //Present, at list tail
    oneItemCheck(list,a);
    emptyCheck(c);
    list.pop_front(); //Just to end with empty list
    emptyCheck(list);
    emptyCheck(a);

    cout<<"Test passed"<<endl;
    return 0;
}

#endif //TEST_ALGORITHM

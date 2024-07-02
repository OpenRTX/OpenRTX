/***************************************************************************
 *   Copyright (C) 2013-2023 by Terraneo Federico                          *
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

#include <ostream>
#include <cstddef>
#include <cassert>
#include <type_traits>
#ifndef TEST_ALGORITHM
#include "interfaces/atomic_ops.h"
#include "error.h"
#endif //TEST_ALGORITHM

//Only enable when testing code that uses IntrusiveList
//#define INTRUSIVE_LIST_ERROR_CHECK

namespace miosix {

// Forward decls
template<typename T>
class intrusive_ref_ptr;

/**
 * Base class from which all intrusive objects derive, contains the
 * data that intrusive objects must have
 */
class Intrusive
{
protected:
    union
    {
        int referenceCount; ///< Used in IntrusiveRefCounted
        //Intrusive *next;    ///< Used by the cleanup list, if it will be done
    } intrusive;
    //virtual ~Intrusive() {} ///< To allow deleting from the cleanup list
};

/**
 * Derive from this class to support intrusive reference counting
 */
class IntrusiveRefCounted : public Intrusive
{
protected:
    /**
     * Constructor, initializes the reference count
     */
    IntrusiveRefCounted() { intrusive.referenceCount=0; }
    
    /**
     * Copy constructor 
     */
    IntrusiveRefCounted(const IntrusiveRefCounted&)
    {
        // The default copy constructor would have copied the reference count,
        // but that's wrong, a new object is being built, and its reference
        // count should be zero
        intrusive.referenceCount=0;
    }
    
    /**
     * Overload of operator=
     */
    IntrusiveRefCounted& operator=(const IntrusiveRefCounted&)
    {
        // The default copy constructor would have copied the reference count,
        // but that's wrong, as we have two distinct object (i.e, chunks of
        // memory to be managed), and their reference counts need to stay
        // separate, so do nothing in operator=
        return *this;
    }
    
    // No destructor

private:
    template<typename T>
    friend class intrusive_ref_ptr; ///< To access the reference count
};

/**
 * An implementation of 20.7.2.4 enable_shared_from_this for IntrusiveRefCounted
 * \param T this class uses the CRTP, just like enable_shared_from_this, so if
 * class Foo derives from IntrusiveRefCountedSharedFromThis, T has to be Foo.
 */
template<typename T>
class IntrusiveRefCountedSharedFromThis
{
public:
    /**
     * Constructor
     */
    IntrusiveRefCountedSharedFromThis()
    {
        static_assert(std::is_base_of<IntrusiveRefCounted,T>::value,"");
    }

    /**
     * \return an intrusive_ref_ptr to this. In this respect,
     * IntrusiveRefCounted also behaves like enable_shared_from_this
     */
    intrusive_ref_ptr<T> shared_from_this()
    {
        // Simply making an intrusive_ref_ptr from this works as the reference
        // count is intrusive
        #ifndef __NO_EXCEPTIONS
        T* result=dynamic_cast<T*>(this);
        assert(result);
        #else //__NO_EXCEPTIONS
        T* result=static_cast<T*>(this);
        #endif //__NO_EXCEPTIONS
        return intrusive_ref_ptr<T>(result);
    }
    
    /**
     * \return an intrusive_ref_ptr to this In this respect,
     * IntrusiveRefCounted also behaves like enable_shared_from_this
     */
    intrusive_ref_ptr<const T> shared_from_this() const
    {
        // Simply making an intrusive_ref_ptr from this works as the reference
        // count is intrusive
        #ifndef __NO_EXCEPTIONS
        const T* result=dynamic_cast<const T*>(this);
        assert(result);
        #else //__NO_EXCEPTIONS
        const T* result=static_cast<const T*>(this);
        #endif //__NO_EXCEPTIONS
        return intrusive_ref_ptr<const T>(result);
    }
    
    /**
     * Destructor
     */
    virtual ~IntrusiveRefCountedSharedFromThis();
};

template<typename T>
IntrusiveRefCountedSharedFromThis<T>::~IntrusiveRefCountedSharedFromThis() {}

/**
 * Reference counted pointer to intrusively reference counted objects.
 * This class is made in a way to resemble the shared_ptr class, as specified
 * in chapter 20.7.2.2 of the C++11 standard.
 * 
 * Thread safety of this class is the same as shared_ptr. The reference
 * count is updated using atomic operations, but this does not make
 * all kind of concurrent access to the same intrusive_ref_ptr safe.
 * A good reference that explains why is
 * www.drdobbs.com/cpp/a-base-class-for-intrusively-reference-c/229218807?pgno=3
 * 
 * \param T type to which the object points
 */
template<typename T>
class intrusive_ref_ptr
{
public:
    typedef T element_type; ///< As shared_ptr, expose the managed type
    
    /**
     * Default constructor
     */
    intrusive_ref_ptr() : object(nullptr) {}
    
    /**
     * Constructor, with raw pointer
     * \param object object to manage
     */
    explicit intrusive_ref_ptr(T *o) : object(o)
    {
        incrementRefCount();
    }

    /**
     * Generalized constructor, with raw pointer
     * \param object object to manage
     */
    template<typename U>
    explicit intrusive_ref_ptr(U *o) : object(o)
    {
        incrementRefCount();
        
        //Disallow polimorphic upcasting of non polimorphic classes,
        //as this will lead to bugs
        static_assert(std::has_virtual_destructor<T>::value,"");
    }
    
    /**
     * Copy constructor, with same type of managed pointer
     * \param rhs object to manage
     */
    intrusive_ref_ptr(const intrusive_ref_ptr& rhs) : object(rhs.object)
    {
        incrementRefCount();
    }
    
    /**
     * Generalized copy constructor, to support upcast among refcounted
     * pointers
     * \param rhs object to manage
     */
    template<typename U>
    intrusive_ref_ptr(const intrusive_ref_ptr<U>& rhs) : object(rhs.get())
    {
        incrementRefCount();
        
        //Disallow polimorphic upcasting of non polimorphic classes,
        //as this will lead to bugs
        static_assert(std::has_virtual_destructor<T>::value,"");
    }
    
    /**
     * Operator=, with same type of managed pointer
     * \param rhs object to manage
     * \return a reference to *this 
     */
    intrusive_ref_ptr& operator= (const intrusive_ref_ptr& rhs);
    
    /**
     * Generalized perator=, to support upcast among refcounted pointers
     * \param rhs object to manage
     * \return a reference to *this 
     */
    template<typename U>
    intrusive_ref_ptr& operator= (const intrusive_ref_ptr<U>& rhs);
    
    /**
     * Operator=, with raw pointer
     * \param rhs object to manage
     * \return a reference to *this 
     */
    intrusive_ref_ptr& operator= (T* o);
    
    /**
     * \return a pointer to the managed object 
     */
    T *get() const { return object; }

    /**
     * \return a reference to the managed object 
     */
    T& operator*() const { return *object; }
    
    /**
     * Allows this class to behave like the managed type
     * \return a pointer to the managed object
     */
    T *operator->() const { return object; }

    /**
     * \return true if the object contains a callback
     */
    explicit operator bool() const
    {
        return object!=nullptr;
    }
    
    /**
     * Swap the managed object with another intrusive_ref_ptr
     * \param rhs the other smart pointer
     */
    void swap(intrusive_ref_ptr& rhs) { std::swap(object,rhs.object); }
    
    /**
     * After a call to this member function, this intrusive_ref_ptr
     * no longer points to the managed object. The managed object is
     * deleted if this was the only pointer at it 
     */
    void reset()
    {
        if(decrementRefCount()) delete object;
        // Object needs to be set to nullptr regardless
        // of whether the object is deleted
        object=nullptr;
    }
    
    /**
     * \return the number of intrusive_ref_ptr that point to the managed object.
     * If return 0, than this points to nullptr 
     */
    int use_count() const
    {
        if(!object) return 0;
        return object->intrusive.referenceCount;
    }
    
    /**
     * \internal
     * This is just an implementation detail.
     * Use the free function atomic_load instead.
     * \return a copy of *this
     */
    intrusive_ref_ptr atomic_load() const;
    
    /**
     * \internal
     * This is just an implementation detail.
     * Use the free function atomic_store instead.
     * Note that this member function is not equivalent to the free function
     * with the same name, as it takes its argument by reference. This may
     * cause problems in some multithreaded use cases, so don't use it directly
     * \param r object to store to *this
     * \return the previous value stored in *this
     */
    intrusive_ref_ptr atomic_exchange(intrusive_ref_ptr& r);
    
    /**
     * Destructor
     */
    ~intrusive_ref_ptr() { reset(); }
private:
    
    /**
     * Increments the reference count
     */
    void incrementRefCount()
    {
        if(object) atomicAdd(&object->intrusive.referenceCount,1);
    }
    
    /**
     * Decrements the reference count
     * \return true if the object has to be deleted 
     */
    bool decrementRefCount()
    {
        if(object==nullptr) return false;
        return atomicAddExchange(&object->intrusive.referenceCount,-1)==1;
    }

    T *object; ///< The managed object
};

template<typename T>
intrusive_ref_ptr<T>& intrusive_ref_ptr<T>::operator=
        (const intrusive_ref_ptr<T>& rhs)
{
    if(*this==rhs) return *this; //Handle assignment to self
    if(decrementRefCount()) delete object;
    object=rhs.object;
    incrementRefCount();
    return *this;
}

template<typename T> template<typename U>
intrusive_ref_ptr<T>& intrusive_ref_ptr<T>::operator=
        (const intrusive_ref_ptr<U>& rhs)
{
    if(*this==rhs) return *this; //Handle assignment to self
    if(decrementRefCount()) delete object;
    object=rhs.get();
    incrementRefCount();
    
    //Disallow polimorphic upcasting of non polimorphic classes,
    //as this will lead to bugs
    static_assert(std::has_virtual_destructor<T>::value,"");
    
    return *this;
}

template<typename T>
intrusive_ref_ptr<T>& intrusive_ref_ptr<T>::operator= (T* o)
{
    if(decrementRefCount()) delete object;
    object=o;
    incrementRefCount();
    return *this;
}

template<typename T>
intrusive_ref_ptr<T> intrusive_ref_ptr<T>::atomic_load() const
{
    intrusive_ref_ptr<T> result; // This gets initialized with nullptr
    
    // According to the C++ standard, this causes undefined behaviour if
    // T has virtual functions, but GCC (and clang) have an implementation
    // based on a compiler intrinsic that does the right thing: it produces
    // the correct answer together with a compiler warning for classes with
    // virtual destructors, and even for classes using multiple inheritance.
    // It (obviously) fails for classes making use of virtual inheritance, but
    // in this case the warning is promoted to an error. The last thing to
    // fix is to remove the warning using the #pragma
    // As a result, intrusive_ref_ptr can't be used with classes that have
    // virtual base classes, but that's an acceptable limitation, especially
    // considering that you get a meaningful compiler error if accidentally
    // trying to use it in such a case.
    #pragma GCC diagnostic ignored "-Winvalid-offsetof"
    const int offsetBytes=offsetof(T,intrusive.referenceCount);
    #pragma GCC diagnostic pop
    
    // Check that referenceCount is properly aligned for the following code to work
    static_assert((offsetBytes % sizeof(int))==0, "");
    
    const int offsetInt=offsetBytes/sizeof(int);
    void * const volatile * objectPtrAddr=
            reinterpret_cast<void * const volatile *>(&object);
    void *loadedObject=atomicFetchAndIncrement(objectPtrAddr,offsetInt,1);
    // This does not increment referenceCount, as it was done before
    result.object=reinterpret_cast<T*>(loadedObject);
    return result;
}

template<typename T>
intrusive_ref_ptr<T> intrusive_ref_ptr<T>::atomic_exchange(
        intrusive_ref_ptr<T>& r)
{
    //Note: this is safe with respect to assignment to self
    T *temp=r.object;
    if(temp) atomicAdd(&temp->intrusive.referenceCount,1);
    
    // Check that the following reinterpret_casts will work as intended.
    // This also means that this code won't work on 64bit machines but for
    // Miosix this isn't a problem for now.
    static_assert(sizeof(void*)==sizeof(int),"");
    
    int tempInt=reinterpret_cast<int>(temp);
    volatile int *objectAddrInt=reinterpret_cast<volatile int*>(&object);
    temp=reinterpret_cast<T*>(atomicSwap(objectAddrInt,tempInt));
    
    intrusive_ref_ptr<T> result; // This gets initialized with nullptr
    // This does not increment referenceCount, as the pointer was swapped
    result.object=temp;
    return result;
}

/**
 * Operator==
 * \param a first pointer
 * \param b second pointer
 * \return true if they point to the same object
 */
template<typename T, typename U>
bool operator==(const intrusive_ref_ptr<T>& a, const intrusive_ref_ptr<U>& b)
{
    return a.get()==b.get();
}

template<typename T, typename U>
bool operator==(const T *a, const intrusive_ref_ptr<U>& b)
{
    return a==b.get();
}

template<typename T, typename U>
bool operator==(const intrusive_ref_ptr<T>& a, const T *b)
{
    return a.get()==b;
}

/**
 * Operator!=
 * \param a first pointer
 * \param b second pointer
 * \return true if they point to different objects
 */
template<typename T, typename U>
bool operator!=(const intrusive_ref_ptr<T>& a, const intrusive_ref_ptr<U>& b)
{
    return a.get()!=b.get();
}

template<typename T, typename U>
bool operator!=(const T *a, const intrusive_ref_ptr<U>& b)
{
    return a!=b.get();
}

template<typename T, typename U>
bool operator!=(const intrusive_ref_ptr<T>& a, const T *b)
{
    return a.get()!=b;
}

/**
 * Operator<, allows to create containers of objects
 * \param a first pointer
 * \param b second pointer
 * \return true if a.get() < b.get()
 */
template<typename T, typename U>
bool operator<(const intrusive_ref_ptr<T>& a, const intrusive_ref_ptr<U>& b)
{
    return a.get()<b.get();
}

template<typename T, typename U>
bool operator<(const T *a, const intrusive_ref_ptr<U>& b)
{
    return a<b.get();
}

template<typename T, typename U>
bool operator<(const intrusive_ref_ptr<T>& a, const T *b)
{
    return a.get()<b;
}

/**
 * Operator<<, allows printing of the pointer value on an ostream
 * \param os ostream where to print the pointer value
 * \param p intrusive_ref_ptr to print
 * \return os
 */
template<typename T>
std::ostream& operator<<(std::ostream& os, const intrusive_ref_ptr<T>& p)
{
    os<<p.get();
    return os;
}

/**
 * Performs static_cast between intrusive_ref_ptr
 * \param r intrusive_ref_ptr of source type
 * \return intrusive_ref_ptr of destination type
 */
template<typename T, typename U>
intrusive_ref_ptr<T> static_pointer_cast(const intrusive_ref_ptr<U>& r)
{
    // Note: 20.7.2.2.9 says this expression done with shared_ptr causes
    // undefined behaviour, however this works with intrusive_ref_ptr
    // as the reference count is intrusive, so the counter isn't duplicated.
    // To ease porting of code to and from shared_ptr it is recomended
    // to use static_pointer_cast, anyway.
    return intrusive_ref_ptr<T>(static_cast<T*>(r.get()));
}

/**
 * Performs dynamic_cast between intrusive_ref_ptr
 * \param r intrusive_ref_ptr of source type
 * \return intrusive_ref_ptr of destination type
 */
template<typename T, typename U>
intrusive_ref_ptr<T> dynamic_pointer_cast(const intrusive_ref_ptr<U>& r)
{
    // Note: 20.7.2.2.9 says this expression done with shared_ptr causes
    // undefined behaviour, however this works with intrusive_ref_ptr
    // as the reference count is intrusive, so the counter isn't duplicated.
    // To ease porting of code to and from shared_ptr it is recomended
    // to use static_pointer_cast, anyway.
    return intrusive_ref_ptr<T>(dynamic_cast<T*>(r.get()));
}

/**
 * Performs const_cast between intrusive_ref_ptr
 * \param r intrusive_ref_ptr of source type
 * \return intrusive_ref_ptr of destination type
 */
template<typename T, typename U>
intrusive_ref_ptr<T> const_pointer_cast(const intrusive_ref_ptr<U>& r)
{
    // Note: 20.7.2.2.9 says this expression done with shared_ptr causes
    // undefined behaviour, however this works with intrusive_ref_ptr
    // as the reference count is intrusive, so the counter isn't duplicated.
    // To ease porting of code to and from shared_ptr it is recomended
    // to use static_pointer_cast, anyway.
    return intrusive_ref_ptr<T>(const_cast<T*>(r.get()));
}

/**
 * Allows concurrent access to an instance of intrusive_ref_ptr.
 * Multiple threads can cooncurrently perform atomic_load(), atomic_store()
 * and atomic_exchange() on the same intrusive_ref_ptr. Any other concurent
 * access not protected by explicit locking (such as threads calling reset(),
 * or using the copy constructor, or deleting the intrusive_ref_ptr) yields
 * undefined behaviour.
 * \param p pointer to an intrusive_ref_ptr shared among threads
 * \return *p, atomically fetching the pointer and incrementing the reference
 * count
 */
template<typename T>
intrusive_ref_ptr<T> atomic_load(const intrusive_ref_ptr<T> *p)
{
    if(p==nullptr) return intrusive_ref_ptr<T>();
    return p->atomic_load();
}

/**
 * Allows concurrent access to an instance of intrusive_ref_ptr.
 * Multiple threads can cooncurrently perform atomic_load(), atomic_store()
 * and atomic_exchange() on the same intrusive_ref_ptr. Any other concurent
 * access not protected by explicit locking (such as threads calling reset(),
 * or using the copy constructor, or deleting the intrusive_ref_ptr) yields
 * undefined behaviour.
 * \param p pointer to an intrusive_ref_ptr shared among threads
 * \param r intrusive_ref_ptr that will be stored in *p
 */
template<typename T>
void atomic_store(intrusive_ref_ptr<T> *p, intrusive_ref_ptr<T> r)
{
    if(p) p->atomic_exchange(r);
}

/**
 * Allows concurrent access to an instance of intrusive_ref_ptr.
 * Multiple threads can cooncurrently perform atomic_load(), atomic_store()
 * and atomic_exchange() on the same intrusive_ref_ptr. Any other concurent
 * access not protected by explicit locking (such as threads calling reset(),
 * or using the copy constructor, or deleting the intrusive_ref_ptr) yields
 * undefined behaviour.
 * \param p pointer to an intrusive_ref_ptr shared among threads
 * \param r value to be stored in *p
 * \return the previous value of *p
 */
template<typename T>
intrusive_ref_ptr<T> atomic_exchange(intrusive_ref_ptr<T> *p,
        intrusive_ref_ptr<T> r)
{
    if(p==nullptr) return intrusive_ref_ptr<T>();
    return p->atomic_exchange(r);
}

//Forward declarations
class IntrusiveListBase;
template<typename T>
class IntrusiveList;

/**
 * Base class from which all items to be put in an IntrusiveList must derive,
 * contains the next and prev pointer that create the list
 */
class IntrusiveListItem
{
private:
    IntrusiveListItem *next=nullptr;
    IntrusiveListItem *prev=nullptr;

    friend class IntrusiveListBase;
    template<typename T>
    friend class IntrusiveList;
};

/**
 * \internal
 * Base class of IntrusiveList with the non-template-dependent part to improve
 * code size when instantiationg multiple IntrusiveLists
 */
class IntrusiveListBase
{
protected:
    IntrusiveListBase() : head(nullptr), tail(nullptr) {}

    void push_back(IntrusiveListItem *item);

    void pop_back();

    void push_front(IntrusiveListItem *item);

    void pop_front();

    void insert(IntrusiveListItem *cur, IntrusiveListItem *item);

    IntrusiveListItem *erase(IntrusiveListItem *cur);

    IntrusiveListItem* front() { return head; }

    IntrusiveListItem* back() { return tail; }

    bool empty() const { return head==nullptr; }

    #ifdef INTRUSIVE_LIST_ERROR_CHECK
    static void fail();
    #endif //INTRUSIVE_LIST_ERROR_CHECK

private:
    IntrusiveListItem *head;
    IntrusiveListItem *tail;
};

/**
 * A doubly linked list that only accepts objects that derive from
 * IntrusiveListItem.
 * 
 * Compared to std::list, this class offers the guarantee that no dynamic memory
 * allocation is performed. Differently from std::list, objects are not copied
 * when put in the list, so this is a non-owning container. For this reason,
 * this class unlike std::list accepts objects by pointer instead of reference
 * in member functions like insert() or push_front(), and returns objects by
 * pointer when dereferncing iterators or in member functions like front().
 * The caller is thus responsible for managing the lifetime of objects put in
 * this list.
 */
template<typename T>
class IntrusiveList : private IntrusiveListBase
{
public:
    /**
     * Intrusive list iterator type
     */
    class iterator
    {
    public:
        iterator() : list(nullptr), cur(nullptr) {}

        T* operator*()
        {
            #ifdef INTRUSIVE_LIST_ERROR_CHECK
            if(list==nullptr || cur==nullptr) IntrusiveListBase::fail();
            #endif //INTRUSIVE_LIST_ERROR_CHECK
            return static_cast<T*>(cur);
        }

        iterator operator++()
        {
            #ifdef INTRUSIVE_LIST_ERROR_CHECK
            if(list==nullptr || cur==nullptr) IntrusiveListBase::fail();
            #endif //INTRUSIVE_LIST_ERROR_CHECK
            cur=cur->next; return *this;
        }

        iterator operator--()
        {
            #ifdef INTRUSIVE_LIST_ERROR_CHECK
            if(list==nullptr || list->empty()) IntrusiveListBase::fail();
            #endif //INTRUSIVE_LIST_ERROR_CHECK
            if(cur!=nullptr) cur=cur->prev;
            else cur=list->IntrusiveListBase::back(); //Special case: decrementing end()
            return *this;
        }

        iterator operator++(int)
        {
            #ifdef INTRUSIVE_LIST_ERROR_CHECK
            if(list==nullptr || cur==nullptr) IntrusiveListBase::fail();
            #endif //INTRUSIVE_LIST_ERROR_CHECK
            iterator result=*this;
            cur=cur->next;
            return result;
        }

        iterator operator--(int)
        {
            #ifdef INTRUSIVE_LIST_ERROR_CHECK
            if(list==nullptr || list->empty()) IntrusiveListBase::fail();
            #endif //INTRUSIVE_LIST_ERROR_CHECK
            iterator result=*this;
            if(cur!=nullptr) cur=cur->prev;
            else cur=list->IntrusiveListBase::back(); //Special case: decrementing end()
            return result;
        }

        bool operator==(const iterator& rhs) { return cur==rhs.cur; }
        bool operator!=(const iterator& rhs) { return cur!=rhs.cur; }

    private:
        iterator(IntrusiveList<T> *list, IntrusiveListItem *cur)
            : list(list), cur(cur) {}

        IntrusiveList<T> *list;
        IntrusiveListItem *cur;

        friend class IntrusiveList<T>;
    };
    
    /**
     * Constructor, produces an empty list
     */
    IntrusiveList() {}

    /**
     * Disabled copy constructor and operator=
     * Since intrusive lists do not store objects by value, and an item can
     * only belong to at most one list, intrusive lists are not copyable.
     */
    IntrusiveList(const IntrusiveList&)=delete;
    IntrusiveList& operator=(const IntrusiveList&)=delete;
    
    /**
     * Adds item to the end of the list
     * \param item item to add
     */
    void push_back(T *item) { IntrusiveListBase::push_back(item); }
    
    /**
     * Removes the last element in the list
     */
    void pop_back() { IntrusiveListBase::pop_back(); }
    
    /**
     * Adds item to the front of the list
     * \param item item to add
     */
    void push_front(T *item) { IntrusiveListBase::push_front(item); }
    
    /**
     * Removes the first item of the list
     */
    void pop_front() { IntrusiveListBase::pop_front(); }
    
    /**
     * Inserts the given item before the position indicated by the iterator
     * \param it position where to insert the item
     * \param item item to insert
     */
    void insert(iterator it, T *item)
    {
        #ifdef INTRUSIVE_LIST_ERROR_CHECK
        if(it.list!=this) fail();
        #endif //INTRUSIVE_LIST_ERROR_CHECK
        IntrusiveListItem *cur=it.cur; //Safe even if it==end() -> cur=nullptr
        IntrusiveListBase::insert(cur,item);
    }
    
    /**
     * Removes the specified item from the list
     * \param it iterator to the item to remove
     * \return an iterator to the next item
     */
    iterator erase(iterator it)
    {
        #ifdef INTRUSIVE_LIST_ERROR_CHECK
        if(it.list!=this) fail();
        #endif //INTRUSIVE_LIST_ERROR_CHECK
        IntrusiveListItem *cur=it.cur;
        return iterator(this,IntrusiveListBase::erase(cur));
    }

    /**
     * Nonportable version of std::list::remove that is O(1) since it relies on
     * the list being intrusive
     * NOTE: can ONLY be called if you are sure the item to remove is either not
     * in any list (in this case, nothing is done) or is in the list it is being
     * removed from. Trying to remove an item that is present in another list
     * produces undefined bahavior.
     * \param item item to remove, must not be nullptr
     * \return true if the item was removed, false if the item was not present
     * in the list
     */
    bool removeFast(T *item)
    {
        if(item->prev==nullptr && IntrusiveListBase::front()!=item) return false;
        IntrusiveListBase::erase(item);
        return true;
    }
    
    /**
     * \return an iterator to the first item
     */
    iterator begin() { return iterator(this,IntrusiveListBase::front()); }
    
    /**
     * \return an iterator to the last item
     */
    iterator end() { return iterator(this,nullptr); }
    
    /**
     * \return a pointer to the first item. List must not be empty
     */
    T* front()
    {
        auto result=IntrusiveListBase::front();
        #ifdef INTRUSIVE_LIST_ERROR_CHECK
        if(result==nullptr) fail();
        #endif //INTRUSIVE_LIST_ERROR_CHECK
        return static_cast<T*>(result);
    }
    
    /**
     * \return a pointer to the last item. List must not be empty
     */
    T* back()
    {
        auto result=IntrusiveListBase::back();
        #ifdef INTRUSIVE_LIST_ERROR_CHECK
        if(result==nullptr) fail();
        #endif //INTRUSIVE_LIST_ERROR_CHECK
        return static_cast<T*>(result);
    }
    
    /**
     * \return true if the list is empty
     */
    bool empty() const { return IntrusiveListBase::empty(); }
};

} //namespace miosix

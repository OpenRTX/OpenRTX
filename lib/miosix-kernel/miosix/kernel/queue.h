/***************************************************************************
 *   Copyright (C) 2014 - 2023 by Terraneo Federico                        *
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

#include "kernel.h"
#include "error.h"

namespace miosix {

/**
 * \addtogroup Sync
 * \{
 */

/**
 * A queue, used to transfer data between TWO threads, or between ONE thread
 * and an IRQ.<br>
 * If you need to tranfer data between more than two threads, you need to
 * use mutexes to ensure that only one thread at a time calls get, and only one
 * thread at a time calls put.<br>
 * Dynamically creating a queue with new or on the stack must be done with care,
 * to avoid deleting a queue with a waiting thread, and to avoid situations
 * where a thread tries to access a deleted queue.
 *
 * \warning the type T most not have a copy constructor or operator= that
 * allocate memory, as allocating memory in FastInterruptDisableLock context
 * and within interrupt handlers is not possible.
 *
 * \tparam T the type of elements in the queue
 * \tparam len the length of the Queue. Value 0 is forbidden
 */
template <typename T, unsigned int len>
class Queue
{
public:
    /**
     * Constructor, create a new empty queue.
     */
    Queue() : waiting(nullptr), numElem(0), putPos(0), getPos(0) {}

    /**
     * \return true if the queue is empty
     */
    bool isEmpty() const { return numElem==0; }

    /**
     * \return true if the queue is full
     */
    bool isFull() const { return numElem==len; }
    
    /**
     * \return the number of elements currently in the queue
     */
    unsigned int size() const { return numElem; }
    
    /**
     * \return the maximum number of elements the queue can hold
     */
    unsigned int capacity() const { return len; }

    /**
     * Put an element to the queue. If the queue is full, then wait until a
     * place becomes available.
     * \param elem element to add to the queue
     */
    void put(const T& elem);

    /**
     * Put an element to the queue. If the queue is full, then use the dLock
     * parameter to enable interrupts and wait until a place becomes available.
     * Being a blocking call, it cannot be called inside an IRQ, it can only be
     * called when interrupts are disabled.
     * \param elem element to add to the queue
     * \param dLock the FastInterruptDisableLock object that was used to disable
     * interrupts in the current context.
     */
    void IRQputBlocking(const T& elem, FastInterruptDisableLock& dLock);

    /**
     * Put an element to the queue, only if the queue is not full.<br>
     * Can ONLY be used inside an IRQ, or when interrupts are disabled.
     * \param elem element to add. The element has been added only if the
     * return value is true
     * \return true if the queue was not full.
     */
    bool IRQput(const T& elem) { return IRQput(elem,nullptr); }

    /**
     * Put an element to the queue, only if the queue is not full.<br>
     * Can ONLY be used inside an IRQ, or when interrupts are disabled.
     * \param elem element to add. The element has been added only if the
     * return value is true
     * \param hppw is not modified if no thread is woken or if the woken thread
     * has a lower or equal priority than the currently running thread, else is
     * set to true
     * \return true if the queue was not full.
     */
    bool IRQput(const T& elem, bool& hppw) { return IRQput(elem,&hppw); }

    /**
     * Get an element from the queue. If the queue is empty, then sleep until
     * an element becomes available.
     * \param elem an element from the queue
     */
    void get(T& elem);

    /**
     * Get an element from the queue. If the queue is empty, then use the dLock
     * parameter to enable interrupts and wait until a place becomes available.
     * Being a blocking call, it cannot be called inside an IRQ, it can only be
     * called when interrupts are disabled.
     * \param elem an element from the queue
     * \param dLock the FastInterruptDisableLock object that was used to disable
     * interrupts in the current context.
     */
    void IRQgetBlocking(T& elem, FastInterruptDisableLock& dLock);

    /**
     * Get an element from the queue, only if the queue is not empty.<br>
     * Can ONLY be used inside an IRQ, or when interrupts are disabled.
     * \param elem an element from the queue. The element is valid only if the
     * return value is true
     * \return true if the queue was not empty
     */
    bool IRQget(T& elem) { return IRQget(elem,nullptr); }

    /**
     * Get an element from the queue, only if the queue is not empty.<br>
     * Can ONLY be used inside an IRQ, or when interrupts are disabled.
     * \param elem an element from the queue. The element is valid only if the
     * return value is true
     * \param hppw is not modified if no thread is woken or if the woken thread
     * has a lower or equal priority than the currently running thread, else is
     * set to true
     * \return true if the queue was not empty
     */
    bool IRQget(T& elem, bool& hppw) { return IRQget(elem,&hppw); }

    /**
     * Clear all items in the queue.<br>
     * Cannot be used inside an IRQ
     */
    void reset()
    {
        FastInterruptDisableLock lock;
        IRQreset();
    }
    
    /**
     * Same as reset(), but to be used only inside IRQs or when interrupts are
     * disabled.
     */
    void IRQreset();

    //Unwanted methods
    Queue(const Queue& s) = delete;
    Queue& operator= (const Queue& s) = delete;

private:
    /**
     * Put an element to the queue, only if the queue is not full.
     * \param elem element to add. The element has been added only if the
     * return value is true
     * \param hppw is not modified if nullptr or no thread is woken or if the
     * woken thread has a lower or equal priority than the currently running
     * thread, else is set to true
     * \return true if the queue was not full
     */
    bool IRQput(const T& elem, bool *hppw);

    /**
     * Get an element from the queue, only if the queue is not empty.
     * \param elem an element from the queue. The element is valid only if the
     * return value is true
     * \param hppw is not modified if nullptr or no thread is woken or if the
     * woken thread has a lower or equal priority than the currently running
     * thread, else is set to true
     * \return true if the queue was not empty
     */
    bool IRQget(T& elem, bool *hppw);

    /**
     * Wake an eventual waiting thread.
     * Must be called when interrupts are disabled
     */
    void IRQwakeWaitingThread()
    {
        if(!waiting) return;
        waiting->IRQwakeup();//Wakeup eventual waiting thread
        waiting=nullptr;
    }

    //Queue data
    T buffer[len];///< queued elements are put here. Used as a ring buffer
    Thread *waiting;///< If not null holds the thread waiting
    volatile unsigned int numElem;///< nuber of elements in the queue
    volatile unsigned int putPos; ///< index of buffer where to get next element
    volatile unsigned int getPos; ///< index of buffer where to put next element
};

template <typename T, unsigned int len>
void Queue<T,len>::put(const T& elem)
{
    FastInterruptDisableLock dLock;
    while(IRQput(elem)==false)
    {
        waiting=Thread::IRQgetCurrentThread();
        Thread::IRQenableIrqAndWait(dLock);
    }
}

template <typename T, unsigned int len>
void Queue<T,len>::IRQputBlocking(const T& elem, FastInterruptDisableLock& dLock)
{
    while(IRQput(elem)==false)
    {
        waiting=Thread::IRQgetCurrentThread();
        Thread::IRQenableIrqAndWait(dLock);
    }
}

template <typename T, unsigned int len>
void Queue<T,len>::get(T& elem)
{
    FastInterruptDisableLock dLock;
    while(IRQget(elem)==false)
    {
        waiting=Thread::IRQgetCurrentThread();
        Thread::IRQenableIrqAndWait(dLock);
    }
}

template <typename T, unsigned int len>
void Queue<T,len>::IRQgetBlocking(T& elem, FastInterruptDisableLock& dLock)
{
    while(IRQget(elem)==false)
    {
        waiting=Thread::IRQgetCurrentThread();
        Thread::IRQenableIrqAndWait(dLock);
    }
}

template <typename T, unsigned int len>
bool Queue<T,len>::IRQput(const T& elem, bool *hppw)
{
    if(hppw && waiting && (Thread::IRQgetCurrentThread()->IRQgetPriority() <
            waiting->IRQgetPriority())) *hppw=true;
    IRQwakeWaitingThread();
    if(isFull()) return false;
    numElem++;
    buffer[putPos]=elem;
    if(++putPos==len) putPos=0;
    return true;
}

template <typename T, unsigned int len>
bool Queue<T,len>::IRQget(T& elem, bool *hppw)
{
    if(hppw && waiting && (Thread::IRQgetCurrentThread()->IRQgetPriority()) <
            waiting->IRQgetPriority()) *hppw=true;
    IRQwakeWaitingThread();
    if(isEmpty()) return false;
    numElem--;
    elem=std::move(buffer[getPos]);
    if(++getPos==len) getPos=0;
    return true;
}

template <typename T, unsigned int len>
void Queue<T,len>::IRQreset()
{
    IRQwakeWaitingThread();
    //Relying on constant folding to omit this code for trivial types
    if(std::is_trivially_destructible<T>::value==false)
    {
        while(!isEmpty())
        {
            numElem--;
            buffer[getPos].~T();
            if(++getPos==len) getPos=0;
        }
    }
    putPos=getPos=numElem=0;
}

//This partial specialization is meant to to produce compiler errors in case an
//attempt is made to instantiate a Queue with zero size, as it is forbidden
template<typename T> class Queue<T,0> {};

/**
 * An unsynchronized circular buffer data structure with the storage dynamically
 * allocated on the heap.
 * Note that unlike Queue, this class is only a data structure and not a
 * synchronization primitive. The synchronization between the thread and
 * the IRQ (or the other thread) must be done by the caller.
 */
template<typename T>
class DynUnsyncQueue
{
public:
    /**
     * Constructor
     * \param elem number of elements of the circular buffer
     */
    DynUnsyncQueue(unsigned int elem) : data(new T[elem]),
            putPos(0), getPos(0), queueSize(0), queueCapacity(elem) {}
    
    /**
     * \return true if the queue is empty
     */
    bool isEmpty() const { return queueSize==0; }
    
    /**
     * \return true if the queue is full
     */
    bool isFull() const { return queueSize==queueCapacity; }
    
    /**
     * \return the number of elements currently in the queue
     */
    unsigned int size() const { return queueSize; }
    
    /**
     * \return the maximum number of elements the queue can hold
     */
    unsigned int capacity() const { return queueCapacity; }
    
    /**
     * Try to put an element in the circular buffer
     * \param elem element to put
     * \return true if the queue was not full 
     */
    bool tryPut(const T& elem);
    
    /**
     * Try to get an element from the circular buffer
     * \param elem element to get will be stored here
     * \return true if the queue was not empty
     */
    bool tryGet(T& elem);
    
    /**
     * Erase all elements in the queue 
     */
    void reset() { putPos=getPos=queueSize=0; }
    
    /**
     * Destructor
     */
    ~DynUnsyncQueue() { delete[] data; }

    //Unwanted methods
    DynUnsyncQueue(const DynUnsyncQueue&) = delete;
    DynUnsyncQueue& operator=(const DynUnsyncQueue&) = delete;

private:
    T *data;
    unsigned int putPos,getPos;
    volatile unsigned int queueSize;
    const unsigned int queueCapacity;
};

template<typename T>
bool DynUnsyncQueue<T>::tryPut(const T& elem)
{
    if(isFull()) return false;
    queueSize++;
    data[putPos++]=elem;
    if(putPos>=queueCapacity) putPos=0;
    return true;
}

template<typename T>
bool DynUnsyncQueue<T>::tryGet(T& elem)
{
    if(isEmpty()) return false;
    queueSize--;
    elem=data[getPos++];
    if(getPos>=queueCapacity) getPos=0;
    return true;
}

/**
 * A class to handle double buffering, but also triple buffering and in general
 * N-buffering. Works between two threads but is especially suited to
 * synchronize between a thread and an interrupt routine.<br>
 * Note that unlike Queue, this class is only a data structure and not a
 * synchronization primitive. The synchronization between the thread and
 * the IRQ (or the other thread) must be done by the caller. <br>
 * The internal implementation treats the buffers as a circular queue of N
 * elements, hence the name.
 * \tparam T type of elements of the buffer, usually char or unsigned char
 * \tparam size maximum size of a buffer
 * \tparam numbuf number of buffers, the default is two resulting in a
 * double buffering scheme. Values 0 and 1 are forbidden
 */
template<typename T, unsigned int size, unsigned char numbuf=2>
class BufferQueue
{
public:
    /**
     * Constructor, all buffers are empty
     */
    BufferQueue() : put(0), get(0), cnt(0) {}

    /**
     * \return true if no buffer is available for reading
     */
    bool isEmpty() const { return cnt==0; }

    /**
     * \return true if no buffer is available for writing
     */
    bool isFull() const { return cnt==numbuf; }
    
    /**
     * \return the maximum size of a buffer
     */
    unsigned int bufferMaxSize() const { return size; }

    /**
     * \return the maximum number of buffers 
     */
    unsigned int numberOfBuffers() const { return numbuf; }

    /**
     * This member function allows to retrieve a buffer ready to be written,
     * if available.
     * \param buffer the available buffer will be assigned here if available
     * \return true if a writable buffer has been found, false otherwise.
     * In this case the buffer parameter is not modified
     */
    bool tryGetWritableBuffer(T *&buffer)
    {
        if(isFull()) return false;
        buffer=buf[put];
        return true;
    }

    /**
     * After having called tryGetWritableBuffer() to retrieve a buffer and
     * having filled it, this member function allows to mark the buffer as
     * available on the reader side.
     * \param actualSize actual size of buffer. It usually equals bufferMaxSize
     * but can be a lower value in case there is less available data
     */
    void bufferFilled(unsigned int actualSize)
    {
        if(isFull()) errorHandler(UNEXPECTED);
        cnt++;
        bufSize[put++]=actualSize;
        if(put>=numbuf) put=0;
    }

    /**
     * \return the number of buffers available for writing (0 to numbuf)
     */
    unsigned char availableForWriting() const { return numbuf-cnt; }

    /**
     * This member function allows to retrieve a buffer ready to be read,
     * if available.
     * \param buffer the available buffer will be assigned here if available
     * \param actualSize the actual size of the buffer, as reported by the
     * writer side
     * \return true if a readable buffer has been found, false otherwise.
     * In this case the buffer and actualSize parameters are not modified
     */
    bool tryGetReadableBuffer(const T *&buffer, unsigned int& actualSize)
    {
        if(isEmpty()) return false;
        buffer=buf[get];
        actualSize=bufSize[get];
        return true;
    }

    /**
     * After having called tryGetReadableBuffer() to retrieve a buffer and
     * having read it, this member function allows to mark the buffer as
     * available on the writer side.
     */
    void bufferEmptied()
    {
        if(isEmpty()) errorHandler(UNEXPECTED);
        cnt--;
        get++;
        if(get>=numbuf) get=0;
    }

    /**
     * \return The number of buffers available for reading (0, to numbuf)
     */
    unsigned char availableForReading() const { return cnt; }

    /**
     * Reset the buffers. As a consequence, the queue becomes empty.
     */
    void reset()
    {
        put=get=cnt=0;
    }

    //Unwanted methods
    BufferQueue(const BufferQueue&) = delete;
    BufferQueue& operator=(const BufferQueue&) = delete;

private:
    T buf[numbuf][size]; // The buffers
    unsigned int bufSize[numbuf]; //To handle partially empty buffers
    unsigned char put; //Put pointer
    unsigned char get; //Get pointer
    volatile unsigned char cnt; //Number of filled buffers, either (0 to numbuf)
};

//These two partial specialization are meant to produce compiler errors in case
//an attempt is made to allocate a BufferQueue with zero or one buffer, as it
//is forbidden
template<typename T, unsigned int size> class BufferQueue<T,size,0> {};
template<typename T, unsigned int size> class BufferQueue<T,size,1> {};

/**
 * \}
 */

} //namespace miosix

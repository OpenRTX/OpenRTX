/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RINGBUF_H
#define RINGBUF_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <pthread.h>
#include <cstdint>

/**
 * Class implementing a statically allocated circular buffer with blocking and
 * non-blocking push and pop functions.
 */
template < typename T, size_t N >
class RingBuffer
{
public:

    /**
     * Constructor.
     */
    RingBuffer() : readPos(0), writePos(0), numElements(0)
    {
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&not_empty, NULL);
        pthread_cond_init(&not_full, NULL);
    }

    /**
     * Destructor.
     */
    ~RingBuffer()
    {
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&not_empty);
        pthread_cond_destroy(&not_full);
    }

    /**
     * Push an element to the buffer.
     *
     * @param elem: element to be pushed.
     * @param blocking: if set to true, when the buffer is full this function
     * blocks the execution flow until at least one empty slot is available.
     * @return true if the element has been successfully pushed to the queue,
     * false if the queue is empty.
     */
    bool push(const T& elem, bool blocking)
    {
        pthread_mutex_lock(&mutex);

        if((numElements >= N) && (blocking == false))
        {
            // No space available and non-blocking call: unlock mutex and return
            pthread_mutex_unlock(&mutex);
            return false;
        }

        // The call is blocking: wait until there is some free space
        while(numElements >= N)
        {
            pthread_cond_wait(&not_full, &mutex);
        }

        // There is free space, push data into the queue
        data[writePos] = elem;
        writePos = (writePos + 1) % N;

        // Signal that the queue is not empty
        if(numElements == 0) pthread_cond_signal(&not_empty);
        numElements += 1;

        pthread_mutex_unlock(&mutex);
        return true;
    }

    /**
     * Pop an element from the buffer.
     *
     * @param elem: place where to store the popped element.
     * @param blocking: if set to true, when the buffer is empty this function
     * blocks the execution flow until at least one element is available.
     * @return true if the element has been successfully popped from the queue,
     * false if the queue is empty.
     */
    bool pop(T& elem, bool blocking)
    {
        pthread_mutex_lock(&mutex);

        if((numElements == 0) && (blocking == false))
        {
            // No elements present and non-blocking call: unlock mutex and return
            pthread_mutex_unlock(&mutex);
            return false;
        }

        // The call is blocking: wait until there is something into the queue
        while(numElements == 0)
        {
            pthread_cond_wait(&not_empty, &mutex);
        }

        // At least one element present pop one.
        elem    = data[readPos];
        readPos = (readPos + 1) % N;

        // Signal that the queue is no more full
        if(numElements >= N) pthread_cond_signal(&not_full);
        numElements -= 1;

        pthread_mutex_unlock(&mutex);

        return true;
    }

    /**
     * Check if the buffer is empty.
     *
     * @return true if the buffer is empty.
     */
    bool empty()
    {
        return numElements == 0;
    }

    /**
     * Check if the buffer is full.
     *
     * @return true if the buffer is full.
     */
    bool full()
    {
        return numElements >= N;
    }

    /**
     * Discard one element from the buffer's tail, creating a new empty slot.
     * In case the buffer is full calling this function unlocks the eventual
     * threads waiting to push data.
     */
    void eraseElement()
    {
        // Nothing to erase
        if(numElements == 0) return;

        pthread_mutex_lock(&mutex);

        // Chomp away one element just by advancing the read pointer.
        readPos = (readPos + 1) % N;

        if(numElements >= N) pthread_cond_signal(&not_full);
        numElements -= 1;

        pthread_mutex_unlock(&mutex);
    }

    /**
     * Reset the buffer to its empty state discarding all the elements stored.
     *
     * Note: reset is "lazy", as it just sets read pointer, write pointer and
     * number of elements to zero. No actual erasure of the stored elements is
     * done until they are effectively overwritten by new push()es.
     */
    void reset()
    {
        readPos     = 0;
        writePos    = 0;
        numElements = 0;
    }

private:

    size_t readPos;      ///< Read pointer.
    size_t writePos;     ///< Write pointer.
    size_t numElements;  ///< Number of elements currently present.
    T      data[N];      ///< Data storage.

    pthread_mutex_t mutex;      ///< Mutex for concurrent access.
    pthread_cond_t  not_empty;  ///< Queue not empty condition.
    pthread_cond_t  not_full;   ///< Queue not full condition.
};

#endif  // RINGBUF_H

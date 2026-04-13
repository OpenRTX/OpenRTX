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
#include <errno.h>

/**
 * Class implementing a statically allocated circular buffer with non-blocking
 * push and pop functions.
 */
template <typename T, size_t N>
class NbRingBuffer
{
public:
    /**
     * Constructor.
     */
    NbRingBuffer() : readPos(0), writePos(0), numElements(0)
    {
        pthread_mutex_init(&mutex, NULL);
    }

    /**
     * Destructor.
     */
    ~NbRingBuffer()
    {
        pthread_mutex_destroy(&mutex);
    }

    /**
     * Push an element to the buffer.
     *
     * @param elem: element to be pushed.
     * @return zero on success, -EAGAIN if the queue is full and -EBUSY if the
     * queue is kept by another thread.
     */
    int tryPush(const T &elem)
    {
        int ret = pthread_mutex_trylock(&mutex);
        if (ret)
            return ret;

        if (full()) {
            pthread_mutex_unlock(&mutex);
            return -EAGAIN;
        }

        doPush(elem);
        pthread_mutex_unlock(&mutex);

        return 0;
    }

    /**
     * Pop an element from the buffer.
     *
     * @param elem: place where to store the popped element.
     * @return zero on success, -EAGAIN if the queue is empty and -EBUSY if the
     * queue is kept by another thread.
     */
    int tryPop(T &elem)
    {
        int ret = pthread_mutex_trylock(&mutex);
        if (ret)
            return ret;

        if (empty()) {
            pthread_mutex_unlock(&mutex);
            return -EAGAIN;
        }

        doPop(elem);
        pthread_mutex_unlock(&mutex);

        return 0;
    }

    /**
     * Check if the buffer is empty.
     * This function is not thread safe.
     *
     * @return true if the buffer is empty.
     */
    bool empty() const
    {
        return numElements == 0;
    }

    /**
     * Check if the buffer is full.
     * This function is not thread safe.
     *
     * @return true if the buffer is full.
     */
    bool full() const
    {
        return numElements >= N;
    }

    /**
     * Get the number of elements currently stored in the buffer.
     * This function is not thread safe.
     *
     * @return the number of elements currently present in the buffer.
     */
    size_t size() const
    {
        return numElements;
    }

    /**
     * Reset the buffer to its empty state discarding all the elements stored,
     * blocking function.

     * Reset is "lazy", as it just sets read pointer, write pointer and
     * number of elements to zero. No actual erasure of the stored elements is
     * done until they are effectively overwritten by new push()es.
     */
    void reset()
    {
        pthread_mutex_lock(&mutex);
        readPos = 0;
        writePos = 0;
        numElements = 0;
        pthread_mutex_unlock(&mutex);
    }

protected:
    /**
     * Enqueueing helper function.
     *
     * @param elem: element to enqueue.
     */
    inline void doPush(const T &elem)
    {
        data[writePos++] = elem;
        if (writePos == N)
            writePos = 0;
        numElements += 1;
    }

    /**
     * Dequeueing helper function.
     *
     * @param elem: element to dequeue.
     */
    inline void doPop(T &elem)
    {
        elem = data[readPos++];
        if (readPos == N)
            readPos = 0;
        numElements -= 1;
    }

    size_t readPos;        ///< Read pointer.
    size_t writePos;       ///< Write pointer.
    size_t numElements;    ///< Number of elements currently present.
    T data[N];             ///< Data storage.

    pthread_mutex_t mutex; ///< Mutex for concurrent access.
};

/**
 * Class implementing a statically allocated circular buffer with blocking and
 * non-blocking push and pop functions.
 */
template <typename T, size_t N>
class RingBuffer : public NbRingBuffer<T, N>
{
public:
    /**
     * Constructor.
     */
    RingBuffer()
    {
        pthread_cond_init(&not_empty, NULL);
        pthread_cond_init(&not_full, NULL);
    }

    /**
     * Destructor.
     */
    ~RingBuffer()
    {
        pthread_cond_destroy(&not_empty);
        pthread_cond_destroy(&not_full);
    }

    /**
     * Push an element to the buffer.
     * If the buffer is full the execution flow is blocked until at least one
     * slot is available.
     *
     * @param elem: element to be pushed.
     */
    void push(const T &elem)
    {
        pthread_mutex_lock(&this->mutex);

        while (this->numElements >= N)
            pthread_cond_wait(&not_full, &this->mutex);

        this->doPush(elem);
        if (this->numElements == 1)
            pthread_cond_signal(&not_empty);

        pthread_mutex_unlock(&this->mutex);
    }

    /**
     * Pop an element from the buffer.
     * If the buffer is empty the execution flow is blocked until at least one
     * element is available.
     *
     * @param elem: place where to store the popped element.
     */
    void pop(T &elem)
    {
        pthread_mutex_lock(&this->mutex);

        while (this->numElements == 0)
            pthread_cond_wait(&not_empty, &this->mutex);

        this->doPop(elem);
        if (this->numElements == (N - 1))
            pthread_cond_signal(&not_full);

        pthread_mutex_unlock(&this->mutex);
    }

    /**
     * Discard one element from the buffer's tail, creating a new empty slot.
     * In case the buffer is full calling this function unlocks the eventual
     * threads waiting to push data.
     */
    void eraseElement()
    {
        pthread_mutex_lock(&this->mutex);

        if (this->numElements == 0) {
            pthread_mutex_unlock(&this->mutex);
            return;
        }

        this->readPos = (this->readPos + 1) % N;
        this->numElements -= 1;

        if (this->numElements == (N - 1))
            pthread_cond_signal(&not_full);

        pthread_mutex_unlock(&this->mutex);
    }

private:
    pthread_cond_t not_empty; ///< Queue not empty condition.
    pthread_cond_t not_full;  ///< Queue not full condition.
};

#endif // RINGBUF_H

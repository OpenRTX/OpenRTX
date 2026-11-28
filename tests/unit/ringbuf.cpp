/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <cerrno>
#include <thread>

#include "core/ringbuf.hpp"

static std::atomic<bool> inCopy(false);
static std::atomic<bool> release(false);

/**
 * Element whose assignment blocks until released, so that a second thread
 * can hold the queue lock for as long as the test needs.
 */
struct SlowElem {
    SlowElem &operator=(const SlowElem &other)
    {
        inCopy = true;
        while (!release)
            std::this_thread::yield();

        value = other.value;
        return *this;
    }

    int value = 0;
};

TEST_CASE("NbRingBuffer: full and empty queues report -EAGAIN",
          "[core][ringbuf]")
{
    NbRingBuffer<int, 2> queue;
    int elem = 0;

    REQUIRE(queue.tryPop(elem) == -EAGAIN);
    REQUIRE(queue.tryPush(1) == 0);
    REQUIRE(queue.tryPush(2) == 0);
    REQUIRE(queue.tryPush(3) == -EAGAIN);

    REQUIRE(queue.tryPop(elem) == 0);
    REQUIRE(elem == 1);
    REQUIRE(queue.tryPop(elem) == 0);
    REQUIRE(elem == 2);
    REQUIRE(queue.tryPop(elem) == -EAGAIN);
}

TEST_CASE("NbRingBuffer: a queue locked by another thread reports -EBUSY",
          "[core][ringbuf]")
{
    NbRingBuffer<SlowElem, 2> queue;
    SlowElem elem;
    elem.value = 42;

    inCopy = false;
    release = false;
    std::thread pusher([&]() { queue.tryPush(elem); });

    while (!inCopy)
        std::this_thread::yield();

    SlowElem other;
    REQUIRE(queue.tryPush(other) == -EBUSY);
    REQUIRE(queue.tryPop(other) == -EBUSY);

    release = true;
    pusher.join();

    REQUIRE(queue.tryPop(other) == 0);
    REQUIRE(other.value == 42);
}

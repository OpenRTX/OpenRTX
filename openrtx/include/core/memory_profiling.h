/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MEMORY_PROFILING_H
#define MEMORY_PROFILING_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \return stack size of the caller thread.
 */
unsigned int getStackSize();

/**
 * \return absolute free stack of current thread.
 * Absolute free stack is the minimum free stack since the thread was
 * created.
 */
unsigned int getAbsoluteFreeStack();

/**
 * \return current free stack of current thread.
 * Current free stack is the free stack at the moment when the this
 * function is called.
 */
unsigned int getCurrentFreeStack();

/**
 * \return heap size which is defined in the linker script The heap is
 * shared among all threads, therefore this function returns the same value
 * regardless which thread is called in.
 */
unsigned int getHeapSize();

/**
 * \return absolute (not current) free heap.
 * Absolute free heap is the minimum free heap since the program started.
 * The heap is shared among all threads, therefore this function returns
 * the same value regardless which thread is called in.
 */
unsigned int getAbsoluteFreeHeap();

/**
 * \return current free heap.
 * Current free heap is the free heap at the moment when the this
 * function is called.
 * The heap is shared among all threads, therefore this function returns
 * the same value regardless which thread is called in.
 */
unsigned int getCurrentFreeHeap();

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_PROFILING_H */

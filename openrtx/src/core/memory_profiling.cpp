/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/memory_profiling.h"

#ifdef _MIOSIX

#include <miosix.h>

/*
 * Provide a C-callable wrapper for the corresponding miosix functions.
 */

unsigned int getStackSize()
{
    return miosix::MemoryProfiling::getStackSize();
}

unsigned int getAbsoluteFreeStack()
{
    return miosix::MemoryProfiling::getAbsoluteFreeStack();
}

unsigned int getCurrentFreeStack()
{
    return miosix::MemoryProfiling::getCurrentFreeStack();
}

unsigned int getHeapSize()
{
    return miosix::MemoryProfiling::getHeapSize();
}

unsigned int getAbsoluteFreeHeap()
{
    return miosix::MemoryProfiling::getAbsoluteFreeHeap();
}

unsigned int getCurrentFreeHeap()
{
    return miosix::MemoryProfiling::getCurrentFreeHeap();
}

#else

/*
 * No memory profiling is possible on x86/64 machines, thus all the functions
 * return 0.
 */

unsigned int getStackSize()
{
    return 0;
}

unsigned int getAbsoluteFreeStack()
{
    return 0;
}

unsigned int getCurrentFreeStack()
{
    return 0;
}

unsigned int getHeapSize()
{
    return 0;
}

unsigned int getAbsoluteFreeHeap()
{
    return 0;
}

unsigned int getCurrentFreeHeap()
{
    return 0;
}

#endif

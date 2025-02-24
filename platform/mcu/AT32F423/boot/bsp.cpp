/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/***********************************************************************
* bsp.cpp Part of the Miosix Embedded OS.
* Board support package, this file initializes hardware.
************************************************************************/

#include <interfaces/bsp.h>
#include <kernel/kernel.h>
#include <kernel/sync.h>

namespace miosix
{

//
// Initialization
//

void IRQbspInit()
{
    CRM->ahben1 |= (1 << 0)     // Enable GPIOA
                |  (1 << 1)     // Enable GPIOB
                |  (1 << 2)     // Enable GPIOC
                |  (1 << 5);    // Enable GPIOF

    // Configure SysTick
    SysTick->LOAD = SystemCoreClock / miosix::TICK_FREQ;
}

void bspInit2()
{

}

//
// Shutdown and reboot
//

void shutdown()
{
    reboot();
}

void reboot()
{
    disableInterrupts();
    miosix_private::IRQsystemReboot();
}

} //namespace miosix

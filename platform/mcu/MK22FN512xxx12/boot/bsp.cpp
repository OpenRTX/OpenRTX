/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/***********************************************************************
* bsp.cpp Part of the Miosix Embedded OS.
* Board support package, this file initializes hardware.
************************************************************************/

#include "interfaces/bsp.h"
#include <kernel/kernel.h>
#include <kernel/sync.h>
#include "hwconfig.h"

#include "peripherals/gpio.h"

namespace miosix
{

//
// Initialization
//

void IRQbspInit()
{
    SIM->SCGC5 |= 0x3E00;   // Enable GPIO clock

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

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/***********************************************************************
* bsp.cpp Part of the Miosix Embedded OS.
* Board support package, this file initializes hardware.
************************************************************************/

#include <interfaces/platform.h>
#include <peripherals/gpio.h>
#include <drivers/USART6.h>
#include <interfaces/bsp.h>
#include <kernel/kernel.h>
#include <kernel/sync.h>
#include <hwconfig.h>

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




    // Bring up USART1
    gpio_setMode(USART6_TX, ALTERNATE);
    usart6_init(115200);
    usart6_IRQwrite("Starting system...\r\n");
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

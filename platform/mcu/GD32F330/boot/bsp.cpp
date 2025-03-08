/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO,                            *
 *                         Federico Terraneo                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

/***********************************************************************
* bsp.cpp Part of the Miosix Embedded OS.
* Board support package, this file initializes hardware.
************************************************************************/

#include <gd32f3x0.h>
#include <gd32f3x0_rcu.h>
#include <drivers/USART0.h>
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
    // RCU->ahben |= (1 << 17)     // Enable GPIOA
    //            |  (1 << 18)     // Enable GPIOB
    //            |  (1 << 19)     // Enable GPIOC
    //            |  (1 << 22);    // Enable GPIOF
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOF);

    // Configure SysTick
    SysTick_Config(SystemCoreClock / miosix::TICK_FREQ);
    
    // Bring up USART1
    gpio_af_set((uint32_t)GPIOA, GPIO_AF_1, GPIO_PIN_9 | GPIO_PIN_10);
    usart0_init(115200);
    //usart0_IRQwrite("Starting system...\r\n");
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

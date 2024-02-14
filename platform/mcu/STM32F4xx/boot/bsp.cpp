/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO,                     *
 *                                Federico Terraneo                        *
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

#include <interfaces/bsp.h>
#include <kernel/kernel.h>
#include <kernel/sync.h>
#include <hwconfig.h>
#include "../drivers/usb_vcom.h"
#include "../drivers/USART3.h"

namespace miosix
{

//
// Initialization
//

void IRQbspInit()
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN
                 |  RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN
                 |  RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOHEN;
    RCC_SYNC();

    GPIOA->OSPEEDR=0xaaaaaaaa; //Default to 50MHz speed for all GPIOS
    GPIOB->OSPEEDR=0xaaaaaaaa;
    GPIOC->OSPEEDR=0xaaaaaaaa;
    GPIOD->OSPEEDR=0xaaaaaaaa;
    GPIOE->OSPEEDR=0xaaaaaaaa;
    GPIOH->OSPEEDR=0xaaaaaaaa;

    /*
     * Enable SWD interface on PA13 and PA14 (Tytera's bootloader disables this
     * functionality).
     * NOTE: these pins are used also for other functions (MIC power and wide/
     * narrow FM reception), thus they cannot be always used for debugging!
     */
    #ifdef MDx_ENABLE_SWD
    GPIOA->MODER  &= ~0x3C000000;   // Clear current setting
    GPIOA->MODER  |= 0x28000000;    // Put back to alternate function
    GPIOA->AFR[1] &= ~0x0FF00000;   // SWD is AF0
    #endif

    #ifdef MD3x0_ENABLE_DBG
    usart3_init(115200);
    usart3_IRQwrite("starting...\r\n");
    #endif

}

void bspInit2()
{
    vcom_init();
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

/***************************************************************************
 *   Copyright (C) 2022 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#include <interfaces/platform.h>
#include <peripherals/gpio.h>
#include <hwconfig.h>
#include "usb.h"

/*
 * USB interrupt handler.
 *

void OTG_FS_IRQHandler(void)
{
    tud_int_handler(0);
}
*/

void usb_init()
{
    gpio_setMode(GPIOA, 11, ALTERNATE | ALTERNATE_FUNC(10));
    gpio_setMode(GPIOA, 12, ALTERNATE | ALTERNATE_FUNC(10));

    gpio_setOutputSpeed(GPIOA, 11, HIGH);      // 100MHz output speed
    gpio_setOutputSpeed(GPIOA, 12, HIGH);      // 100MHz output speed

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
    __DSB();

    // Disable VBUS detection and activate the USB transceiver
    USB_OTG_FS->GCCFG   |= USB_OTG_GCCFG_NOVBUSSENS
                        |  USB_OTG_GCCFG_PWRDWN;

    // Force USB device mode
    USB_OTG_FS->GUSBCFG |= USB_OTG_GUSBCFG_FDMOD;

    NVIC_SetPriority(OTG_FS_IRQn, 14);

    tusb_init();
}

void usb_terminate()
{
    RCC->AHB2ENR &= ~RCC_AHB2ENR_OTGFSEN;
    __DSB();
}

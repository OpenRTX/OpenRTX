/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "peripherals/gpio.h"
#include "hwconfig.h"
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

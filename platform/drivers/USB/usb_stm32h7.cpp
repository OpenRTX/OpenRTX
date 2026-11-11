/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include "interfaces/delays.h"
#include "peripherals/gpio.h"
#include "stm32h7xx.h"
#include "pinmap.h"
#include "tusb.h"
#include "usb.h"

/*
 * STM32H743 OTG FS controller (USB2 in RM0433, on PA11 and PA12) for
 * TinyUSB's DWC2 driver. Register references are to RM0433, "STM32H742,
 * STM32H743/753 and STM32H750 Value line advanced Arm-based 32-bit MCUs":
 * https://www.st.com/resource/en/reference_manual/rm0433-stm32h742-stm32h743753-and-stm32h750-value-line-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
 */

/**
 * Alternate function of PA11 and PA12 for OTG_FS, from the alternate
 * function table of DS12110, the STM32H743 datasheet.
 */
#define OTG_FS_AF 10

/**
 * Interrupt priority: Miosix runs without interrupt nesting, and 14 is near
 * the lowest, as for the other peripherals.
 */
#define OTG_FS_IRQ_PRIORITY 14

/** Wait for a ready flag at most this many 10 us steps: 10 ms. */
#define READY_POLL_STEPS 1000

/**
 * Wait for a hardware ready flag, giving up after READY_POLL_STEPS.
 *
 * @param reg: register holding the flag.
 * @param mask: the flag.
 * @return true when the flag was set in time.
 */
static bool waitReady(volatile const uint32_t *reg, uint32_t mask)
{
    for (uint32_t i = 0; i < READY_POLL_STEPS; i++) {
        if ((*reg & mask) != 0)
            return true;

        delayUs(10);
    }

    return false;
}

int usb_init()
{
    // VDD33USB detector on (PWR_CR3 USB33DEN): the FS PHY needs the 3.3 V
    // supply. The internal USB regulator (USBREGEN) stays off: it draws from
    // the battery and keeps its voltage above the power-off threshold.
    PWR->CR3 |= PWR_CR3_USB33DEN;
    if (!waitReady(&PWR->CR3, PWR_CR3_USB33RDY))
        return -ETIMEDOUT;

    gpio_setMode(USB_DN, ALTERNATE | ALTERNATE_FUNC(OTG_FS_AF));
    gpio_setMode(USB_DP, ALTERNATE | ALTERNATE_FUNC(OTG_FS_AF));
    gpio_setOutputSpeed(USB_DN, HIGH);
    gpio_setOutputSpeed(USB_DP, HIGH);

    // USB kernel clock from the 48 MHz RC oscillator (RCC_CR HSI48ON;
    // RCC_D2CCIP2R USBSEL = 11): PLL1Q runs at 100 MHz and PLL3 is unused.
    RCC->CR |= RCC_CR_HSI48ON;
    if (!waitReady(&RCC->CR, RCC_CR_HSI48RDY))
        return -ETIMEDOUT;

    RCC->D2CCIP2R = (RCC->D2CCIP2R & ~RCC_D2CCIP2R_USBSEL)
                  | RCC_D2CCIP2R_USBSEL_0 | RCC_D2CCIP2R_USBSEL_1;

    // Trim HSI48 to the host's 1 kHz start-of-frame with the clock recovery
    // system, since untrimmed it is close to the USB tolerance: CRS_CFGR
    // SYNCSRC = 11 selects the OTG FS SOF, and the reset RELOAD and FELIM
    // already suit a 48 MHz clock and a 1 kHz sync.
    RCC->APB1HENR |= RCC_APB1HENR_CRSEN;
    __DSB();
    CRS->CFGR = (CRS->CFGR & ~CRS_CFGR_SYNCSRC) | CRS_CFGR_SYNCSRC_0
              | CRS_CFGR_SYNCSRC_1;
    CRS->CR |= CRS_CR_AUTOTRIMEN | CRS_CR_CEN;

    // Clock the controller and reset it, clearing what a soft reboot left.
    // Its unused ULPI clock stops in Sleep (RCC_AHB1LPENR), as ST's HAL
    // does, so the core can idle in WFI.
    RCC->AHB1ENR |= RCC_AHB1ENR_USB2OTGFSEN;
    RCC->AHB1LPENR &= ~RCC_AHB1LPENR_USB2OTGFSULPILPEN;
    __DSB();
    RCC->AHB1RSTR |= RCC_AHB1RSTR_USB2OTGFSRST;
    __DSB();
    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_USB2OTGFSRST;
    __DSB();

    // No VBUS sensing (OTG_GCCFG VBDEN), so the PHY treats VBUS as present:
    // the CS7000-PLUS uses PA9, the sense pin, for its Bluetooth UART.
    // TinyUSB enables the interrupt itself once its state is set up.
    USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_VBDEN;
    NVIC_SetPriority(OTG_FS_IRQn, OTG_FS_IRQ_PRIORITY);

    return tusb_init() ? 0 : -EIO;
}

void OTG_FS_IRQHandler()
{
    tud_int_handler(0);
}

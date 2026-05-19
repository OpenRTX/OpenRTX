/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RCC_H
#define RCC_H

#include "hwconfig.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enumeration type for STM32 internal busses.
 */
enum PeriphBus
{
    PERIPH_BUS_AHB  = 0,
    PERIPH_BUS_APB1 = 1,
    PERIPH_BUS_APB2 = 2,

    PERIPH_BUS_NUM
};

/**
 * Get the clock frequency of a given peripheral bus.
 *
 * @param bus: bus identifier.
 * @return bus clock frequency in Hz or zero in case of errors.
 */
uint32_t rcc_getBusClock(const uint8_t bus);

#ifdef __cplusplus
}

/**
 * Get the clock frequency of the bus a peripheral is attached to.
 *
 * @param periph: peripheral base address
 * @return bus clock frequency in Hz or zero in case of errors.
 */
static constexpr uint32_t rcc_getPeriphClock(const void *periph)
{
    uint32_t addr = reinterpret_cast<uint32_t>(periph);

    switch(addr & 0xFFFF0000) {
        case APB1PERIPH_BASE:
            return rcc_getBusClock(PERIPH_BUS_APB1);
            break;

        case APB2PERIPH_BASE:
            return rcc_getBusClock(PERIPH_BUS_APB2);
            break;

        case AHB1PERIPH_BASE:
        case AHB2PERIPH_BASE:
            return rcc_getBusClock(PERIPH_BUS_AHB);
            break;
    }

    return 0;
}
#endif

#endif /* RCC_H */

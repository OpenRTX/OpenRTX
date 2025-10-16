/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PMU_H
#define PMU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initalise the AXP2101 power management unit and power on the main power
 * supply rails.
 */
void pmu_init();

/**
 * Shutdown the AXP2101 and remove power from all the power supply rails.
 * Calling this function shuts down the device.
 */
void pmu_terminate();

/**
 * Get current battery voltage.
 *
 * @return battery voltage in mV.
 */
uint16_t pmu_getVbat();

/**
 * Control the baseband power supply rail.
 *
 * @param enable: set to true to enable the baseband power supply.
 */
void pmu_setBasebandPower(bool enable);

/**
 * Control the GPS power supply rail.
 *
 * @param enable: set to true to enable the GPS power supply.
 */
void pmu_setGPSPower(bool enable);

/**
 * Handle the AXP2101 IRQ flags.
 * This function has to be called periodically.
 */
void pmu_handleIRQ();

/**
 * Get the current status of the power button input.
 * The function returns zero if the button is released, one if is pressed and
 * two in case of long press.
 *
 * @return current status of the power button.
 */
uint8_t pmu_pwrBtnStatus();

#ifdef __cplusplus
}
#endif

#endif // PMU_H

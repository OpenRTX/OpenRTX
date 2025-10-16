/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef BATTERY_H
#define BATTERY_H

#include <stdint.h>

/**
 * This function computes the battery's state of charge given its current voltage.
 * @param vbat: battery voltage in millivolt.
 * @return state of charge percentage, from 0% to 100%.
 */
uint8_t battery_getCharge(uint16_t vbat);

#endif /* BATTERY_H */

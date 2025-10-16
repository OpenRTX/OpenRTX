/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CH_SELECTOR_H
#define CH_SELECTOR_H

/**
 * Low-level driver for correct handling of channel selector knobs connected to
 * a quadrature encoder.
 * This header file only provides the API for driver initialisation and shutdown,
 * while the readout of current encoder position is provided by target-specific
 * sources by implementating platform_getChSelector().
 */

/**
 * Initialise channel selector driver.
 */
void chSelector_init();

/**
 * Terminate channel selector driver.
 */
void chSelector_terminate();

#endif /* CH_SELECTOR_H */

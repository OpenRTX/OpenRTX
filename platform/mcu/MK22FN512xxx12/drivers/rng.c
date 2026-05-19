/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "peripherals/rng.h"
#include "MK22F51212.h"

void rng_init()
{
    SIM->SCGC6 |= SIM_SCGC6_RNGA(1);
    RNG->CR    |= RNG_CR_GO(1);
}

void rng_terminate()
{
    SIM->SCGC6 &= ~SIM_SCGC6_RNGA(1);
}

uint32_t rng_get()
{
    // Wait until there is some data
    while((RNG->SR & RNG_SR_OREG_LVL_MASK) == 0) ;

    return RNG->OR;
}

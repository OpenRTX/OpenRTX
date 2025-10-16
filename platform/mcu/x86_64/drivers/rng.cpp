/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "peripherals/rng.h"
#include <random>
#include <limits>

static std::mt19937 rng;

void rng_init()
{
    std::random_device seed;
    rng.seed(seed());
}

void rng_terminate()
{

}

uint32_t rng_get()
{
    std::uniform_int_distribution< uint32_t >
    distribution(0, std::numeric_limits<uint32_t>::max());

    return distribution(rng);
}

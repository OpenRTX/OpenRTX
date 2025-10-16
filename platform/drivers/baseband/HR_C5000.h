/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HRC5000_H
#define HRC5000_H

#include "drivers/baseband/HR_Cx000.h"

enum class C5000_SpiOpModes : uint8_t
{
    CONFIG = 0,     ///< Main configuration registers.
    AUX    = 1,     ///< Auxiliary configuration registers.
    DATA   = 2,     ///< Data register.
    SOUND  = 3,     ///< Voice prompt sample register.
    CMX638 = 4,     ///< CMX638 configuration register.
    AMBE3K = 5      ///< AMBE3000 configuration register.
};

using HR_C5000 = HR_Cx000 < C5000_SpiOpModes >;

#endif // HRC5000_H

/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/M17/M17DSP.hpp"
#include "hwconfig.h"

#ifdef CONFIG_M17
Fir< std::tuple_size< decltype(M17::rrc_taps_48k) >::value > M17::rrc_48k(M17::rrc_taps_48k);
Fir< std::tuple_size< decltype(M17::rrc_taps_24k) >::value > M17::rrc_24k(M17::rrc_taps_24k);
#endif

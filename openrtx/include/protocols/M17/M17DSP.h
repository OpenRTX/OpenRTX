/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Wojciech Kaczmarski SP5WWP                      *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef M17_DSP_H
#define M17_DSP_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <dsp.h>
#include <array>

namespace M17
{

/*
 * Coefficients for M17 RRC filter
 */
extern const std::array<float, 81> rrc_taps;

/*
 * FIR implementation of the RRC filter for baseband audio generation.
 */
extern Fir< std::tuple_size< decltype(rrc_taps) >::value > rrc;

} /* M17 */

#endif /* M17_DSP_H */
